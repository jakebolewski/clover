/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holderi nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "object.h"

#include <CL/cl.h>
#include <string>
#include <vector>

namespace llvm
{
    class MemoryBuffer;
    class Module;
    class Function;
}

namespace Coal
{

class Context;
class Compiler;
class DeviceInterface;
class DeviceProgram;
class Kernel;

class Program : public Object
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
        cl_int loadBinaries(const unsigned char **data, const size_t *lengths,
                            cl_int *binary_status, cl_uint num_devices,
                            DeviceInterface * const*device_list);
        cl_int build(const char *options,
                     void (CL_CALLBACK *pfn_notify)(cl_program program,
                                                    void *user_data),
                     void *user_data, cl_uint num_devices,
                     DeviceInterface * const*device_list);

        Type type() const;
        State state() const;

        Kernel *createKernel(const std::string &name, cl_int *errcode_ret);
        std::vector<Kernel *> createKernels(cl_int *errcode_ret);
        DeviceProgram *deviceDependentProgram(DeviceInterface *device) const;

        cl_int info(cl_program_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;
        cl_int buildInfo(DeviceInterface *device,
                         cl_program_build_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret) const;

    private:
        Type p_type;
        State p_state;
        std::string p_source;

        struct DeviceDependent
        {
            DeviceInterface *device;
            DeviceProgram *program;
            std::string unlinked_binary;
            llvm::Module *linked_module;
            Compiler *compiler;
        };

        std::vector<DeviceDependent> p_device_dependent;
        DeviceDependent p_null_device_dependent;

        void setDevices(cl_uint num_devices, DeviceInterface * const*devices);
        DeviceDependent &deviceDependent(DeviceInterface *device);
        const DeviceDependent &deviceDependent(DeviceInterface *device) const;
        std::vector<llvm::Function *> kernelFunctions(DeviceDependent &dep);
};

}

struct _cl_program : public Coal::Program
{};

#endif
