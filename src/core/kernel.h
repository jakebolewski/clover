#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <CL/cl.h>

#include <vector>
#include <string>

namespace llvm
{
    class Function;
    class Module;
}

namespace Coal
{

class Program;
class DeviceInterface;

class Kernel
{
    public:
        Kernel(Program *program);
        ~Kernel();

        void reference();
        bool dereference();

        cl_int addFunction(DeviceInterface *device, llvm::Function *function,
                           llvm::Module *module);
        llvm::Function *function(DeviceInterface *device) const;
        cl_int setArg(cl_uint index, size_t size, const void *value);

        Program *program() const;

        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);

    private:
        Program *p_program;
        unsigned int p_references;
        std::string p_name;

        struct DeviceDependent
        {
            DeviceInterface *device;
            llvm::Function *function;
            llvm::Module *module;
        };

        struct Arg
        {
            unsigned short vec_dim;
            bool set;
            size_t kernel_alloc_size;  /*!< Size of the memory that must be allocated at kernel execution */

            enum File
            {
                Private = 0,
                Global = 1,
                Local = 2,
                Constant = 3
            } file;

            enum Kind
            {
                Invalid,
                Int8,
                Int16,
                Int32,
                Int64,
                Float,
                Double,
                Buffer,
                Image2D,
                Image3D
                // TODO: Sampler
            } kind;

            union
            {
                #define TYPE_VAL(type) type type##_val
                TYPE_VAL(uint8_t);
                TYPE_VAL(uint16_t);
                TYPE_VAL(uint32_t);
                TYPE_VAL(uint64_t);
                TYPE_VAL(cl_float);
                TYPE_VAL(double);
                TYPE_VAL(cl_mem);
                #undef TYPE_VAL
            } value;

            inline bool operator !=(const Arg &b)
            {
                return (kind != b.kind) || (vec_dim != b.vec_dim);
            }
            size_t valueSize() const;
        };

        std::vector<DeviceDependent> p_device_dependent;
        std::vector<Arg> p_args;
        const DeviceDependent &deviceDependent(DeviceInterface *device) const;
        DeviceDependent &deviceDependent(DeviceInterface *device);
};

}

class _cl_kernel : public Coal::Kernel
{};

#endif
