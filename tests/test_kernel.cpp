#include "test_kernel.h"
#include "CL/cl.h"

#include <stdio.h>

static void native_kernel(void *args)
{
    struct ags
    {
        size_t buffer_size;
        char *buffer;
    };
    
    struct ags *data = (struct ags *)args;
    int i;
    
    // Not
    for (int i=0; i<data->buffer_size; ++i)
    {
        data->buffer[i] = ~data->buffer[i];
    }
}

START_TEST (test_native_kernel)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_int result;
    cl_event events[2];
    cl_mem buf1, buf2;
    
    char s1[] = "Lorem ipsum dolor sit amet";
    char s2[] = "I want to tell you that you rock";
    
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
    
    queue = clCreateCommandQueue(ctx, device, 
                                 CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );
    
    buf1 = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, 
                          sizeof(s1), (void *)&s1, &result);
    fail_if(
        result != CL_SUCCESS || buf1 == 0,
        "cannot create a buffer"
    );
    
    buf2 = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                          sizeof(s2), (void *)&s2, &result);
    fail_if(
        result != CL_SUCCESS || buf2 == 0,
        "cannot create a buffer"
    );
    
    struct
    {
        size_t buffer_size;
        char *buffer;
    } args;
    
    args.buffer_size = sizeof(s1);
    args.buffer = 0;
    
    const void *mem_loc = (const void *)&args.buffer;
    
    result = clEnqueueNativeKernel(queue, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    fail_if(
        result != CL_INVALID_VALUE,
        "user_func cannot be NULL"
    );
    
    result = clEnqueueNativeKernel(queue, &native_kernel, 0, sizeof(args),
                                   1, &buf1, &mem_loc, 0, 0, 
                                   &events[0]);
    fail_if(
        result != CL_INVALID_VALUE,
        "args cannot be NULL when cb_args != 0"
    );
    
    result = clEnqueueNativeKernel(queue, &native_kernel, &args, sizeof(args),
                                   1, 0, &mem_loc, 0, 0, 
                                   &events[0]);
    fail_if(
        result != CL_INVALID_VALUE,
        "mem_list cannot be NULL when num_mem_objects != 0"
    );
    
    result = clEnqueueNativeKernel(queue, &native_kernel, &args, sizeof(args),
                                   1, &buf1, 0, 0, 0, &events[0]);
    fail_if(
        result != CL_INVALID_VALUE,
        "args_mem_loc cannot be NULL when num_mem_objects != 0"
    );
    
    result = clEnqueueNativeKernel(queue, &native_kernel, &args, sizeof(args),
                                   1, &buf1, &mem_loc, 0, 0, 
                                   &events[0]);
    fail_if(
        result != CL_SUCCESS,
        "unable to enqueue native kernel nr 1"
    );
    
    args.buffer_size = sizeof(s2);
    
    result = clEnqueueNativeKernel(queue, &native_kernel, &args, sizeof(args),
                                   1, &buf2, &mem_loc, 0, 0, 
                                   &events[1]);
    fail_if(
        result != CL_SUCCESS,
        "unable to enqueue native kernel nr 2"
    );
    
    // Wait for events
    result = clWaitForEvents(2, events);
    fail_if(
        result != CL_SUCCESS,
        "unable to wait for events"
    );
    
    fail_if(
        s1[0] != ~'L' || s2[0] != ~'I',
        "the native kernel hasn't done its job"
    );
    
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
}
END_TEST

TCase *cl_kernel_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("kernel");
    tcase_add_test(tc, test_native_kernel);
    return tc;
}
