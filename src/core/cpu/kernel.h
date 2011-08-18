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
 *     * Neither the name of the copyright holder nor the
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

#ifndef __CPU_KERNEL_H__
#define __CPU_KERNEL_H__

#include "../deviceinterface.h"
#include <core/config.h>

#include <llvm/ExecutionEngine/GenericValue.h>
#include <vector>
#include <string>

#include <ucontext.h>
#include <pthread.h>

namespace llvm
{
    class Function;
}

namespace Coal
{

class CPUDevice;
class Kernel;
class KernelEvent;
class Image2D;

class CPUKernel : public DeviceKernel
{
    public:
        CPUKernel(CPUDevice *device, Kernel *kernel, llvm::Function *function);
        ~CPUKernel();

        size_t workGroupSize() const;
        cl_ulong localMemSize() const;
        cl_ulong privateMemSize() const;
        size_t preferredWorkGroupSizeMultiple() const;
        size_t guessWorkGroupSize(cl_uint num_dims, cl_uint dim,
                                  size_t global_work_size) const;

        Kernel *kernel() const;
        CPUDevice *device() const;

        llvm::Function *function() const;
        llvm::Function *callFunction();
        static size_t typeOffset(size_t &offset, size_t type_len);

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
        llvm::Function *p_function, *p_call_function;
        pthread_mutex_t p_call_function_mutex;
};

class CPUKernelEvent;

class CPUKernelWorkGroup
{
    public:
        CPUKernelWorkGroup(CPUKernel *kernel, KernelEvent *event,
                           CPUKernelEvent *cpu_event,
                           const size_t *work_group_index);
        ~CPUKernelWorkGroup();

        void *callArgs(std::vector<void *> &locals_to_free);
        bool run();

        // Native functions
        size_t getGlobalId(cl_uint dimindx) const;
        cl_uint getWorkDim() const;
        size_t getGlobalSize(cl_uint dimindx) const;
        size_t getLocalSize(cl_uint dimindx) const;
        size_t getLocalID(cl_uint dimindx) const;
        size_t getNumGroups(cl_uint dimindx) const;
        size_t getGroupID(cl_uint dimindx) const;
        size_t getGlobalOffset(cl_uint dimindx) const;
        void barrier(unsigned int flags);
        void *getImageData(Image2D *image, int x, int y, int z) const;

        void builtinNotFound(const std::string &name) const;

    private:
        CPUKernel *p_kernel;
        CPUKernelEvent *p_cpu_event;
        KernelEvent *p_event;
        cl_uint p_work_dim;
        size_t p_index[MAX_WORK_DIMS],
               p_max_local_id[MAX_WORK_DIMS],
               p_global_id_start_offset[MAX_WORK_DIMS];

        void (*p_kernel_func_addr)(void *);
        void *p_args;

        // Machinery to have barrier() working
        struct Context
        {
            size_t local_id[MAX_WORK_DIMS];
            ucontext_t context;
            unsigned int initialized;
        };

        Context *getContextAddr(unsigned int index);

        Context *p_current_context;
        Context p_dummy_context;
        void *p_contexts;
        size_t p_stack_size;
        unsigned int p_num_work_items, p_current_work_item;
        bool p_had_barrier;
};

class CPUKernelEvent
{
    public:
        CPUKernelEvent(CPUDevice *device, KernelEvent *event);
        ~CPUKernelEvent();

        bool reserve();  /*!< The next Work Group that will execute will be the last. Locks the event */
        bool finished(); /*!< All the work groups have finished */
        CPUKernelWorkGroup *takeInstance(); /*!< Must be called exactly one time after reserve(). Unlocks the event */

        void *kernelArgs() const;
        void cacheKernelArgs(void *args);

        void workGroupFinished();

    private:
        CPUDevice *p_device;
        KernelEvent *p_event;
        size_t p_current_work_group[MAX_WORK_DIMS],
               p_max_work_groups[MAX_WORK_DIMS];
        size_t p_current_wg, p_finished_wg, p_num_wg;
        pthread_mutex_t p_mutex;
        void *p_kernel_args;
};

}

#endif
