#include "commandqueue.h"
#include "context.h"
#include "deviceinterface.h"
#include "propertylist.h"
#include "events.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Coal;

/*
 * CommandQueue
 */

CommandQueue::CommandQueue(Context *ctx,
                           DeviceInterface *device,
                           cl_command_queue_properties properties, 
                           cl_int *errcode_ret)
: p_ctx(ctx), p_device(device), p_properties(properties), p_references(1)
{
    // Increment the reference count of the context
    // We begin by doing this to be able to unconditionnally release the context
    // in the destructor, being certain that the context was actually retained.
    clRetainContext((cl_context)ctx);
    
    // Initialize the locking machinery
    pthread_mutex_init(&p_event_list_mutex, 0);
    
    // Check that the device belongs to the context
    if (!ctx->hasDevice(device))
    {
        *errcode_ret = CL_INVALID_DEVICE;
        return;
    }
    
    *errcode_ret = checkProperties();
}

CommandQueue::~CommandQueue()
{
    // Free the mutex
    pthread_mutex_destroy(&p_event_list_mutex);
    
    // Release the parent context
    clReleaseContext((cl_context)p_ctx);
}

void CommandQueue::reference()
{
    p_references++;
}

bool CommandQueue::dereference()
{
    p_references--;
    return (p_references == 0);
}

cl_int CommandQueue::info(cl_context_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret)
{
    void *value = 0;
    int value_length = 0;
    
    union {
        cl_uint cl_uint_var;
        cl_device_id cl_device_id_var;
        cl_context cl_context_var;
        cl_command_queue_properties cl_command_queue_properties_var;
    };
    
    switch (param_name)
    {
        case CL_QUEUE_CONTEXT:
            SIMPLE_ASSIGN(cl_context, p_ctx);
            break;
            
        case CL_QUEUE_DEVICE:
            SIMPLE_ASSIGN(cl_device_id, p_device);
            break;
            
        case CL_QUEUE_REFERENCE_COUNT:
            SIMPLE_ASSIGN(cl_uint, p_references);
            break;
            
        case CL_QUEUE_PROPERTIES:
            SIMPLE_ASSIGN(cl_command_queue_properties, p_properties);
            break;
            
        default:
            return CL_INVALID_VALUE;
    }
    
    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;
    
    if (param_value_size_ret)
        *param_value_size_ret = value_length;
        
    if (param_value)
        memcpy(param_value, value, value_length);
    
    return CL_SUCCESS;
}
        
cl_int CommandQueue::setProperty(cl_command_queue_properties properties,
                                 cl_bool enable,
                                 cl_command_queue_properties *old_properties)
{
    if (old_properties)
        *old_properties = p_properties;
    
    if (enable)
        p_properties |= properties;
    else
        p_properties &= ~properties;
    
    return checkProperties();
}

cl_int CommandQueue::checkProperties() const
{
    // Check that all the properties are valid
    cl_command_queue_properties properties = 
        CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
        CL_QUEUE_PROFILING_ENABLE;
        
    if ((p_properties & properties) != p_properties)
        return CL_INVALID_VALUE;
    
    // Check that the device handles these properties
    cl_int result;
    
    result = p_device->info(CL_DEVICE_QUEUE_PROPERTIES,
                            sizeof(cl_command_queue_properties),
                            &properties,
                            0);
    
    if (result != CL_SUCCESS)
        return result;
    
    if ((p_properties & properties) != p_properties)
        return CL_INVALID_QUEUE_PROPERTIES;
    
    return CL_SUCCESS;
}

void CommandQueue::queueEvent(Event *event)
{
    // Append the event at the end of the list
    pthread_mutex_lock(&p_event_list_mutex);
    
    p_events.push_back(event);
    
    pthread_mutex_unlock(&p_event_list_mutex);
    
    // Explore the list for events we can push on the device
    pushEventsOnDevice();
}

void CommandQueue::cleanEvents()
{
    // NOTE: This function must only be called from the main thread
    pthread_mutex_lock(&p_event_list_mutex);
    
    std::list<Event *>::iterator it = p_events.begin();
    
    for (; it != p_events.end(); ++it)
    {
        Event *event = *it;
        
        if (event->status() == Event::Complete)
        {
            // We cannot be deleted from inside us
            event->setReleaseParent(false);
            
            p_events.erase(it);
            clReleaseEvent((cl_event)event);
        }
    }
    
    pthread_mutex_unlock(&p_event_list_mutex);
    
    // Check now if we have to be deleted
    if (p_references == 0)
        delete this;
}

void CommandQueue::pushEventsOnDevice()
{
    pthread_mutex_lock(&p_event_list_mutex);
    // Explore the events in p_events and push on the device all of them that
    // are :
    //
    // - Not already pushed (in Event::Queued state)
    // - Not after a barrier, except if we begin with a barrier
    // - If we are in-order, only the first event in Event::Queued state can
    //   be pushed
    
    std::list<Event *>::iterator it = p_events.begin();
    bool first = true;
    
    for (; it != p_events.end(); ++it)
    {
        Event *event = *it;
        
        // If the event is completed, remove it
        if (event->status() == Event::Complete)
            continue;
        
        // We cannot do out-of-order, so we can only push the first event.
        if ((p_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) == 0 &&
            !first)
            break;
        
        // If we encounter a barrier, check if it's the first in the list
        if (event->type() == Event::Barrier)
        {
            if (first)
            {
                // Remove the barrier, we don't need it anymore
                p_events.erase(it);
                continue;
            }
            else
            {
                // We have events to wait, stop
                break;
            }
        }
        
        // Completed events and first barriers are out, it remains real events
        // that have to block in-order execution.
        first = false;
        
        // If the event is not "pushable" (in Event::Queued state), skip it
        if (event->status() != Event::Queued)
            continue;
        
        // Check that all the waiting-on events of this event are finished
        cl_uint count;
        const Event **event_wait_list;
        bool skip_event = false;
        
        event_wait_list = event->waitEvents(count);
        
        for (int i=0; i<count; ++i)
        {
            if (event_wait_list[i]->status() != Event::Complete)
            {
                skip_event = true;
                break;
            }
        }
        
        if (skip_event)
            continue;
            
        // The event can be pushed, if we need to
        if (!event->isDummy())
        {
            event->setStatus(Event::Submitted);
            p_device->pushEvent(event);
        }
        else
        {
            // Set the event as completed. This will call pushEventsOnDevice,
            // again, so release the lock to avoid a deadlock. We also return
            // because the recursive call will continue our work.
            pthread_mutex_unlock(&p_event_list_mutex);
            event->setStatus(Event::Complete);
            return;
        }
    }
    
    pthread_mutex_unlock(&p_event_list_mutex);
}

/*
 * Event
 */

Event::Event(CommandQueue *parent,
             EventStatus status,
             cl_uint num_events_in_wait_list, 
             const Event **event_wait_list,
             cl_int *errcode_ret)
: p_references(1), p_parent(parent), 
  p_num_events_in_wait_list(num_events_in_wait_list), p_event_wait_list(0),
  p_device_data(0), p_status(status), p_release_parent(true)
{
    // Retain our parent
    if (parent)
        clRetainCommandQueue((cl_command_queue)p_parent);
    
    // Initialize the locking machinery
    pthread_cond_init(&p_state_change_cond, 0);
    pthread_mutex_init(&p_state_mutex, 0);
    
    // Check sanity of parameters
    if (!event_wait_list && num_events_in_wait_list)
    {
        *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
        return;
    }
    
    if (event_wait_list && !num_events_in_wait_list)
    {
        *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
        return;
    }
    
    // Check that none of the events in event_wait_list is in an error state
    for (int i=0; i<num_events_in_wait_list; ++i)
    {
        if (event_wait_list[i] == 0)
        {
            *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
            return;
        }
        else if (event_wait_list[i]->status() < 0)
        {
            *errcode_ret = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
            return;
        }
    }
    
    // Allocate a new buffer for the events to wait
    if (num_events_in_wait_list)
    {
        const unsigned int len = num_events_in_wait_list * sizeof(Event *);
        p_event_wait_list = (const Event **)malloc(len);
        
        if (!p_event_wait_list)
        {
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return;
        }
        
        memcpy((void *)p_event_wait_list, (void *)event_wait_list, len);
    }
    
    // Explore the events we are waiting on and reference them
    for (int i=0; i<num_events_in_wait_list; ++i)
    {
        clRetainEvent((cl_event)event_wait_list[i]);
        
        if (event_wait_list[i]->type() == Event::User && parent)
            ((UserEvent *)event_wait_list[i])->addDependentCommandQueue(parent);
    }
}

Event::~Event()
{
    if (p_event_wait_list)
        free((void *)p_event_wait_list);
    
    pthread_mutex_destroy(&p_state_mutex);
    pthread_cond_destroy(&p_state_change_cond);
    
    if (p_parent)
    {
        if (p_release_parent)
            clReleaseCommandQueue((cl_command_queue)p_parent);
        else
            p_parent->dereference();  // Dereference but don't delete
    }
}

void Event::setReleaseParent(bool release)
{
    p_release_parent = release;
}

bool Event::isSingleShot() const
{
    // NDRangeKernel is a single event that can be executed on several execution
    // units. The other (buffer copying) must be executed in one part.
    
    return (type() != NDRangeKernel);
}

bool Event::isDummy() const
{
    // A dummy event has nothing to do on an execution device and must be
    // completed directly after being "submitted".
    
    switch (type())
    {
        case Marker:
        case User:
        case Barrier:
            return true;
            
        default:
            return false;
    }
}

void Event::reference()
{
    p_references++;
}

bool Event::dereference()
{
    p_references--;
    return (p_references == 0);
}

void Event::setStatus(EventStatus status)
{
    pthread_mutex_lock(&p_state_mutex);
    p_status = status;
    
    pthread_cond_broadcast(&p_state_change_cond);
    
    // Call the callbacks
    std::multimap<EventStatus, CallbackData>::const_iterator it;
    std::pair<std::multimap<EventStatus, CallbackData>::const_iterator,
              std::multimap<EventStatus, CallbackData>::const_iterator> ret;

    ret = p_callbacks.equal_range(status > 0 ? status : Complete);
    
    for (it=ret.first; it!=ret.second; ++it)
    {
        const CallbackData &data = (*it).second;
        data.callback((cl_event)this, p_status, data.user_data);
    }
    
    pthread_mutex_unlock(&p_state_mutex);
    
    // If the event is completed, inform our parent so it can push other events
    // to the device.
    if (p_parent && status == Complete)
        p_parent->pushEventsOnDevice();
    else if (type() == Event::User)
        ((UserEvent *)this)->flushQueues();
}

void Event::setDeviceData(void *data)
{
    p_device_data = data;
}

Event::EventStatus Event::status() const
{
    // HACK : We need const qualifier but we also need to lock a mutex
    Event *me = (Event *)(void *)this;
    
    pthread_mutex_lock(&me->p_state_mutex);
    
    EventStatus ret = p_status;
    
    pthread_mutex_unlock(&me->p_state_mutex);
    
    return ret;
}

void Event::waitForStatus(EventStatus status)
{
    pthread_mutex_lock(&p_state_mutex);
    
    while (p_status != status && p_status > 0)
    {
        pthread_cond_wait(&p_state_change_cond, &p_state_mutex);
    }
    
    pthread_mutex_unlock(&p_state_mutex);
}

void *Event::deviceData()
{
    return p_device_data;
}

const Event **Event::waitEvents(cl_uint &count) const
{
    count = p_num_events_in_wait_list;
    return p_event_wait_list;
}

void Event::setCallback(cl_int command_exec_callback_type, 
                        event_callback callback,
                        void *user_data)
{
    CallbackData data;
    
    data.callback = callback;
    data.user_data = user_data;
    
    pthread_mutex_lock(&p_state_mutex);
    
    p_callbacks.insert(std::pair<EventStatus, CallbackData>(
        (EventStatus)command_exec_callback_type,
        data));
    
    pthread_mutex_unlock(&p_state_mutex);
}

cl_int Event::info(cl_context_info param_name,
                   size_t param_value_size,
                   void *param_value,
                   size_t *param_value_size_ret)
{
    void *value = 0;
    int value_length = 0;
    
    union {
        cl_command_queue cl_command_queue_var;
        cl_context cl_context_var;
        cl_command_type cl_command_type_var;
        cl_int cl_int_var;
        cl_uint cl_uint_var;
    };
    
    switch (param_name)
    {
        case CL_EVENT_COMMAND_QUEUE:
            SIMPLE_ASSIGN(cl_command_queue, p_parent);
            break;
            
        case CL_EVENT_CONTEXT:
            if (p_parent)
            {
                // Tail call to CommandQueue
                return p_parent->info(CL_QUEUE_CONTEXT, param_value_size,
                                    param_value, param_value_size_ret);
            }
            else
            {
                if (type() == User)
                    SIMPLE_ASSIGN(cl_context, ((UserEvent *)this)->context())
                else
                    SIMPLE_ASSIGN(cl_context, 0);
            }
            
        case CL_EVENT_COMMAND_TYPE:
            SIMPLE_ASSIGN(cl_command_type, type());
            break;
            
        case CL_EVENT_COMMAND_EXECUTION_STATUS:
            SIMPLE_ASSIGN(cl_int, status());
            break;
            
        case CL_EVENT_REFERENCE_COUNT:
            SIMPLE_ASSIGN(cl_uint, p_references);
            break;
            
        default:
            return CL_INVALID_VALUE;
    }
    
    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;
    
    if (param_value_size_ret)
        *param_value_size_ret = value_length;
        
    if (param_value)
        memcpy(param_value, value, value_length);
    
    return CL_SUCCESS;
}