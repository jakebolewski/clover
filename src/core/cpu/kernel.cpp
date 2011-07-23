#include "kernel.h"
#include "device.h"
#include "buffer.h"
#include "program.h"

#include "../kernel.h"
#include "../memobject.h"
#include "../events.h"
#include "../program.h"

#include <llvm/Function.h>
#include <llvm/Constants.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/Support/Casting.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace Coal;

template<typename T>
bool incVec(cl_ulong dims, T *vec, T *maxs)
{
    bool overflow = false;

    for (cl_ulong i=0; i<dims; ++i)
    {
        vec[i] += 1;

        if (vec[i] > maxs[i])
        {
            vec[i] = 0;
            overflow = true;
        }
        else
        {
            overflow = false;
            break;
        }
    }

    return overflow;
}

static llvm::Constant *getPointerConstant(llvm::LLVMContext &C,
                                          llvm::Type *type,
                                          void *const *value)
{
    llvm::Constant *rs = 0;

    if (sizeof(void *) == 4)
        rs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(C), *(uint32_t *)value);
    else
        rs = llvm::ConstantInt::get(llvm::Type::getInt64Ty(C), *(uint64_t *)value);

    // Cast to kernel's pointer type
    rs = llvm::ConstantExpr::getIntToPtr(rs, type);

    return rs;
}

CPUKernel::CPUKernel(CPUDevice *device, Kernel *kernel, llvm::Function *function)
: DeviceKernel(), p_device(device), p_kernel(kernel), p_function(function),
  p_call_function(0)
{
    pthread_mutex_init(&p_call_function_mutex, 0);
}

CPUKernel::~CPUKernel()
{
    if (p_call_function)
        p_call_function->eraseFromParent();

    pthread_mutex_destroy(&p_call_function_mutex);
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

llvm::Function *CPUKernel::function() const
{
    return p_function;
}

Kernel *CPUKernel::kernel() const
{
    return p_kernel;
}

CPUDevice *CPUKernel::device() const
{
    return p_device;
}

llvm::Function *CPUKernel::callFunction(std::vector<void *> &freeLocal)
{
    pthread_mutex_lock(&p_call_function_mutex);

    // If we can reuse the same function between work groups, do it
    if (!p_kernel->needsLocalAllocation() && p_call_function)
    {
        llvm::Function *rs = p_call_function;
        pthread_mutex_unlock(&p_call_function_mutex);

        return rs;
    }

    // Create a LLVM function that calls the kernels with its arguments
    // Code inspired from llvm/lib/ExecutionEngine/JIT/JIT.cpp
    // Copyright The LLVM Compiler Infrastructure
    llvm::FunctionType *k_func_type = p_function->getFunctionType();
    llvm::FunctionType *f_type =
        llvm::FunctionType::get(p_function->getReturnType(), false);
    llvm::Function *stub = llvm::Function::Create(f_type,
                                                  llvm::Function::InternalLinkage,
                                                "", p_function->getParent());

    // Insert a basic block
    llvm::BasicBlock *block = llvm::BasicBlock::Create(p_function->getContext(),
                                                       "", stub);

    llvm::SmallVector<llvm::Value *, 8> args;

    // Add each kernel arg to args
    for (int i=0; i<p_kernel->numArgs(); ++i)
    {
        const Kernel::Arg &a = p_kernel->arg(i);
        llvm::Constant *arg_constant = 0;

        // To handle vectors (float4, etc)
        llvm::SmallVector<llvm::Constant *, 4> vec_elements;

        // Explore the vector elements
        for (unsigned short k=0; k<a.vecDim(); ++k)
        {
            const void *value = a.value(k);
            llvm::Constant *C = 0;

            switch (a.kind())
            {
                case Kernel::Arg::Int8:
                    C = llvm::ConstantInt::get(stub->getContext(),
                                               llvm::APInt(8, *(uint8_t *)value));
                    break;

                case Kernel::Arg::Int16:
                    C = llvm::ConstantInt::get(stub->getContext(),
                                               llvm::APInt(16, *(uint16_t *)value));
                    break;

                case Kernel::Arg::Int32:
                    C = llvm::ConstantInt::get(stub->getContext(),
                                               llvm::APInt(32, *(uint32_t *)value));
                    break;

                case Kernel::Arg::Int64:
                    C = llvm::ConstantInt::get(stub->getContext(),
                                               llvm::APInt(64, *(uint64_t *)value));
                    break;

                case Kernel::Arg::Float:
                    C = llvm::ConstantFP::get(stub->getContext(),
                                              llvm::APFloat(*(float *)value));
                    break;

                case Kernel::Arg::Double:
                    C = llvm::ConstantFP::get(stub->getContext(),
                                              llvm::APFloat(*(double *)value));
                    break;

                case Kernel::Arg::Buffer:
                {
                    MemObject *buffer = *(MemObject **)value;

                    if (a.file() == Kernel::Arg::Local)
                    {
                        // Alloc a buffer and pass it to the kernel
                        void *local_buffer = std::malloc(a.allocAtKernelRuntime());
                        C = getPointerConstant(stub->getContext(),
                                               k_func_type->getParamType(i),
                                               &local_buffer);

                        freeLocal.push_back(local_buffer);
                    }
                    else
                    {
                        if (!buffer)
                        {
                            // We can do that, just send NULL
                            C = llvm::ConstantPointerNull::get(
                                    llvm::cast<llvm::PointerType>(
                                        k_func_type->getParamType(i)));
                        }
                        else
                        {
                            // Get the CPU buffer, allocate it and get its pointer
                            CPUBuffer *cpubuf =
                                (CPUBuffer *)buffer->deviceBuffer(p_device);
                            void *buf_ptr = 0;

                            if (!cpubuf->allocated())
                                cpubuf->allocate();

                            buf_ptr = cpubuf->data();

                            C = getPointerConstant(stub->getContext(),
                                                   k_func_type->getParamType(i),
                                                   &buf_ptr);
                        }
                    }

                    break;
                }

                case Kernel::Arg::Image2D:
                case Kernel::Arg::Image3D:
                    // Assign a pointer to the image object, the intrinsic functions
                    // will handle them
                    C = getPointerConstant(stub->getContext(),
                                           k_func_type->getParamType(i),
                                           (void **)value);
                    break;

                default:
                    break;
            }

            // Add the vector element
            vec_elements.push_back(C);
        }

        // If the arg was a vector, handle it
        if (a.vecDim() == 1)
        {
            arg_constant = vec_elements.front();
        }
        else
        {
            arg_constant = llvm::ConstantVector::get(vec_elements);
        }

        // Append the arg
        args.push_back(arg_constant);
    }

    // Create the call instruction
    llvm::CallInst *call_inst = llvm::CallInst::Create(p_function, args, "", block);
    call_inst->setCallingConv(p_function->getCallingConv());
    call_inst->setTailCall();

    // Create a return instruction to end the stub
    llvm::ReturnInst::Create(stub->getContext(), block);

    // Retain the function if it can be reused
    if (!p_kernel->needsLocalAllocation())
        p_call_function = stub;

    pthread_mutex_unlock(&p_call_function_mutex);

    return stub;
}

/*
 * CPUKernelEvent
 */
CPUKernelEvent::CPUKernelEvent(CPUDevice *device, KernelEvent *event)
: p_device(device), p_event(event), p_current_work_group(0),
  p_max_work_groups(0), p_current_wg(0), p_finished_wg(0)
{
    // Mutex
    pthread_mutex_init(&p_mutex, 0);

    // Tables
    p_table_sizes = event->work_dim() * sizeof(size_t);

    p_current_work_group = (size_t *)std::malloc(p_table_sizes);
    p_max_work_groups = (size_t *)std::malloc(p_table_sizes);

    // Set current work group to (0, 0, ..., 0)
    std::memset(p_current_work_group, 0, p_table_sizes);

    // Populate p_max_work_groups
    p_num_wg = 1;

    for (cl_uint i=0; i<event->work_dim(); ++i)
    {
        p_max_work_groups[i] =
            (event->global_work_size(i) / event->local_work_size(i)) - 1; // 0..n-1, not 1..n

        p_num_wg *= p_max_work_groups[i] + 1;
    }
}

CPUKernelEvent::~CPUKernelEvent()
{
    pthread_mutex_destroy(&p_mutex);

    std::free(p_current_work_group);
    std::free(p_max_work_groups);
}

bool CPUKernelEvent::reserve()
{
    int rs;

    // Lock, this will be unlocked in takeInstance()
    pthread_mutex_lock(&p_mutex);

    // Last work group if current == max - 1
    return (p_current_wg == p_num_wg - 1);
}

bool CPUKernelEvent::finished()
{
    bool rs;

    pthread_mutex_lock(&p_mutex);

    rs = (p_finished_wg == p_num_wg);

    pthread_mutex_unlock(&p_mutex);

    return rs;
}

void CPUKernelEvent::workGroupFinished()
{
    pthread_mutex_lock(&p_mutex);

    p_finished_wg++;

    pthread_mutex_unlock(&p_mutex);
}

CPUKernelWorkGroup *CPUKernelEvent::takeInstance()
{
    CPUKernelWorkGroup *wg = new CPUKernelWorkGroup((CPUKernel *)p_event->deviceKernel(),
                                                    p_event,
                                                    this,
                                                    p_current_work_group);

    // Increment current work group
    incVec(p_event->work_dim(), p_current_work_group, p_max_work_groups);
    p_current_wg += 1;

    // Release event
    pthread_mutex_unlock(&p_mutex);

    return wg;
}

/*
 * CPUKernelWorkGroup
 */
CPUKernelWorkGroup::CPUKernelWorkGroup(CPUKernel *kernel, KernelEvent *event,
                                       CPUKernelEvent *cpu_event,
                                       const size_t *work_group_index)
: p_kernel(kernel), p_event(event), p_index(0), p_current(0), p_maxs(0),
  p_cpu_event(cpu_event), p_global_id(0)
{
    p_table_sizes = event->work_dim() * sizeof(size_t);

    p_index = (size_t *)std::malloc(p_table_sizes);
    p_current = (size_t *)std::malloc(p_table_sizes);
    p_maxs = (size_t *)std::malloc(p_table_sizes);
    p_global_id = (size_t *)std::malloc(p_table_sizes);

    // Set index
    std::memcpy(p_index, work_group_index, p_table_sizes);

    // Set maxs and global id
    for (unsigned int i=0; i<event->work_dim(); ++i)
    {
        p_maxs[i] = event->local_work_size(i) - 1; // 0..n-1, not 1..n

        // Set global id
        p_global_id[i] = (p_index[i] * p_event->local_work_size(i))
                         + p_event->global_work_offset(i);
    }
}

CPUKernelWorkGroup::~CPUKernelWorkGroup()
{
    std::free(p_index);
    std::free(p_current);
    std::free(p_maxs);

    p_cpu_event->workGroupFinished();
}

bool CPUKernelWorkGroup::run()
{
    // Set current pos to 0
    std::memset(p_current, 0, p_event->work_dim() * sizeof(size_t));

    // Get the kernel function to call
    bool free_after = p_kernel->kernel()->needsLocalAllocation();
    std::vector<void *> local_to_free;
    llvm::Function *kernel_func = p_kernel->callFunction(local_to_free);

    if (!kernel_func)
        return false;

    CPUProgram *prog =
        (CPUProgram *)(p_kernel->kernel()->program()
            ->deviceDependentProgram(p_kernel->device()));

    void (*kernel_func_addr)() = (void(*)())prog->jit()->getPointerToFunction(kernel_func);

    // Tell the builtins this thread will run a kernel
    setThreadLocalWorkGroup(this);

    do
    {
        // Simply call the "call function", it and the builtins will do the rest
        kernel_func_addr();
    } while (!incVec(p_event->work_dim(), p_current, p_maxs));

    // We may have some cleanup to do
    if (free_after)
    {
        for (int i=0; i<local_to_free.size(); ++i)
        {
            std::free(local_to_free[i]);
        }

        // Bye function
        kernel_func->eraseFromParent();
    }

    return true;
}

cl_uint CPUKernelWorkGroup::getWorkDim() const
{
    return p_event->work_dim();
}

size_t CPUKernelWorkGroup::getGlobalId(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 0;

    return p_global_id[dimindx] + p_current[dimindx];
}

size_t CPUKernelWorkGroup::getGlobalSize(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 1;

    return p_event->global_work_size(dimindx);
}

size_t CPUKernelWorkGroup::getLocalSize(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 1;

    return p_event->local_work_size(dimindx);
}

size_t CPUKernelWorkGroup::getLocalID(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 0;

    return p_current[dimindx];
}

size_t CPUKernelWorkGroup::getNumGroups(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 1;

    return (p_event->global_work_size(dimindx) /
            p_event->local_work_size(dimindx));
}

size_t CPUKernelWorkGroup::getGroupID(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 0;

    return p_index[dimindx];
}

size_t CPUKernelWorkGroup::getGlobalOffset(cl_uint dimindx) const
{
    if (dimindx > p_event->work_dim())
        return 0;

    return p_event->global_work_offset(dimindx);
}

void CPUKernelWorkGroup::builtinNotFound(const std::string &name) const
{
    std::cout << "OpenCL: Non-existant builtin function " << name
              << " found in kernel " << p_kernel->function()->getNameStr()
              << '.' << std::endl;
}
