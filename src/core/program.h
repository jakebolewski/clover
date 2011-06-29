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
        
        cl_int loadSources(cl_uint count, const char **strings, 
                           const size_t *lengths);
        /** We don't use the devices here, we use LLVM IR */
        cl_int loadBinary(const unsigned char *data, size_t length,
                          cl_int *binary_status);
        cl_int build(const char *options,
                     void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                    void *user_data),
                     void *user_data);
        
        void reference();
        bool dereference();
        
        Type type() const;
        Context *context() const;
        
    private:
        Context *p_ctx;
        unsigned int p_references;
        Type p_type;
        
        std::string p_source;
        llvm::Module *p_module;
};

}

struct _cl_program : public Coal::Program
{};

#endif