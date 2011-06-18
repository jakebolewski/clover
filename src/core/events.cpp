#include "events.h"
#include "commandqueue.h"
#include "memobject.h"
#include "deviceinterface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace Coal;

/*
 * Read/Write buffers
 */

BufferEvent::BufferEvent(CommandQueue *parent, 
                         MemObject *buffer,
                         size_t offset,
                         size_t cb,
                         void *ptr,
                         cl_map_flags map_flags,
                         EventType type,
                         cl_uint num_events_in_wait_list, 
                         const Event **event_wait_list,
                         cl_int *errcode_ret)
: Event(parent, Queued, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_buffer(buffer), p_offset(offset), p_cb(cb), p_ptr(ptr), 
  p_map_flags(map_flags), p_type(type)
{
    // Correct buffer
    if (!buffer)
    {
        *errcode_ret = CL_INVALID_MEM_OBJECT;
        return;
    }
    
    // Buffer's context must match the CommandQueue one
    Context *ctx = 0;
    *errcode_ret = parent->info(CL_QUEUE_CONTEXT, sizeof(Context *), &ctx, 0);
    
    if (errcode_ret != CL_SUCCESS) return;
    
    if (buffer->context() != ctx)
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return;
    }
    
    // Check for out-of-bounds reads
    if (!ptr)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    if (offset + cb > buffer->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    // Map flags
    if (type == Event::MapBuffer)
    {
        if (map_flags | ~(CL_MAP_READ | CL_MAP_WRITE))
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }
    }
    
    // Alignment of SubBuffers
    DeviceInterface *device = 0;
    cl_uint align;
    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *), 
                                &device, 0);
    
    if (errcode_ret != CL_SUCCESS) return;
    
    if (buffer->type() == MemObject::SubBuffer)
    {
        *errcode_ret = device->info(CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(uint),
                                    &align, 0);
        
        if (errcode_ret != CL_SUCCESS) return;
        
        size_t mask = 0;
        
        for (int i=0; i<align; ++i)
            mask = 1 | (mask << 1);
        
        if (((SubBuffer *)buffer)->offset() | mask)
        {
            *errcode_ret = CL_MISALIGNED_SUB_BUFFER_OFFSET;
            return;
        }
    }
    
    // Allocate the buffer for the device
    if (!buffer->allocate(device))
    {
        *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE;
        return;
    }
}

MemObject *BufferEvent::buffer() const
{
    return p_buffer;
}

size_t BufferEvent::offset() const
{
    return p_offset;
}

size_t BufferEvent::cb() const
{
    return p_cb;
}

void *BufferEvent::ptr() const
{
    return p_ptr;
}

void BufferEvent::setPtr(void *ptr)
{
    p_ptr = ptr;
}

Event::EventType BufferEvent::type() const
{
    return p_type;
}

/*
 * Native kernel
 */
NativeKernelEvent::NativeKernelEvent(CommandQueue *parent, 
                                     void (*user_func)(void *), 
                                     void *args, 
                                     size_t cb_args, 
                                     cl_uint num_mem_objects, 
                                     const MemObject **mem_list, 
                                     const void **args_mem_loc, 
                                     cl_uint num_events_in_wait_list, 
                                     const Event **event_wait_list,
                                     cl_int *errcode_ret) 
: Event (parent, Queued, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_user_func((void *)user_func), p_args(0)
{
    // Parameters sanity
    if (!user_func)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    if (!args && (cb_args || num_mem_objects))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    if (args && !cb_args)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    if (num_mem_objects && (!mem_list || !args_mem_loc))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    if (!num_mem_objects && (mem_list || args_mem_loc))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
    
    // Check that the device can execute a native kernel
    DeviceInterface *device;
    cl_device_exec_capabilities caps;
    
    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *), 
                                &device, 0);
    
    if (*errcode_ret != CL_SUCCESS)
        return;
    
    *errcode_ret = device->info(CL_DEVICE_EXECUTION_CAPABILITIES,
                                sizeof(cl_device_exec_capabilities), &caps, 0);
    
    if (*errcode_ret != CL_SUCCESS)
        return;
    
    if ((caps & CL_EXEC_NATIVE_KERNEL) == 0)
    {
        *errcode_ret = CL_INVALID_OPERATION;
        return;
    }
    
    // Copy the arguments in a new list
    if (cb_args)
    {
        p_args = malloc(cb_args);
        
        if (!p_args)
        {
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return;
        }
        
        memcpy((void *)p_args, (void *)args, cb_args);
        
        // Replace memory objects with global pointers
        for (int i=0; i<num_mem_objects; ++i)
        {
            const MemObject *buffer = mem_list[i];
            const char *loc = (const char *)args_mem_loc[i];
            
            if (!buffer)
            {
                *errcode_ret = CL_INVALID_MEM_OBJECT;
                return;
            }
            
            // We need to do relocation : loc is in args, we need it in p_args
            size_t delta = (char *)p_args - (char *)args;
            loc += delta;
            
            *(void **)loc = buffer->deviceBuffer(device)->nativeGlobalPointer();
        }
    }
}

NativeKernelEvent::~NativeKernelEvent()
{
    if (p_args)
        free((void *)p_args);
}

Event::EventType NativeKernelEvent::type() const
{
    return Event::NativeKernel;
}

void *NativeKernelEvent::function() const
{
    return p_user_func;
}

void *NativeKernelEvent::args() const
{
    return p_args;
}

/*
 * User event
 */
UserEvent::UserEvent(Context *context, cl_int *errcode_ret)
: Event(0, Submitted, 0, 0, errcode_ret), p_context(context)
{}
        
Event::EventType UserEvent::type() const
{
    return Event::User;
}

Context *UserEvent::context() const
{
    return p_context;
}

void UserEvent::addDependentCommandQueue(CommandQueue *queue)
{
    std::vector<CommandQueue *>::const_iterator it;
    
    for (it = p_dependent_queues.begin(); it != p_dependent_queues.end(); ++it)
        if (*it == queue)
            return;
    
    p_dependent_queues.push_back(queue);
}

void UserEvent::flushQueues()
{
    std::vector<CommandQueue *>::const_iterator it;
    
    for (it = p_dependent_queues.begin(); it != p_dependent_queues.end(); ++it)
        (*it)->pushEventsOnDevice();
}