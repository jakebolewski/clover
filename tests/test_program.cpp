#include "test_program.h"
#include "CL/cl.h"

#include <stdio.h>

const char program_source[] =
    "#define __global __attribute__((address_space(1)))\n"
    "\n"
    "__kernel void test(__global float *a, __global float *b, int n) {\n"
    "   int i;\n"
    "\n"
    "   for (i=0; i<n; i++) {\n"
    "       a[i] = 3.1415926f * b[0] * b[0];\n"
    "   }\n"
    "}\n";

START_TEST (test_create_program)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_program program;
    cl_int result;
    
    const char *src = program_source;
    size_t program_len = sizeof(program_source);
    
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
    
    program = clCreateProgramWithSource(ctx, 0, &src, 0, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "count cannot be 0"
    );
    
    program = clCreateProgramWithSource(ctx, 1, 0, 0, &result);
    fail_if(
        result != CL_INVALID_VALUE,
        "strings cannot be NULL"
    );
    
    program = clCreateProgramWithSource(ctx, 1, &src, &program_len,
                                        &result);
    fail_if(
        result != CL_SUCCESS,
        "cannot create a program from source with sane arguments"
    );
    
    clReleaseProgram(program); // Sorry
    
    program = clCreateProgramWithSource(ctx, 1, &src, 0, &result);
    fail_if(
        result != CL_SUCCESS,
        "lengths can be NULL and it must work"
    );
    
    result = clBuildProgram(program, 1, &device, "", 0, 0);
    printf("result : %i\n", result);
    
    clReleaseContext(ctx);
}
END_TEST

TCase *cl_program_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("program");
    tcase_add_test(tc, test_create_program);
    return tc;
}
