#include "CL/cl.h"

#include <core/program.h>
#include <core/kernel.h>

// Kernel Object APIs
cl_kernel
clCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret)
{
    cl_int dummy_errcode;

    if (!errcode_ret)
        errcode_ret = &dummy_errcode;

    if (!kernel_name)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return 0;
    }

    if (!program)
    {
        *errcode_ret = CL_INVALID_PROGRAM;
        return 0;
    }

    if (program->state() != Coal::Program::Built)
    {
        *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE;
        return 0;
    }

    Coal::Kernel *kernel = program->createKernel(kernel_name, errcode_ret);

    if (*errcode_ret != CL_SUCCESS)
    {
        delete kernel;
        return 0;
    }

    return (cl_kernel)kernel;
}

cl_int
clCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret)
{
    cl_int rs = CL_SUCCESS;

    if (!program)
        return CL_INVALID_PROGRAM;

    if (program->state() != Coal::Program::Built)
        return CL_INVALID_PROGRAM_EXECUTABLE;

    std::vector<Coal::Kernel *> ks = program->createKernels(&rs);

    if (rs != CL_SUCCESS)
    {
        while (ks.size())
        {
            delete ks.back();
            ks.pop_back();
        }

        return rs;
    }

    // Check that the kernels will fit in the array, if needed
    if (num_kernels_ret)
        *num_kernels_ret = ks.size();

    if (kernels && num_kernels < ks.size())
    {
        while (ks.size())
        {
            delete ks.back();
            ks.pop_back();
        }

        return CL_INVALID_VALUE;
    }

    if (!kernels)
    {
        // We don't need the kernels in fact
        while (ks.size())
        {
            delete ks.back();
            ks.pop_back();
        }
    }
    else
    {
        // Copy the kernels
        for (int i=0; i<ks.size(); ++i)
        {
            kernels[i] = (cl_kernel)ks[i];
        }
    }

    return CL_SUCCESS;
}

cl_int
clRetainKernel(cl_kernel    kernel)
{
    if (!kernel)
        return CL_INVALID_KERNEL;

    kernel->reference();

    return CL_SUCCESS;
}

cl_int
clReleaseKernel(cl_kernel   kernel)
{
    if (!kernel)
        return CL_INVALID_KERNEL;

    if (kernel->dereference())
        delete kernel;

    return CL_SUCCESS;
}

cl_int
clSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_indx,
               size_t       arg_size,
               const void * arg_value)
{
    return 0;
}

cl_int
clGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret)
{
    return 0;
}

cl_int
clGetKernelWorkGroupInfo(cl_kernel                  kernel,
                         cl_device_id               device,
                         cl_kernel_work_group_info  param_name,
                         size_t                     param_value_size,
                         void *                     param_value,
                         size_t *                   param_value_size_ret)
{
    return 0;
}
