#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "commandqueue.h"

#include <vector>

namespace Coal
{

class MemObject;
class Kernel;
class DeviceKernel;

class BufferEvent : public Event
{
    public:
        BufferEvent(CommandQueue *parent,
                    MemObject *buffer,
                    cl_uint num_events_in_wait_list,
                    const Event **event_wait_list,
                    cl_int *errcode_ret);

        MemObject *buffer() const;

    private:
        MemObject *p_buffer;
};

class ReadWriteBufferEvent : public BufferEvent
{
    public:
        ReadWriteBufferEvent(CommandQueue *parent,
                             MemObject *buffer,
                             size_t offset,
                             size_t cb,
                             void *ptr,
                             cl_uint num_events_in_wait_list,
                             const Event **event_wait_list,
                             cl_int *errcode_ret);

        size_t offset() const;
        size_t cb() const;
        void *ptr() const;

    private:
        size_t p_offset, p_cb;
        void *p_ptr;
};

class ReadBufferEvent : public ReadWriteBufferEvent
{
    public:
        ReadBufferEvent(CommandQueue *parent,
                        MemObject *buffer,
                        size_t offset,
                        size_t cb,
                        void *ptr,
                        cl_uint num_events_in_wait_list,
                        const Event **event_wait_list,
                        cl_int *errcode_ret);

        Type type() const;
};

class WriteBufferEvent : public ReadWriteBufferEvent
{
    public:
        WriteBufferEvent(CommandQueue *parent,
                         MemObject *buffer,
                         size_t offset,
                         size_t cb,
                         void *ptr,
                         cl_uint num_events_in_wait_list,
                         const Event **event_wait_list,
                         cl_int *errcode_ret);

        Type type() const;
};

class MapBufferEvent : public BufferEvent
{
    public:
        MapBufferEvent(CommandQueue *parent,
                       MemObject *buffer,
                       size_t offset,
                       size_t cb,
                       cl_map_flags map_flags,
                       cl_uint num_events_in_wait_list,
                       const Event **event_wait_list,
                       cl_int *errcode_ret);

        Type type() const;

        size_t offset() const;
        size_t cb() const;
        void *ptr() const;
        void setPtr(void *ptr);

    private:
        size_t p_offset, p_cb;
        cl_map_flags p_map_flags;
        void *p_ptr;
};

class UnmapBufferEvent : public BufferEvent
{
    public:
        UnmapBufferEvent(CommandQueue *parent,
                         MemObject *buffer,
                         void *mapped_addr,
                         cl_uint num_events_in_wait_list,
                         const Event **event_wait_list,
                         cl_int *errcode_ret);

        Type type() const;

        void *mapping() const;

    private:
        void *p_mapping;
};

class NativeKernelEvent : public Event
{
    public:
        NativeKernelEvent(CommandQueue *parent,
                          void (*user_func)(void *),
                          void *args,
                          size_t cb_args,
                          cl_uint num_mem_objects,
                          const MemObject **mem_list,
                          const void **args_mem_loc,
                          cl_uint num_events_in_wait_list,
                          const Event **event_wait_list,
                          cl_int *errcode_ret);
        ~NativeKernelEvent();

        Type type() const;

        void *function() const;
        void *args() const;

    private:
        void *p_user_func;
        void *p_args;
};

class KernelEvent : public Event
{
    public:
        KernelEvent(CommandQueue *parent,
                    Kernel *kernel,
                    cl_uint work_dim,
                    const size_t *global_work_offset,
                    const size_t *global_work_size,
                    const size_t *local_work_size,
                    cl_uint num_events_in_wait_list,
                    const Event **event_wait_list,
                    cl_int *errcode_ret);
        ~KernelEvent();

        cl_uint work_dim() const;
        size_t global_work_offset(cl_uint dim) const;
        size_t global_work_size(cl_uint dim) const;
        size_t local_work_size(cl_uint dim) const;
        Kernel *kernel() const;

        virtual Type type() const;

    private:
        cl_uint p_work_dim;
        size_t *p_global_work_offset, *p_global_work_size, *p_local_work_size,
               *p_max_work_item_sizes;
        Kernel *p_kernel;
        DeviceKernel *p_dev_kernel;
};

class TaskEvent : public KernelEvent
{
    public:
        TaskEvent(CommandQueue *parent,
                  Kernel *kernel,
                  cl_uint num_events_in_wait_list,
                  const Event **event_wait_list,
                  cl_int *errcode_ret);

        Type type() const;
};

class UserEvent : public Event
{
    public:
        UserEvent(Context *context, cl_int *errcode_ret);

        Type type() const;
        Context *context() const;
        void addDependentCommandQueue(CommandQueue *queue);  // We need to call pushOnDevice somewhere
        void flushQueues();

    private:
        Context *p_context;
        std::vector<CommandQueue *> p_dependent_queues;
};

}

#endif
