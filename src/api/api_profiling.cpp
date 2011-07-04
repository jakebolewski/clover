#include "CL/cl.h"
#include <core/commandqueue.h>

// Profiling APIs
cl_int
clGetEventProfilingInfo(cl_event            event,
                        cl_profiling_info   param_name,
                        size_t              param_value_size,
                        void *              param_value,
                        size_t *            param_value_size_ret)
{
    if (!event)
        return CL_INVALID_EVENT;

    return event->profilingInfo(param_name, param_value_size, param_value,
                                param_value_size_ret);
}

