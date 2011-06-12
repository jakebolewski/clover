#include "test_commandqueue.h"
#include "CL/cl.h"

#include <stdio.h>
#include <unistd.h>

START_TEST (test_create_command_queue)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_int result;
    
    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to get the default device"
    );
    
    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS || ctx == 0,
        "unable to create a valid context"
    );
    
    queue = clCreateCommandQueue(0, device, 0, &result);
    fail_if(
        result != CL_INVALID_CONTEXT,
        "context must be valid"
    );
    
    queue = clCreateCommandQueue(ctx, 0, 0, &result);
    fail_if(
        result != CL_INVALID_DEVICE,
        "device cannot be NULL"
    );
    
    // HACK : may crash if implementation changes, even if the test should pass.
    queue = clCreateCommandQueue(ctx, (cl_device_id)1337, 0, &result);
    fail_if(
        result != CL_INVALID_DEVICE,
        "1337 is not a device associated to the context"
    );
    
    queue = clCreateCommandQueue(ctx, device, 1337, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "1337 is not a valid value for properties"
    );
    
    queue = clCreateCommandQueue(ctx, device, 0, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );
    
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
}
END_TEST

START_TEST (test_get_command_queue_info)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_int result;
    
    union {
        cl_context ctx;
        cl_device_id device;
        cl_uint refcount;
        cl_command_queue_properties properties;
    } info;
    
    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to get the default device"
    );
    
    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS || ctx == 0,
        "unable to create a valid context"
    );
    
    queue = clCreateCommandQueue(ctx, device, 0, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );
    
    result = clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, sizeof(cl_context),
                                   (void *)&info, 0);
    fail_if(
        result != CL_SUCCESS || info.ctx != ctx,
        "the queue doesn't retain its context"
    );
    
    result = clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(cl_device_id),
                                   (void *)&info, 0);
    fail_if(
        result != CL_SUCCESS || info.device != device,
        "the queue doesn't retain its device"
    );
    
    result = clGetCommandQueueInfo(queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint),
                                   (void *)&info, 0);
    fail_if(
        result != CL_SUCCESS || info.refcount != 1,
        "the queue must have a refcount of 1 when it's created"
    );
    
    result = clGetCommandQueueInfo(queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties),
                                   (void *)&info, 0);
    fail_if(
        result != CL_SUCCESS || info.properties != 0,
        "we gave no properties to the command queue"
    );
    
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
}
END_TEST

START_TEST (test_object_tree)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_int result;
    cl_uint refcount;
    
    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to get the default device"
    );
    
    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS || ctx == 0,
        "unable to create a valid context"
    );
    
    queue = clCreateCommandQueue(ctx, device, 0, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );
    
    result = clGetContextInfo(ctx, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_uint),
                              (void *)&refcount, 0);
    fail_if(
        result != CL_SUCCESS || refcount != 2,
        "the queue must increment the refcount of its context"
    );
    
    clReleaseCommandQueue(queue);
    
    result = clGetContextInfo(ctx, CL_CONTEXT_REFERENCE_COUNT, sizeof(cl_uint),
                              (void *)&refcount, 0);
    fail_if(
        result != CL_SUCCESS || refcount != 1,
        "the queue must decrement the refcount of its context when it's destroyed"
    );
    
    clReleaseContext(ctx);
}
END_TEST

static void event_notify(cl_event event, cl_int exec_status, void *user_data)
{
    unsigned char *good = (unsigned char *)user_data;
    
    *good = 1;
}

START_TEST (test_events)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_int result;
    cl_event user_event, write_event;
    cl_mem buf;
    
    char s[] = "Original content";
    unsigned char good = 0;
    
    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to get the default device"
    );
    
    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS || ctx == 0,
        "unable to create a valid context"
    );
    
    queue = clCreateCommandQueue(ctx, device, 0, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );
    
    user_event = clCreateUserEvent(0, &result);
    fail_if(
        result != CL_INVALID_CONTEXT,
        "0 is not a valid context"
    );
    
    user_event = clCreateUserEvent(ctx, &result);
    fail_if(
        result != CL_SUCCESS || user_event == 0,
        "cannot create an user event"
    );
    
    buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                         sizeof(s), s, &result);
    fail_if(
        result != CL_SUCCESS,
        "cannot create a valid CL_MEM_USE_HOST_PTR read-write buffer"
    );
    
    // Queue a write buffer
    result = clEnqueueWriteBuffer(queue, buf, 0, 0, 8, "Modified", 1, 
                                  &user_event, &write_event);
    fail_if(
        result != CL_SUCCESS,
        "cannot enqueue an asynchronous write buffer command"
    );
    
    result = clSetEventCallback(write_event, CL_SUBMITTED, &event_notify, &good);
    fail_if(
        result != CL_INVALID_VALUE,
        "callback_type must be CL_COMPLETE in OpenCL 1.1"
    );
    
    result = clSetEventCallback(write_event, CL_COMPLETE, &event_notify, &good);
    fail_if(
        result != CL_COMPLETE,
        "cannot register an event callback"
    );
    
    sleep(1); // Let the worker threads a chance to do faulty things
    
    fail_if(
        good != 0 || strncmp(s, "Original content", sizeof(s)),
        "at this time, nothing can have happened, the user event isn't complete"
    );
    
    // Now we can execute everything
    result = clSetUserEventStatus(write_event, CL_COMPLETE);
    fail_if(
        result != CL_INVALID_EVENT,
        "write_event is not an user event"
    );
    
    result = clSetUserEventStatus(user_event, CL_SUBMITTED);
    fail_if(
        result != CL_INVALID_VALUE,
        "the execution status must be CL_COMPLETE"
    );
    
    result = clSetUserEventStatus(user_event, CL_COMPLETE);
    fail_if(
        result != CL_SUCCESS,
        "cannot set the user event as completed"
    );
    
    // And wait (TODO: More careful checks of this function)
    result = clWaitForEvents(1, &write_event);
    fail_if(
        result != CL_SUCCESS,
        "cannot wait for events"
    );
    
    // Checks that all went good
    fail_if(
        good != 1,
        "the callback function must be called when an event is completed"
    );
    fail_if(
        strncmp(s, "Modified content", sizeof(s)),
        "the buffer must contain \"Modified content\""
    );
    
    result = clSetUserEventStatus(user_event, CL_COMPLETE);
    fail_if(
        result != CL_INVALID_OPERATION,
        "we cannot call clSetUserEventStatus two times for an event"
    );
}
END_TEST

TCase *cl_commandqueue_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("commandqueue");
    tcase_add_test(tc, test_create_command_queue);
    tcase_add_test(tc, test_get_command_queue_info);
    tcase_add_test(tc, test_object_tree);
    tcase_add_test(tc, test_events);
    return tc;
}
