#include "program.h"
#include "context.h"
#include "compiler.h"
#include "propertylist.h"

#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

using namespace Coal;

Program::Program(Context *ctx)
: p_ctx(ctx), p_references(1), p_type(Invalid), p_linked_module(0), p_num_devices(0),
  p_devices(0), p_state(Empty)
{
    // Retain parent context
    clRetainContext((cl_context)ctx);
}

Program::~Program()
{
    clReleaseContext((cl_context)p_ctx);

    if (p_devices)
        std::free((void *)p_devices);
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
    p_state = Loaded;

    return CL_SUCCESS;
}

cl_int Program::loadBinary(const unsigned char *data, size_t length,
                           cl_int *binary_status, cl_uint num_devices,
                          const cl_device_id *device_list)
{
    // Sanity checks
    if (!data || !length)
    {
        *binary_status = CL_INVALID_VALUE;
        return CL_INVALID_VALUE;
    }

    // Set device infos
    p_num_devices = num_devices;
    p_devices = (cl_device_id *) std::malloc(num_devices * sizeof(cl_device_id));

    if (!p_devices)
        return CL_OUT_OF_HOST_MEMORY;

    std::memcpy(p_devices, device_list, num_devices * sizeof(cl_device_id));

    // Load the data
    p_unlinked_binary = std::string((const char *)data, length);

    const llvm::StringRef s_data((const char *)data, length);
    const llvm::StringRef s_name("<binary>");

    llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data, s_name,
                                                                  false);

    if (!buffer)
        return CL_OUT_OF_HOST_MEMORY;

    // Parse the bitcode and put it in a Module
    p_linked_module = ParseBitcodeFile(buffer, llvm::getGlobalContext());

    if (!p_linked_module)
    {
        *binary_status = CL_INVALID_VALUE;
        return CL_INVALID_BINARY;
    }

    p_type = Binary;
    p_state = Loaded;

    *binary_status = CL_SUCCESS;
    return CL_SUCCESS;
}

cl_int Program::build(const char *options,
                      void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                     void *user_data),
                      void *user_data, cl_uint num_devices,
                      const cl_device_id *device_list)
{
    p_options = std::string(options);

    // Set device infos
    if (!p_num_devices)
    {
        p_num_devices = num_devices;
        p_devices = (cl_device_id *) std::malloc(num_devices * sizeof(cl_device_id));

        if (!p_devices)
            return CL_OUT_OF_HOST_MEMORY;

        std::memcpy(p_devices, device_list, num_devices * sizeof(cl_device_id));
    }

    // Do we need to compile a source ?
    if (p_type == Source)
    {
        Compiler *compiler = new Compiler(p_options);

        if (!compiler->valid())
        {
            if (pfn_notify)
                pfn_notify((cl_program)this, user_data);

            p_state = Failed;
            return CL_BUILD_PROGRAM_FAILURE;
        }

        const llvm::StringRef s_data(p_source);
        const llvm::StringRef s_name("<source>");

        llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data,
                                                                      s_name);

        p_linked_module = compiler->compile(buffer);
        p_log = compiler->log();
        delete compiler;

        if (!p_linked_module)
        {
            if (pfn_notify)
                pfn_notify((cl_program)this, user_data);

            p_state = Failed;
            return CL_BUILD_PROGRAM_FAILURE;
        }

        // Save module binary in p_unlinked_binary
        llvm::raw_string_ostream ostream(p_unlinked_binary);

        llvm::WriteBitcodeToFile(p_linked_module, ostream);
        ostream.flush();
    }

    // TODO: Asynchronous compile
    if (pfn_notify)
        pfn_notify((cl_program)this, user_data);

    p_state = Built;

    return CL_SUCCESS;
}

Program::Type Program::type() const
{
    return p_type;
}

Program::State Program::state() const
{
    return p_state;
}

Context *Program::context() const
{
    return p_ctx;
}

cl_int Program::info(cl_context_info param_name,
                     size_t param_value_size,
                     void *param_value,
                     size_t *param_value_size_ret)
{
    void *value = 0;
    size_t value_length = 0;
    llvm::SmallVector<size_t, 4> binary_sizes;

    union {
        cl_uint cl_uint_var;
        cl_context cl_context_var;
    };

    switch (param_name)
    {
        case CL_PROGRAM_REFERENCE_COUNT:
            SIMPLE_ASSIGN(cl_uint, p_references);
            break;

        case CL_PROGRAM_NUM_DEVICES:
            SIMPLE_ASSIGN(cl_uint, p_num_devices);
            break;

        case CL_PROGRAM_DEVICES:
            MEM_ASSIGN(p_num_devices * sizeof(cl_device_id), p_devices);
            break;

        case CL_PROGRAM_CONTEXT:
            SIMPLE_ASSIGN(cl_context, p_ctx);
            break;

        case CL_PROGRAM_SOURCE:
            MEM_ASSIGN(p_source.size(), p_source.data());
            break;

        case CL_PROGRAM_BINARY_SIZES:
            for (int i=0; i<p_num_devices; ++i)
            {
                binary_sizes.push_back(p_unlinked_binary.size());
            }

            value = binary_sizes.data();
            value_length = binary_sizes.size() * sizeof(size_t);
            break;

        case CL_PROGRAM_BINARIES:
        {
            // Special case : param_value points to an array of p_num_devices
            // application-allocated unsigned char* pointers. Check it's good
            // and std::memcpy the data

            unsigned char **binaries = (unsigned char **)param_value;
            value_length = p_num_devices * sizeof(unsigned char *);

            if (!param_value || param_value_size < value_length)
                return CL_INVALID_VALUE;

            for (int i=0; i<p_num_devices; ++i)
            {
                unsigned char *dest = binaries[i];

                if (!dest)
                    continue;

                std::memcpy(dest, p_unlinked_binary.data(),
                            p_unlinked_binary.size());
            }

            if (param_value_size_ret)
                *param_value_size_ret = value_length;

            return CL_SUCCESS;
        }

        default:
            return CL_INVALID_VALUE;
    }

    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;

    if (param_value_size_ret)
        *param_value_size_ret = value_length;

    if (param_value)
        std::memcpy(param_value, value, value_length);

    return CL_SUCCESS;
}

cl_int Program::buildInfo(cl_context_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret)
{
    const void *value = 0;
    size_t value_length = 0;

    union {
        cl_build_status cl_build_status_var;
    };

    switch (param_name)
    {
        case CL_PROGRAM_BUILD_STATUS:
            switch (p_state)
            {
                case Empty:
                case Loaded:
                    SIMPLE_ASSIGN(cl_build_status, CL_BUILD_NONE);
                    break;
                case Built:
                    SIMPLE_ASSIGN(cl_build_status, CL_BUILD_SUCCESS);
                    break;
                case Failed:
                    SIMPLE_ASSIGN(cl_build_status, CL_BUILD_ERROR);
                    break;
                // TODO: CL_BUILD_IN_PROGRESS
            }
            break;

        case CL_PROGRAM_BUILD_OPTIONS:
            value = p_options.c_str();
            value_length = p_options.size() + 1;
            break;

        case CL_PROGRAM_BUILD_LOG:
            value = p_log.c_str();
            value_length = p_log.size() + 1;
            break;

        default:
            return CL_INVALID_VALUE;
    }

    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;

    if (param_value_size_ret)
        *param_value_size_ret = value_length;

    if (param_value)
        std::memcpy(param_value, value, value_length);

    return CL_SUCCESS;
}
