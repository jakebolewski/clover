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

#include <stdlib.h.embed.h>
#include <stdlib.c.bc.embed.h>

using namespace Coal;

Program::Program(Context *ctx)
: p_ctx(ctx), p_references(1), p_type(Invalid), p_state(Empty)
{
    // Retain parent context
    clRetainContext((cl_context)ctx);
}

Program::~Program()
{
    clReleaseContext((cl_context)p_ctx);

    while (p_device_dependent.size())
    {
        DeviceDependent &dep = p_device_dependent.back();

        delete dep.compiler;
        delete dep.linked_module;

        p_device_dependent.pop_back();
    }
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
    p_source = std::string(embed_stdlib_h);

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

void Program::setDevices(cl_uint num_devices, DeviceInterface * const*devices)
{
    p_device_dependent.resize(num_devices);

    for (int i=0; i<num_devices; ++i)
    {
        DeviceDependent &dep = p_device_dependent.at(i);

        dep.device = devices[i];
        dep.linked_module = 0;
        dep.compiler = new Compiler(dep.device);
    }
}

Program::DeviceDependent &Program::deviceDependent(DeviceInterface *device)
{
    for (int i=0; i<p_device_dependent.size(); ++i)
    {
        DeviceDependent &rs = p_device_dependent.at(i);

        if (rs.device == device)
            return rs;
    }
}

cl_int Program::loadBinaries(const unsigned char **data, const size_t *lengths,
                             cl_int *binary_status, cl_uint num_devices,
                             DeviceInterface * const*device_list)
{
    // Set device infos
    setDevices(num_devices, device_list);

    // Load the data
    for (int i=0; i<num_devices; ++i)
    {
        DeviceDependent &dep = deviceDependent(device_list[i]);

        // Load bitcode
        dep.unlinked_binary = std::string((const char *)data[i], lengths[i]);

        // Make a module of it
        const llvm::StringRef s_data(dep.unlinked_binary);
        const llvm::StringRef s_name("<binary>");

        llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data,
                                                                      s_name,
                                                                      false);

        if (!buffer)
            return CL_OUT_OF_HOST_MEMORY;

        dep.linked_module = ParseBitcodeFile(buffer, llvm::getGlobalContext());

        if (!dep.linked_module)
        {
            binary_status[i] = CL_INVALID_VALUE;
            return CL_INVALID_BINARY;
        }

        binary_status[i] = CL_SUCCESS;
    }

    p_type = Binary;
    p_state = Loaded;

    return CL_SUCCESS;
}

cl_int Program::build(const char *options,
                      void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                     void *user_data),
                      void *user_data, cl_uint num_devices,
                      DeviceInterface * const*device_list)
{
    // Set device infos
    if (!p_device_dependent.size())
    {
        setDevices(num_devices, device_list);
    }

    // Do we need to compile the source for each device ?
    if (p_type == Source)
    {
        for (int i=0; i<num_devices; ++i)
        {
            DeviceDependent &dep = deviceDependent(device_list[i]);

            // Load source
            const llvm::StringRef s_data(p_source);
            const llvm::StringRef s_name("<source>");

            llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(s_data,
                                                                        s_name);

            // Compile
            if (!dep.compiler->compile(options, buffer))
            {
                p_state = Failed;

                if (pfn_notify)
                    pfn_notify((cl_program)this, user_data);

                return CL_BUILD_PROGRAM_FAILURE;
            }

            // Get module and its bitcode
            dep.linked_module = dep.compiler->module();

            llvm::raw_string_ostream ostream(dep.unlinked_binary);

            llvm::WriteBitcodeToFile(dep.linked_module, ostream);
            ostream.flush();
        }
    }

    // Link p_linked_module with the stdlib if the device needs that

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
    llvm::SmallVector<DeviceInterface *, 4> devices;

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
            SIMPLE_ASSIGN(cl_uint, p_device_dependent.size());
            break;

        case CL_PROGRAM_DEVICES:
            for (int i=0; i<p_device_dependent.size(); ++i)
            {
                DeviceDependent &dep = p_device_dependent.at(i);

                devices.push_back(dep.device);
            }

            value = devices.data();
            value_length = devices.size() * sizeof(DeviceInterface *);
            break;

        case CL_PROGRAM_CONTEXT:
            SIMPLE_ASSIGN(cl_context, p_ctx);
            break;

        case CL_PROGRAM_SOURCE:
            MEM_ASSIGN(p_source.size(), p_source.data());
            break;

        case CL_PROGRAM_BINARY_SIZES:
            for (int i=0; i<p_device_dependent.size(); ++i)
            {
                DeviceDependent &dep = p_device_dependent.at(i);

                binary_sizes.push_back(dep.unlinked_binary.size());
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
            value_length = p_device_dependent.size() * sizeof(unsigned char *);

            if (!param_value || param_value_size < value_length)
                return CL_INVALID_VALUE;

            for (int i=0; i<p_device_dependent.size(); ++i)
            {
                DeviceDependent &dep = p_device_dependent.at(i);
                unsigned char *dest = binaries[i];

                if (!dest)
                    continue;

                std::memcpy(dest, dep.unlinked_binary.data(),
                            dep.unlinked_binary.size());
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

cl_int Program::buildInfo(DeviceInterface *device,
                          cl_context_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret)
{
    const void *value = 0;
    size_t value_length = 0;
    DeviceDependent &dep = deviceDependent(device);

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
            value = dep.compiler->options().c_str();
            value_length = dep.compiler->options().size() + 1;
            break;

        case CL_PROGRAM_BUILD_LOG:
            value = dep.compiler->log().c_str();
            value_length = dep.compiler->log().size() + 1;
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
