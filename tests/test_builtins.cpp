#include <iostream>
#include <cstdlib>

#include "test_builtins.h"
#include "CL/cl.h"

#include <stdint.h>

const char sampler_source[] =
    "__kernel void test_case(__global uint *rs, sampler_t sampler) {\n"
    "   sampler_t good_sampler = CLK_NORMALIZED_COORDS_TRUE |\n"
    "                            CLK_ADDRESS_MIRRORED_REPEAT |\n"
    "                            CLK_FILTER_NEAREST;\n"
    "\n"
    "   if (sampler != good_sampler) *rs = 1;"
    "}\n";

enum TestCaseKind
{
    NormalKind,
    SamplerKind
};

/*
 * To ease testing, each kernel will be a Task kernel taking a pointer to an
 * integer and running built-in functions. If an error is encountered, the
 * integer pointed to by the arg will be set accordingly. If the kernel succeeds,
 * this integer is set to 0.
 */
static uint32_t run_kernel(const char *source, TestCaseKind kind)
{
    cl_platform_id platform = 0;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_program program;
    cl_int result;
    cl_kernel kernel;
    cl_event event;
    cl_mem rs_buf;

    cl_sampler sampler;

    uint32_t rs = 0;

    result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, 0);
    if (result != CL_SUCCESS) return 65536;

    ctx = clCreateContext(0, 1, &device, 0, 0, &result);
    if (result != CL_SUCCESS) return 65537;

    queue = clCreateCommandQueue(ctx, device, 0, &result);
    if (result != CL_SUCCESS) return 65538;

    program = clCreateProgramWithSource(ctx, 1, &source, 0, &result);
    if (result != CL_SUCCESS) return 65539;

    result = clBuildProgram(program, 1, &device, "", 0, 0);
    if (result != CL_SUCCESS)
    {
        // Print log
        char *log = 0;
        size_t len = 0;

        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, 0, &len);
        log = (char *)std::malloc(len);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, len, log, 0);

        std::cout << log << std::endl;
        std::free(log);

        return 65540;
    }

    kernel = clCreateKernel(program, "test_case", &result);
    if (result != CL_SUCCESS) return 65541;

    // Create the result buffer
    rs_buf = clCreateBuffer(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                            sizeof(rs), &rs, &result);
    if (result != CL_SUCCESS) return 65542;

    result = clSetKernelArg(kernel, 0, sizeof(cl_mem), &rs_buf);
    if (result != CL_SUCCESS) return 65543;

    // Kind
    switch (kind)
    {
        case NormalKind:
            break;

        case SamplerKind:
            sampler = clCreateSampler(ctx, 1, CL_ADDRESS_MIRRORED_REPEAT, CL_FILTER_NEAREST, &result);
            if (result != CL_SUCCESS) return 65546;

            result = clSetKernelArg(kernel, 1, sizeof(cl_sampler), &sampler);
            if (result != CL_SUCCESS) return 65547;
            break;
    }

    result = clEnqueueTask(queue, kernel, 0, 0, &event);
    if (result != CL_SUCCESS) return 65544;

    result = clWaitForEvents(1, &event);
    if (result != CL_SUCCESS) return 65545;

    if (kind == SamplerKind) clReleaseSampler(sampler);
    clReleaseEvent(event);
    clReleaseMemObject(rs_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);

    return rs;
}

static const char *default_error(uint32_t errcode)
{
    switch (errcode)
    {
        case 0:
            return 0;
        case 65536:
            return "Cannot get a device ID";
        case 65537:
            return "Cannot create a context";
        case 65538:
            return "Cannot create a command queue";
        case 65539:
            return "Cannot create a program with given source";
        case 65540:
            return "Cannot build the program";
        case 65541:
            return "Cannot create the test_case kernel";
        case 65542:
            return "Cannot create a buffer holding a uint32_t";
        case 65543:
            return "Cannot set kernel argument";
        case 65544:
            return "Cannot enqueue a task kernel";
        case 65545:
            return "Cannot wait for the event";
        case 65546:
            return "Cannot create a sampler";
        case 65547:
            return "Cannot set a sampler kernel argument";

        default:
            return "Unknown error code";
    }
}

START_TEST (test_sampler)
{
    uint32_t rs = run_kernel(sampler_source, SamplerKind);
    const char *errstr = 0;

    switch (rs)
    {
        case 1:
            errstr = "Sampler bitfield invalid";
            break;
        default:
            errstr = default_error(rs);
    }

    fail_if(
        errstr != 0,
        errstr
    );
}
END_TEST

TCase *cl_builtins_tcase_create(void)
{
    TCase *tc = NULL;
    tc = tcase_create("builtins");
    tcase_add_test(tc, test_sampler);
    return tc;
}
