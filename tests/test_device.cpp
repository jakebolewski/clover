#include "test_device.h"
#include "CL/cl.h"

START_TEST (test_get_device_ids)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_uint num_devices;
    cl_int result;

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, &device, &num_devices);
    fail_if(
        result != CL_INVALID_VALUE,
        "num_entries cannot be NULL when devices is not null"
    );

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, 0, 0);
    fail_if(
        result != CL_INVALID_VALUE,
        "num_devices and devices cannot be NULL at the same time"
    );

    result = clGetDeviceIDs((cl_platform_id)1337, CL_DEVICE_TYPE_CPU, 1, &device, &num_devices);
    fail_if(
        result != CL_INVALID_PLATFORM,
        "1337 is not a valid platform"
    );

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &num_devices);
    fail_if(
        result != CL_DEVICE_NOT_FOUND,
        "there are no GPU devices currently available"
    );

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, 0, &num_devices);
    fail_if(
        result != CL_SUCCESS || num_devices != 1,
        "we must succeed and say that we have one CPU device"
    );

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &num_devices);
    fail_if(
        result != CL_SUCCESS || num_devices != 1 || device == 0,
        "we must succeed and have one CPU device"
    );
}
END_TEST

START_TEST (test_get_device_info)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_uint num_devices;
    cl_int result;

    size_t size_ret;
    char value[500];

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &num_devices);
    fail_if(
        result != CL_SUCCESS,
        "unable to get a CPU device"
    );

    result = clGetDeviceInfo(0, CL_DEVICE_TYPE, 500, value, &size_ret);
    fail_if(
        result != CL_INVALID_DEVICE,
        "0 is not a valid device"
    );

    result = clGetDeviceInfo(device, 13376334, 500, value, &size_ret);
    fail_if(
        result != CL_INVALID_VALUE,
        "13376334 is not a valid param_name"
    );

    result = clGetDeviceInfo(device, CL_DEVICE_TYPE, 1, value, &size_ret);
    fail_if(
        result != CL_INVALID_VALUE,
        "1 is too small to contain a cl_device_type"
    );

    result = clGetDeviceInfo(device, CL_DEVICE_TYPE, 0, 0, &size_ret);
    fail_if(
        result != CL_SUCCESS || size_ret != sizeof(cl_device_type),
        "we have to succeed and to say that the result is a cl_device_type"
    );

    result = clGetDeviceInfo(device, CL_DEVICE_TYPE, 500, value, &size_ret);
    fail_if(
        result != CL_SUCCESS || *(cl_device_type*)(value) != CL_DEVICE_TYPE_CPU,
        "we have to say the device is a CPU"
    );

    result = clGetDeviceInfo(device, CL_DEVICE_VENDOR, 500, value, &size_ret);
    fail_if(
        result != CL_SUCCESS,
        "we must succeed"
    );
    fail_if(
        strncmp(value, "Mesa", size_ret) != 0,
        "the device vendor must be \"Mesa\""
    );
}
END_TEST

TCase *cl_device_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("device");
    tcase_add_test(tc, test_get_device_ids);
    tcase_add_test(tc, test_get_device_info);
    return tc;
}
