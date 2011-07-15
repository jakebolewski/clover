#ifndef __CPU_KERNEL_H__
#define __CPU_KERNEL_H__

#include "../deviceinterface.h"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <vector>
#include <string>
#include <pthread.h>

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

        Kernel *kernel() const;
        CPUDevice *device() const;

        llvm::Function *function() const;
        llvm::Function *callFunction(std::vector<void *> &freeLocal);

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
        llvm::Function *p_function, *p_call_function;
        pthread_mutex_t p_call_function_mutex;
};

class CPUKernelWorkGroup
{
    public:
        CPUKernelWorkGroup(CPUKernel *kernel, KernelEvent *event,
                           const size_t *work_group_index);
        ~CPUKernelWorkGroup();

        bool run();

        // Native functions
        size_t getGlobalId(cl_uint dimindx) const;

    private:
        CPUKernel *p_kernel;
        KernelEvent *p_event;
        size_t *p_index, *p_current, *p_maxs;
        size_t p_table_sizes;
};

class CPUKernelEvent
{
    public:
        CPUKernelEvent(CPUDevice *device, KernelEvent *event);
        ~CPUKernelEvent();

        bool reserve();  /*!< The next Work Group that will execute will be the last. Locks the event */
        bool lastNoLock() const; /*!< Same as reserve() but without locking. */
        CPUKernelWorkGroup *takeInstance(); /*!< Must be called exactly one time after reserv(). Unlocks the event */
        const size_t *currentWorkGroup() const;

    private:
        CPUDevice *p_device;
        KernelEvent *p_event;
        size_t *p_current_work_group, *p_max_work_groups;
        size_t p_table_sizes;
        pthread_mutex_t p_mutex;
};

}

void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current);
void *getBuiltin(const std::string &name);

#endif
