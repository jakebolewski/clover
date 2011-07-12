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
