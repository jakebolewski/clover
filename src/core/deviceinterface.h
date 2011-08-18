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

#ifndef __DEVICEINTERFACE_H__
#define __DEVICEINTERFACE_H__

#include <CL/cl.h>
#include "object.h"

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

class DeviceInterface : public Object
{
    public:
        DeviceInterface() : Object(Object::T_Device, 0) {}
        virtual ~DeviceInterface() {}

        virtual cl_int info(cl_device_info param_name,
                            size_t param_value_size,
                            void *param_value,
                            size_t *param_value_size_ret) const = 0;

        virtual DeviceBuffer *createDeviceBuffer(MemObject *buffer, cl_int *rs) = 0;
        virtual DeviceProgram *createDeviceProgram(Program *program) = 0;
        virtual DeviceKernel *createDeviceKernel(Kernel *kernel, 
                                                 llvm::Function *function) = 0;

        virtual void pushEvent(Event *event) = 0;

        /** @note must set mapping address of MapBuffer events */
        virtual cl_int initEventDeviceData(Event *event) = 0;
        virtual void freeEventDeviceData(Event *event) = 0;
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
