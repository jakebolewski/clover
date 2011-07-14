#include "kernel.h"
#include "device.h"
#include "buffer.h"

#include "../kernel.h"
#include "../memobject.h"

#include <llvm/Function.h>
#include <llvm/Constants.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/Support/Casting.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include <cstdlib>
#include <iostream>

using namespace Coal;

CPUKernel::CPUKernel(CPUDevice *device, Kernel *kernel, llvm::Function *function)
: DeviceKernel(), p_device(device), p_kernel(kernel), p_function(function),
  p_call_function(0)
{

}

CPUKernel::~CPUKernel()
{
    if (p_call_function)
        p_call_function->eraseFromParent();
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

static llvm::Constant *getPointerConstant(llvm::LLVMContext &C,
                                          const llvm::Type *type,
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

llvm::Function *CPUKernel::callFunction()
{
    // If we can reuse the same function between work groups, do it
    if (!p_kernel->needsLocalAllocation() && p_call_function)
        return p_call_function;

    // Create a LLVM function that calls the kernels with its arguments
    // Code inspired from llvm/lib/ExecutionEngine/JIT/JIT.cpp
    // Copyright The LLVM Compiler Infrastructure
    const llvm::FunctionType *k_func_type = p_function->getFunctionType();
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
                        // NOTE: Free this after use !
                        void *local_buffer = std::malloc(a.allocAtKernelRuntime());
                        C = getPointerConstant(stub->getContext(),
                                               k_func_type->getParamType(i),
                                               &local_buffer);
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
                    // Assign a pointer to the image object, the instrinsic functions
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
    llvm::CallInst *call_inst = llvm::CallInst::Create(p_function, args.begin(),
                                                       args.end(), "", block);
    call_inst->setCallingConv(p_function->getCallingConv());
    call_inst->setTailCall();

    // Create a return instruction to end the stub
    llvm::ReturnInst::Create(stub->getContext(), block);

    // DEBUG
    stub->getParent()->dump();

    // Retain the function if it can be reused
    if (!p_kernel->needsLocalAllocation())
        p_call_function = stub;

    return stub;
}
