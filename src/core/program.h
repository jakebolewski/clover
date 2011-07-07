#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <CL/cl.h>
#include <string>
#include <vector>

namespace llvm
{
    class MemoryBuffer;
    class Module;
}

namespace Coal
{

class Context;
class Compiler;
class DeviceInterface;

class Program
{
    public:
        Program(Context *ctx);
        ~Program();

        enum Type
        {
            Invalid,
            Source,
            Binary
        };

        enum State
        {
            Empty,  /*!< Just created */
            Loaded, /*!< Source or binary loaded */
            Built,  /*!< Built */
            Failed, /*!< Build failed */
        };

        cl_int loadSources(cl_uint count, const char **strings,
                           const size_t *lengths);
        /** We don't use the devices here, we use LLVM IR */
        cl_int loadBinaries(const unsigned char **data, const size_t *lengths,
                            cl_int *binary_status, cl_uint num_devices,
                            DeviceInterface * const*device_list);
        cl_int build(const char *options,
                     void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                    void *user_data),
                     void *user_data, cl_uint num_devices,
                     DeviceInterface * const*device_list);

        void reference();
        bool dereference();

        Type type() const;
        State state() const;
        Context *context() const;

        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);
        cl_int buildInfo(DeviceInterface *device,
                         cl_context_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret);

    private:
        Context *p_ctx;
        unsigned int p_references;
        Type p_type;
        State p_state;
        std::string p_source;

        struct DeviceDependent
        {
            DeviceInterface *device;
            std::string unlinked_binary;
            llvm::Module *linked_module;
            Compiler *compiler;
        };

        std::vector<DeviceDependent> p_device_dependent;

        void setDevices(cl_uint num_devices, DeviceInterface * const*devices);
        DeviceDependent &deviceDependent(DeviceInterface *device);
};

}

struct _cl_program : public Coal::Program
{};

#endif
