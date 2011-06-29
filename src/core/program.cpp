#include "program.h"
#include "context.h"
#include "compiler.h"

#include <string>
#include <cstring>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

using namespace Coal;

Program::Program(Context *ctx)
: p_ctx(ctx), p_references(1), p_type(Invalid), p_module(0)
{
    // Retain parent context
    clRetainContext((cl_context)ctx);
}

Program::~Program()
{
    clReleaseContext((cl_context)p_ctx);
}

void Program::reference()
{
    p_references++;
}

bool Program::dereference()
{
    p_references--;
    return (p_references == 0);
}

cl_int Program::loadSources(cl_uint count, const char **strings, 
                            const size_t *lengths)
{
    // Merge all strings into one big one
    for (int i=0; i<count; ++i)
    {
        size_t len = 0;
        const char *data = strings[i];
        
        if (!data)
            return CL_INVALID_VALUE;
        
        // Get the length of the source
        if (lengths && lengths[i])
            len = lengths[i];
        else
            len = std::strlen(data);
        
        // Remove trailing \0's, it's not good for sources (it can arise when
        // the client application wrongly sets lengths
        while (len > 0 && data[len-1] == 0)
            len--;
        
        // Merge the string
        std::string part(data, len);
        p_source += part;
    }
    
    p_type = Source;
    
    return CL_SUCCESS;
}

cl_int Program::loadBinary(const unsigned char *data, size_t length,
                           cl_int *binary_status)
{
    // Sanity checks
    if (!data || !length)
    {
        *binary_status = CL_INVALID_VALUE;
        return CL_INVALID_VALUE;
    }
    
    // Load the data
    const llvm::StringRef s_data((const char *)data, length);
    const llvm::StringRef s_name("<binary>");
    
    llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data, s_name);
    
    if (!buffer)
        return CL_OUT_OF_HOST_MEMORY;
    
    // Parse the bitcode and put it in a Module
    p_module = ParseBitcodeFile(buffer, llvm::getGlobalContext());
    
    if (!p_module)
    {
        *binary_status = CL_INVALID_VALUE;
        return CL_INVALID_BINARY;
    }
    
    p_type = Binary;
    
    *binary_status = CL_SUCCESS;
    return CL_SUCCESS;
}

cl_int Program::build(const char *options,
                      void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                     void *user_data),
                      void *user_data)
{
    // Do we need to compile a source ?
    if (p_type == Source)
    {
        Compiler *compiler = new Compiler(options);
        
        const llvm::StringRef s_data(p_source);
        const llvm::StringRef s_name("<source>");
        
        llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data,
                                                                      s_name);
        
        p_module = compiler->compile(buffer);
        delete compiler;
        
        if (!p_module)
        {
            if (pfn_notify)
                pfn_notify((cl_program)this, user_data);
            
            return CL_BUILD_PROGRAM_FAILURE;
        }
        
        // DEBUG
        p_module->dump();
    }
    
    // TODO: Asynchronous compile
    if (pfn_notify)
        pfn_notify((cl_program)this, user_data);
    
    return CL_SUCCESS;
}

Program::Type Program::type() const
{
    return p_type;
}

Context *Program::context() const
{
    return p_ctx;
}