#ifndef __CPU_KERNEL_H__
#define __CPU_KERNEL_H__

#include "../deviceinterface.h"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <vector>
#include <string>

namespace llvm
{
    class Function;
}

namespace Coal
{

class CPUDevice;
class Kernel;
class KernelEvent;

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
        bool lastSlot() const;
        Kernel *kernel() const;
        CPUDevice *device() const;

        llvm::Function *function() const;
        llvm::Function *callFunction(std::vector<void *> &freeLocal);

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
        llvm::Function *p_function, *p_call_function;
};

class CPUKernelWorkGroup
{
    public:
        CPUKernelWorkGroup(CPUKernel *kernel, KernelEvent *event);
        ~CPUKernelWorkGroup();

        bool run();

    private:
        CPUKernel *p_kernel;
        KernelEvent *p_event;
        size_t *p_index, *p_current, *p_maxs;
        size_t p_table_sizes;
};

}

void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current);
void *getBuiltin(const std::string &name);

#endif
