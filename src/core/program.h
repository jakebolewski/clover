#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <CL/cl.h>
#include <string>

namespace llvm
{
    class MemoryBuffer;
    class Module;
}

namespace Coal
{

class Context;
class Compiler;

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
        cl_int loadBinary(const unsigned char *data, size_t length,
                          cl_int *binary_status, cl_uint num_devices,
                          const cl_device_id *device_list);
        cl_int build(const char *options,
                     void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                    void *user_data),
                     void *user_data, cl_uint num_devices,
                     const cl_device_id *device_list);

        void reference();
        bool dereference();

        Type type() const;
        State state() const;
        Context *context() const;

        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);
        cl_int buildInfo(cl_context_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret);

    private:
        Context *p_ctx;
        Compiler *p_compiler;
        unsigned int p_references;
        Type p_type;
        State p_state;
        cl_uint p_num_devices;
        cl_device_id *p_devices;

        std::string p_source;
        std::string p_unlinked_binary;
        llvm::Module *p_linked_module;
};

}

struct _cl_program : public Coal::Program
{};

#endif
