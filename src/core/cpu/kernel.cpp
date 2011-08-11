#include "kernel.h"
#include "device.h"
#include "buffer.h"
#include "program.h"
#include "builtins.h"

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
#include <sys/mman.h>

using namespace Coal;

static llvm::Constant *getPointerConstant(llvm::LLVMContext &C,
                                          llvm::Type *type,
                                          void *value)
{
    llvm::Constant *rs = 0;

    if (sizeof(void *) == 4)
        rs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(C), (uint64_t)value);
    else
        rs = llvm::ConstantInt::get(llvm::Type::getInt64Ty(C), (uint64_t)value);

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
        if (divisor > global_work_size || divisor > cpus * 32)
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
    for (unsigned int i=0; i<p_kernel->numArgs(); ++i)
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
                case Kernel::Arg::Sampler:
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
                                               local_buffer);

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

                            buffer->allocate(p_device);

                            buf_ptr = cpubuf->data();

                            C = getPointerConstant(stub->getContext(),
                                                   k_func_type->getParamType(i),
                                                   buf_ptr);
                        }
                    }

                    break;
                }

                case Kernel::Arg::Image2D:
                case Kernel::Arg::Image3D:
                {
                    Image2D *image = *(Image2D **)value;
                    image->allocate(p_device);

                    // Assign a pointer to the image object, the intrinsic functions
                    // will handle them
                    C = getPointerConstant(stub->getContext(),
                                           k_func_type->getParamType(i),
                                           (void *)image);
                    break;
                }

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
: p_device(device), p_event(event), p_current_wg(0), p_finished_wg(0)
{
    // Mutex
    pthread_mutex_init(&p_mutex, 0);

    // Set current work group to (0, 0, ..., 0)
    std::memset(p_current_work_group, 0, event->work_dim() * sizeof(size_t));

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
}

bool CPUKernelEvent::reserve()
{
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
: p_kernel(kernel), p_cpu_event(cpu_event), p_event(event),
  p_work_dim(event->work_dim()), p_contexts(0), p_stack_size(8192 /* TODO */),
  p_had_barrier(false)
{

    // Set index
    std::memcpy(p_index, work_group_index, p_work_dim * sizeof(size_t));

    // Set maxs and global id
    p_num_work_items = 1;

    for (unsigned int i=0; i<p_work_dim; ++i)
    {
        p_max_local_id[i] = event->local_work_size(i) - 1; // 0..n-1, not 1..n
        p_num_work_items *= event->local_work_size(i);

        // Set global id
        p_global_id_start_offset[i] = (p_index[i] * event->local_work_size(i))
                         + event->global_work_offset(i);
    }
}

CPUKernelWorkGroup::~CPUKernelWorkGroup()
{
    p_cpu_event->workGroupFinished();
}

bool CPUKernelWorkGroup::run()
{
    // Get the kernel function to call
    bool free_after = p_kernel->kernel()->needsLocalAllocation();
    std::vector<void *> local_to_free;
    llvm::Function *kernel_func = p_kernel->callFunction(local_to_free);

    if (!kernel_func)
        return false;

    Program *p = (Program *)p_kernel->kernel()->parent();
    CPUProgram *prog = (CPUProgram *)(p->deviceDependentProgram(p_kernel->device()));

    p_kernel_func_addr = (void(*)())prog->jit()->getPointerToFunction(kernel_func);

    // Tell the builtins this thread will run a kernel work group
    setThreadLocalWorkGroup(this);

    // Initialize the dummy context used by the builtins before a call to barrier()
    p_current_work_item = 0;
    p_current_context = &p_dummy_context;

    std::memset(p_dummy_context.local_id, 0, p_work_dim * sizeof(size_t));

    do
    {
        // Simply call the "call function", it and the builtins will do the rest
        p_kernel_func_addr();
    } while (!p_had_barrier &&
             !incVec(p_work_dim, p_dummy_context.local_id, p_max_local_id));

    // If no barrier() call was made, all is fine. If not, only the first
    // work-item has currently finished. We must let the others run.
    if (p_had_barrier)
    {
        Context *main_context = p_current_context; // After the first swapcontext,
                                                   // we will not be able to trust
                                                   // p_current_context anymore.

        // We'll call swapcontext for each remaining work-item. They will
        // finish, and when they'll do so, this main context will be resumed, so
        // it's easy (i starts from 1 because the main context already finished)
        for (unsigned int i=1; i<p_num_work_items; ++i)
        {
            Context *ctx = getContextAddr(i);
            swapcontext(&main_context->context, &ctx->context);
        }
    }

    // We may have some cleanup to do
    if (free_after)
    {
        for (size_t i=0; i<local_to_free.size(); ++i)
        {
            std::free(local_to_free[i]);
        }

        // Bye function
        kernel_func->eraseFromParent();
    }

    return true;
}

CPUKernelWorkGroup::Context *CPUKernelWorkGroup::getContextAddr(unsigned int index)
{
    size_t size;
    char *data = (char *)p_contexts;

    // Each Context in data is an element of size p_stack_size + sizeof(Context)
    size = p_stack_size + sizeof(Context);
    size *= index;  // To get an offset

    return (Context *)(data + size); // Pointer to the context
}
