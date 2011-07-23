#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "refcounted.h"

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
class DeviceKernel;

class Kernel : public RefCounted
{
    public:
        Kernel(Program *program);
        ~Kernel();

        class Arg
        {
            public:
                enum File
                {
                    Private = 0,
                    Global = 1,
                    Local = 2,
                    Constant = 3
                };
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
                };

                Arg(unsigned short vec_dim, File file, Kind kind);
                ~Arg();

                void alloc();
                void loadData(const void *data);
                void setAllocAtKernelRuntime(size_t size);

                bool operator !=(const Arg &b);

                size_t valueSize() const;
                unsigned short vecDim() const;
                File file() const;
                Kind kind() const;
                bool defined() const;
                size_t allocAtKernelRuntime() const;
                const void *value(unsigned short index) const;

            private:
                unsigned short p_vec_dim;
                File p_file;
                Kind p_kind;
                void *p_data;
                bool p_defined;
                size_t p_runtime_alloc;
        };

        cl_int addFunction(DeviceInterface *device, llvm::Function *function,
                           llvm::Module *module);
        llvm::Function *function(DeviceInterface *device) const;
        cl_int setArg(cl_uint index, size_t size, const void *value);
        unsigned int numArgs() const;
        const Arg &arg(unsigned int index) const;

        Program *program() const;
        DeviceKernel *deviceDependentKernel(DeviceInterface *device) const;

        bool argsSpecified() const;
        bool needsLocalAllocation() const;  /*!< One or more arguments is __local */

        cl_int info(cl_kernel_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;
        cl_int workGroupInfo(DeviceInterface *device,
                             cl_kernel_work_group_info param_name,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret) const;

    private:
        Program *p_program;
        std::string p_name;
        bool p_local_args;

        struct DeviceDependent
        {
            DeviceInterface *device;
            DeviceKernel *kernel;
            llvm::Function *function;
            llvm::Module *module;
        };

        std::vector<DeviceDependent> p_device_dependent;
        std::vector<Arg> p_args;
        DeviceDependent null_dep;

        const DeviceDependent &deviceDependent(DeviceInterface *device) const;
        DeviceDependent &deviceDependent(DeviceInterface *device);
};

}

class _cl_kernel : public Coal::Kernel
{};

#endif
