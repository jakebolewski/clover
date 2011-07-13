#include "kernel.h"
#include "device.h"

#include "../kernel.h"

using namespace Coal;

CPUKernel::CPUKernel(CPUDevice *device, Kernel *kernel)
: DeviceKernel(), p_device(device), p_kernel(kernel)
{

}

CPUKernel::~CPUKernel()
{

}

size_t CPUKernel::workGroupSize() const
{
    return 0; // TODO
}

cl_ulong CPUKernel::localMemSize() const
{
    return 0; // TODO
}

cl_ulong CPUKernel::privateMemSize() const
{
    return 0; // TODO
}

size_t CPUKernel::preferredWorkGroupSizeMultiple() const
{
    return 0; // TODO
}

template<typename T>
T k_exp(T base, unsigned int e)
{
    T rs = base;

    for (unsigned int i=1; i<e; ++i)
        rs *= base;

    return rs;
}

// Try to find the size a work group has to have to be executed the fastest on
// the CPU.
size_t CPUKernel::guessWorkGroupSize(cl_uint num_dims, cl_uint dim,
                          size_t global_work_size) const
{
    unsigned int cpus = p_device->numCPUs();

    // Don't break in too small parts
    if (k_exp(global_work_size, num_dims) > 64)
        return global_work_size;

    // Find the divisor of global_work_size the closest to cpus but >= than it
    unsigned int divisor = cpus;

    while (true)
    {
        if ((global_work_size % divisor) == 0)
            break;

        // Don't let the loop go up to global_work_size, the overhead would be
        // too huge
        if (divisor > cpus * 32)
        {
            divisor = 1;  // Not parallel but has no CommandQueue overhead
            break;
        }
    }

    // Return the size
    return global_work_size / divisor;
}
