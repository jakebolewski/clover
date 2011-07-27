#include <iostream>

#include "test_mem.h"
#include "CL/cl.h"

START_TEST (test_create_buffer)
{
    cl_context ctx;
    cl_mem buf;
    cl_int result;
    char s[] = "Hello, world !";

    ctx = clCreateContextFromType(0, CL_DEVICE_TYPE_CPU, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS,
        "unable to create a valid context"
    );

    buf = clCreateBuffer(0, CL_MEM_READ_WRITE, sizeof(s), 0, &result);
    fail_if(
        result != CL_INVALID_CONTEXT,
        "0 is not a valid context"
    );

    buf = clCreateBuffer(ctx, 1337, sizeof(s), 0, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "1337 is not a valid cl_mem_flags"
    );

    buf = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR, sizeof(s), 0, &result);
    fail_if(
        result != CL_INVALID_HOST_PTR,
        "host_ptr cannot be NULL if flags is CL_MEM_USE_HOST_PTR"
    );

    buf = clCreateBuffer(ctx, CL_MEM_COPY_HOST_PTR, sizeof(s), 0, &result);
    fail_if(
        result != CL_INVALID_HOST_PTR,
        "host_ptr cannot be NULL if flags is CL_MEM_COPY_HOST_PTR"
    );

    buf = clCreateBuffer(ctx, 0, sizeof(s), s, &result);
    fail_if(
        result != CL_INVALID_HOST_PTR,
        "host_ptr must be NULL if flags is not CL_MEM_{COPY/USE}_HOST_PTR"
    );

    buf = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR, 0, s, &result);
    fail_if(
        result != CL_INVALID_BUFFER_SIZE,
        "size cannot be 0"
    );

    buf = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                         sizeof(s), s, &result);
    fail_if(
        result != CL_SUCCESS,
        "cannot create a valid CL_MEM_COPY_HOST_PTR read-write buffer"
    );

    clReleaseMemObject(buf);
    clReleaseContext(ctx);
}
END_TEST

START_TEST (test_create_sub_buffer)
{
    cl_context ctx;
    cl_mem buf, subbuf;
    cl_int result;
    char s[] = "Hello, world !";

    cl_buffer_region create_info;    // "Hello, [world] !"

    create_info.origin = 7;
    create_info.size = 5;

    ctx = clCreateContextFromType(0, CL_DEVICE_TYPE_CPU, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS,
        "unable to create a valid context"
    );

    buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                         sizeof(s), s, &result);
    fail_if(
        result != CL_SUCCESS,
        "cannot create a valid CL_MEM_USE_HOST_PTR read-write buffer"
    );

    subbuf = clCreateSubBuffer(0, CL_MEM_WRITE_ONLY,
                               CL_BUFFER_CREATE_TYPE_REGION,
                               (void *)&create_info, &result);
    fail_if(
        result != CL_INVALID_MEM_OBJECT,
        "0 is not a valid mem object"
    );

    subbuf = clCreateSubBuffer(buf, CL_MEM_READ_ONLY,
                               CL_BUFFER_CREATE_TYPE_REGION,
                               (void *)&create_info, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "READ_ONLY is not compatible with WRITE_ONLY"
    );

    subbuf = clCreateSubBuffer(buf, CL_MEM_WRITE_ONLY, 1337, (void *)&create_info,
                            &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "1337 is not a valid buffer_create_type"
    );

    subbuf = clCreateSubBuffer(buf, CL_MEM_WRITE_ONLY,
                            CL_BUFFER_CREATE_TYPE_REGION, 0, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "buffer_create_info cannot be NULL"
    );

    create_info.size = 0;

    subbuf = clCreateSubBuffer(buf, CL_MEM_WRITE_ONLY,
                            CL_BUFFER_CREATE_TYPE_REGION,
                            (void *)&create_info, &result);
    fail_if(
        result != CL_INVALID_BUFFER_SIZE,
        "create_info.size cannot be 0"
    );

    create_info.size = 5;

    subbuf = clCreateSubBuffer(buf, CL_MEM_WRITE_ONLY,
                            CL_BUFFER_CREATE_TYPE_REGION,
                            (void *)&create_info, &result);
    fail_if(
        result != CL_SUCCESS || subbuf == 0,
        "cannot create a valid sub-buffer"
    );

    clCreateSubBuffer(subbuf, CL_MEM_WRITE_ONLY,
                            CL_BUFFER_CREATE_TYPE_REGION,
                            (void *)&create_info, &result);
    fail_if(
        result != CL_INVALID_MEM_OBJECT,
        "we cannot create a sub-buffer of a sub-buffer"
    );

    clReleaseMemObject(subbuf);
    clReleaseMemObject(buf);
    clReleaseContext(ctx);
}
END_TEST

START_TEST (test_read_write_subbuf)
{
    cl_context ctx;
    cl_mem buf, subbuf;
    cl_command_queue queue;
    cl_device_id device;
    cl_int result;
    char s[] = "Hello, Denis !";

    cl_buffer_region create_info;

    create_info.origin = 7;      // "Hello, [denis] !"
    create_info.size = 5;

    result = clGetDeviceIDs(0, CL_DEVICE_TYPE_CPU, 1, &device, 0);
    fail_if(
        result != CL_SUCCESS,
        "cannot get a device"
    );

    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS,
        "unable to create a valid context"
    );

    queue = clCreateCommandQueue(ctx, device, 0, &result);
    fail_if(
        result != CL_SUCCESS || queue == 0,
        "cannot create a command queue"
    );

    buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                         sizeof(s), s, &result);
    fail_if(
        result != CL_SUCCESS,
        "cannot create a valid CL_MEM_USE_HOST_PTR read-write buffer"
    );

    subbuf = clCreateSubBuffer(buf, CL_MEM_WRITE_ONLY,
                            CL_BUFFER_CREATE_TYPE_REGION,
                            (void *)&create_info, &result);
    fail_if(
        result != CL_SUCCESS || subbuf == 0,
        "cannot create a valid sub-buffer"
    );

    ////
    char *hostptr;
    char *valid_hostptr = s;

    valid_hostptr += create_info.origin;

    result = clGetMemObjectInfo(subbuf, CL_MEM_HOST_PTR, sizeof(char *),
                                (void *)&hostptr, 0);
    fail_if(
        result != CL_SUCCESS || hostptr != valid_hostptr,
        "the host ptr of a subbuffer must point to a subportion of its parent buffer"
    );

    result = clEnqueueWriteBuffer(queue, subbuf, 1, 0, 5, "world", 0, 0, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to write to the sub buffer"
    );

    char data[16];

    result = clEnqueueReadBuffer(queue, subbuf, 1, 0, 5, data, 0, 0, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to read the sub buffer"
    );
    fail_if(
        strncmp(data, "world", 5),
        "the subbuffer must contain \"world\""
    );

    result = clEnqueueReadBuffer(queue, buf, 1, 0, sizeof(s), data, 0, 0, 0);
    fail_if(
        result != CL_SUCCESS,
        "unable to read the buffer"
    );
    fail_if(
        strncmp(data, "Hello, world !", sizeof(s)),
        "the buffer must contain \"Hello, world !\""
    );

    clReleaseCommandQueue(queue);
    clReleaseMemObject(subbuf);
    clReleaseMemObject(buf);
    clReleaseContext(ctx);
}
END_TEST

START_TEST (test_images)
{
    cl_context ctx;
    cl_mem image2d;
    cl_int result;

    unsigned char image2d_data_24bpp[] = {
        255, 0, 0,  0, 255, 0,
        0, 0, 255,  255, 255, 0
    };

    cl_image_format fmt;

    fmt.image_channel_data_type = CL_UNORM_INT8;
    fmt.image_channel_order = CL_RGB;

    ctx = clCreateContextFromType(0, CL_DEVICE_TYPE_CPU, 0, 0, &result);
    fail_if(
        result != CL_SUCCESS,
        "unable to create a valid context"
    );

    image2d = clCreateImage2D(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &fmt,
                              2, 2, 7, image2d_data_24bpp, &result);
    fail_if(
        result != CL_INVALID_IMAGE_SIZE,
        "7 is not a valid row pitch for 24bpp, it isn't divisible by 3"
    );

    image2d = clCreateImage2D(ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &fmt,
                              2, 2, 0, image2d_data_24bpp, &result);
    fail_if(
        result != CL_SUCCESS || image2d == 0,
        "cannot create a valid 2x2 image2D"
    );

    clReleaseMemObject(image2d);
    clReleaseContext(ctx);
}
END_TEST

TCase *cl_mem_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("mem");
    tcase_add_test(tc, test_create_buffer);
    tcase_add_test(tc, test_create_sub_buffer);
    tcase_add_test(tc, test_read_write_subbuf);
    tcase_add_test(tc, test_images);
    return tc;
}
