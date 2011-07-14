#ifndef __DEVICEINTERFACE_H__
#define __DEVICEINTERFACE_H__

#include <CL/cl.h>

namespace llvm
{
    class PassManager;
    class Module;
    class Function;
}

namespace Coal
{

class DeviceBuffer;
class DeviceProgram;
class DeviceKernel;

class MemObject;
class Event;
class Program;
class Kernel;

class DeviceInterface
{
    public:
        DeviceInterface() {}
        virtual ~DeviceInterface() {}

        virtual cl_int info(cl_device_info param_name,
                            size_t param_value_size,
                            void *param_value,
                            size_t *param_value_size_ret) = 0;

        virtual DeviceBuffer *createDeviceBuffer(MemObject *buffer, cl_int *rs) = 0;
        virtual DeviceProgram *createDeviceProgram(Program *program) = 0;
        virtual DeviceKernel *createDeviceKernel(Kernel *kernel, 
                                                 llvm::Function *function) = 0;

        virtual void pushEvent(Event *event) = 0;

        /** @note must set mapping address of MapBuffer events */
        virtual cl_int initEventDeviceData(Event *event) = 0;
};

class DeviceBuffer
{
    public:
        DeviceBuffer() {}
        virtual ~DeviceBuffer() {}

        virtual bool allocate() = 0;

        virtual DeviceInterface *device() const = 0;
        virtual bool allocated() const = 0;

        virtual void *nativeGlobalPointer() const = 0;
};

class DeviceProgram
{
    public:
        DeviceProgram() {}
        virtual ~DeviceProgram() {}

        virtual bool linkStdLib() const = 0;
        virtual void createOptimizationPasses(llvm::PassManager *manager,
                                              bool optimize) = 0;
        virtual bool build(llvm::Module *module) = 0;
};

class DeviceKernel
{
	public:
		DeviceKernel() {}
		virtual ~DeviceKernel() {}

		virtual size_t workGroupSize() const = 0;
		virtual cl_ulong localMemSize() const = 0;
        virtual cl_ulong privateMemSize() const = 0;
		virtual size_t preferredWorkGroupSizeMultiple() const = 0;
        virtual size_t guessWorkGroupSize(cl_uint num_dims, cl_uint dim,
                                          size_t global_work_size) const = 0;
};

}

struct _cl_device_id : public Coal::DeviceInterface
{};

#endif
