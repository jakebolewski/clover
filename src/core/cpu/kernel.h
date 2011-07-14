#ifndef __CPU_KERNEL_H__
#define __CPU_KERNEL_H__

#include "../deviceinterface.h"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <vector>

namespace llvm
{
    class Function;
}

namespace Coal
{

class CPUDevice;
class Kernel;

class CPUKernel : public DeviceKernel
{
    public:
        CPUKernel(CPUDevice *device, Kernel *kernel, llvm::Function *function);
        ~CPUKernel();

        size_t workGroupSize() const;
		cl_ulong localMemSize() const;
        cl_ulong privateMemSize() const;
		size_t preferredWorkGroupSizeMultiple() const;
        size_t guessWorkGroupSize(cl_uint num_dims, cl_uint dim,
                                  size_t global_work_size) const;

        llvm::Function *function() const;
        llvm::Function *callFunction();

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
        llvm::Function *p_function, *p_call_function;
};

}

#endif
