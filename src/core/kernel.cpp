#include "kernel.h"

#include <string>
#include <iostream>
#include <cstring>

#include <llvm/Support/Casting.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>

using namespace Coal;
Kernel::Kernel(Program *program)
: p_program(program), p_references(1)
{
    clRetainProgram((cl_program)program); // TODO: Say a kernel is attached to the program (that becomes unalterable)
}

Kernel::~Kernel()
{
    clReleaseProgram((cl_program)p_program);
}

void Kernel::reference()
{
    p_references++;
}

bool Kernel::dereference()
{
    p_references--;
    return (p_references == 0);
}

const Kernel::DeviceDependent &Kernel::deviceDependent(DeviceInterface *device) const
{
    for (int i=0; i<p_device_dependent.size(); ++i)
    {
        const DeviceDependent &rs = p_device_dependent[i];

        if (rs.device == device)
            return rs;
    }
}

Kernel::DeviceDependent &Kernel::deviceDependent(DeviceInterface *device)
{
    for (int i=0; i<p_device_dependent.size(); ++i)
    {
        DeviceDependent &rs = p_device_dependent[i];

        if (rs.device == device)
            return rs;
    }
}

cl_int Kernel::addFunction(DeviceInterface *device, llvm::Function *function,
                           llvm::Module *module)
{
    // Add a device dependent
    DeviceDependent dep;

    dep.device = device;
    dep.function = function;
    dep.module = module;

    // Build the arg list of the kernel (or verify it if a previous function
    // was already registered)
    const llvm::FunctionType *f = function->getFunctionType();
    bool append = (p_args.size() == 0);

    if (!append && p_args.size() != f->getNumParams())
        return CL_INVALID_KERNEL_DEFINITION;

    for (int i=0; i<f->getNumParams(); ++i)
    {
        const llvm::Type *arg_type = f->getParamType(i);
        Arg a;

        a.kind = Arg::Invalid;
        a.vec_dim = 1;
        a.file = Arg::Private;
        a.kernel_alloc_size = 0;
        a.set = false;

        if (arg_type->isPointerTy())
        {
            // It's a pointer, dereference it
            const llvm::PointerType *p_type = llvm::cast<llvm::PointerType>(arg_type);

            a.file = (Arg::File)p_type->getAddressSpace();
            arg_type = p_type->getElementType();

            // Get the name of the type to see if it's something like image2d, etc
            std::string name = module->getTypeName(arg_type);

            if (name == "image2d")
            {
                // TODO: Address space qualifiers for image types, and read_only
                a.kind = Arg::Image2D;
            }
            else if (name == "image3d")
            {
                a.kind = Arg::Image3D;
            }
            else if (name == "sampler")
            {
                // TODO: Sampler
            }
            else
            {
                a.kind = Arg::Buffer;
            }
        }
        else
        {
            if (arg_type->isVectorTy())
            {
                // It's a vector, we need its element's type
                const llvm::VectorType *v_type = llvm::cast<llvm::VectorType>(arg_type);

                a.vec_dim = v_type->getNumElements();
                arg_type = v_type->getElementType();
            }

            // Get type kind
            if (arg_type->isFloatTy())
            {
                a.kind = Arg::Float;
            }
            else if (arg_type->isDoubleTy())
            {
                a.kind = Arg::Double;
            }
            else if (arg_type->isIntegerTy())
            {
                const llvm::IntegerType *i_type = llvm::cast<llvm::IntegerType>(arg_type);

                if (i_type->getBitWidth() == 8)
                {
                    a.kind = Arg::Int8;
                }
                else if (i_type->getBitWidth() == 16)
                {
                    a.kind = Arg::Int16;
                }
                else if (i_type->getBitWidth() == 32)
                {
                    a.kind = Arg::Int32;
                }
                else if (i_type->getBitWidth() == 64)
                {
                    a.kind = Arg::Int64;
                }
            }
        }

        // Check if we recognized the type
        if (a.kind == Arg::Invalid)
            return CL_INVALID_KERNEL_DEFINITION;

        // If we also have a function registered, check for signature compliance
        if (!append && a != p_args[i])
            return CL_INVALID_KERNEL_DEFINITION;

        // Append arg if needed
        if (append)
            p_args.push_back(a);
    }

    p_device_dependent.push_back(dep);

    return CL_SUCCESS;
}

llvm::Function *Kernel::function(DeviceInterface *device) const
{
    const DeviceDependent &dep = deviceDependent(device);

    return dep.function;
}

size_t Kernel::Arg::valueSize() const
{
    switch (kind)
    {
        case Invalid:
            return 0;
        case Int8:
            return 1;
        case Int16:
            return 2;
        case Int32:
            return 4;
        case Int64:
            return 8;
        case Float:
            return sizeof(cl_float);
        case Double:
            return sizeof(double);
        case Buffer:
        case Image2D:
        case Image3D:
            return sizeof(cl_mem);
    }
}

cl_int Kernel::setArg(cl_uint index, size_t size, const void *value)
{
    if (index > p_args.size())
        return CL_INVALID_ARG_INDEX;

    Arg &arg = p_args[index];

    // Special case for __local pointers
    if (arg.file == Arg::Local)
    {
        if (size == 0)
            return CL_INVALID_ARG_SIZE;

        if (value != 0)
            return CL_INVALID_ARG_VALUE;

        arg.kernel_alloc_size = size;

        return CL_SUCCESS;
    }

    // Check that size corresponds to the arg type
    size_t arg_size = arg.valueSize();

    if (size != arg_size)
        return CL_INVALID_ARG_SIZE;

    // Check for null values
    if (!value)
    {
        switch (arg.kind)
        {
            case Arg::Buffer:
            case Arg::Image2D:
            case Arg::Image3D:
                // Special case buffers : value can be 0 (or point to 0)
                arg.value.cl_mem_val = 0;
                arg.set = true;
                return CL_SUCCESS;

            // TODO samplers
            default:
                return CL_INVALID_ARG_VALUE;
        }
    }

    // Copy the data
    std::memcpy(&arg.value, value, arg_size);

    arg.set = true;

    return CL_SUCCESS;
}

Program *Kernel::program() const
{
    return p_program;
}
