#ifndef __CPU_KERNEL_H__
#define __CPU_KERNEL_H__

#include "../deviceinterface.h"

namespace Coal
{

class CPUDevice;
class Kernel;

class CPUKernel : public DeviceKernel
{
    public:
        CPUKernel(CPUDevice *device, Kernel *kernel);
        ~CPUKernel();

        size_t workGroupSize() const;
		cl_ulong localMemSize() const;
        cl_ulong privateMemSize() const;
		size_t preferredWorkGroupSizeMultiple() const;

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
};

}

#endif
