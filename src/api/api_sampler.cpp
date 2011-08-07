#include "CL/cl.h"

#include "core/sampler.h"
#include "core/context.h"

// Sampler APIs
cl_sampler
clCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret)
{
    cl_int dummy_errcode;

    if (!errcode_ret)
        errcode_ret = &dummy_errcode;

    if (!context->isA(Coal::Object::T_Context))
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return 0;
    }

    *errcode_ret = CL_SUCCESS;

    Coal::Sampler *sampler = new Coal::Sampler((Coal::Context *)context,
                                               normalized_coords,
                                               addressing_mode,
                                               filter_mode,
                                               errcode_ret);

    if (*errcode_ret != CL_SUCCESS)
    {
        delete sampler;
        return 0;
    }

    return (cl_sampler)sampler;
}

cl_int
clRetainSampler(cl_sampler sampler)
{
    if (!sampler->isA(Coal::Object::T_Sampler))
        return CL_INVALID_SAMPLER;

    sampler->reference();

    return CL_SUCCESS;
}

cl_int
clReleaseSampler(cl_sampler sampler)
{
    if (!sampler->isA(Coal::Object::T_Sampler))
        return CL_INVALID_SAMPLER;

    if (sampler->dereference())
        delete sampler;

    return CL_SUCCESS;
}

cl_int
clGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret)
{
    if (!sampler->isA(Coal::Object::T_Sampler))
        return CL_INVALID_SAMPLER;

    return sampler->info(param_name, param_value_size, param_value,
                         param_value_size_ret);
}
