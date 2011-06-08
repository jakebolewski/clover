#include <CL/cl.h>

#include <core/commandqueue.h>

// Event Object APIs
cl_int
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list)
{
    if (!num_events || !event_list)
        return CL_INVALID_VALUE;
    
    // Check the events in the list
    cl_context global_ctx = 0;
    
    for (int i=0; i<num_events; ++i)
    {
        if (!event_list[i])
            return CL_INVALID_EVENT;
        
        if (event_list[i]->status() < 0)
            return CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
        
        cl_context evt_ctx = 0;
        cl_int rs;
        
        rs = event_list[i]->info(CL_EVENT_CONTEXT, sizeof(cl_context), &evt_ctx,
                                 0);
        
        if (rs != CL_SUCCESS) return rs;
        
        if (global_ctx == 0)
            global_ctx = evt_ctx;
        else if (global_ctx != evt_ctx)
            return CL_INVALID_CONTEXT;
    }
    
    // Wait for the events
    for (int i=0; i<num_events; ++i)
    {
        event_list[i]->waitForStatus(Coal::Event::Complete);
    }
    
    return CL_SUCCESS;
}

cl_int
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret)
{
    if (!event)
        return CL_INVALID_EVENT;
    
    return event->info(param_name, param_value_size, param_value, 
                       param_value_size_ret);
}

cl_int
clSetEventCallback(cl_event     event,
                   cl_int       command_exec_callback_type,
                   void         (CL_CALLBACK *pfn_event_notify)(cl_event event,
                                                                cl_int exec_status,
                                                                void *user_data),
                   void *user_data)
{
    if (!event)
        return CL_INVALID_EVENT;
    
    if (!pfn_event_notify || command_exec_callback_type != CL_COMPLETE)
        return CL_INVALID_VALUE;
    
    event->setCallback(command_exec_callback_type, pfn_event_notify, user_data);
    
    return CL_SUCCESS;
}

cl_int
clRetainEvent(cl_event event)
{
    if (!event)
        return CL_INVALID_EVENT;
    
    event->reference();
    
    return CL_SUCCESS;
}

cl_int
clReleaseEvent(cl_event event)
{
    if (!event)
        return CL_INVALID_EVENT;
    
    if (event->dereference())
        delete event;
    
    return CL_SUCCESS;
}

cl_event
clCreateUserEvent(cl_context    context,
                  cl_int *      errcode_ret)
{
    return 0;
}

cl_int
clSetUserEventStatus(cl_event   event,
                     cl_int     execution_status)
{
    return 0;
}
