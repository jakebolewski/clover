#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "commandqueue.h"
#include "config.h"

#include <vector>

namespace Coal
{

class MemObject;
class Kernel;
class DeviceKernel;
class DeviceInterface;

class BufferEvent : public Event
{
    public:
        BufferEvent(CommandQueue *parent,
                    MemObject *buffer,
                    cl_uint num_events_in_wait_list,
                    const Event **event_wait_list,
                    cl_int *errcode_ret);

        MemObject *buffer() const;

        static bool isSubBufferAligned(const MemObject *buffer,
                                       const DeviceInterface *device);

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

class ReadWriteBufferRectEvent : public BufferEvent
{
    public:
        ReadWriteBufferRectEvent(CommandQueue *parent,
                                 MemObject *buffer,
                                 const size_t buffer_origin[3],
                                 const size_t host_origin[3],
                                 const size_t region[3],
                                 size_t buffer_row_pitch,
                                 size_t buffer_slice_pitch,
                                 size_t host_row_pitch,
                                 size_t host_slice_pitch,
                                 void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 const Event **event_wait_list,
                                 cl_int *errcode_ret);

        size_t buffer_origin(unsigned int index) const;
        size_t host_origin(unsigned int index) const;
        size_t region(unsigned int index) const;
        size_t buffer_row_pitch() const;
        size_t buffer_slice_pitch() const;
        size_t host_row_pitch() const;
        size_t host_slice_pitch() const;
        void *ptr() const;

    private:
        size_t p_buffer_origin[3], p_host_origin[3], p_region[3];
        size_t p_buffer_row_pitch, p_buffer_slice_pitch;
        size_t p_host_row_pitch, p_host_slice_pitch;
        void *p_ptr;
};

class ReadBufferRectEvent : public ReadWriteBufferRectEvent
{
    public:
        ReadBufferRectEvent(CommandQueue *parent,
                            MemObject *buffer,
                            const size_t buffer_origin[3],
                            const size_t host_origin[3],
                            const size_t region[3],
                            size_t buffer_row_pitch,
                            size_t buffer_slice_pitch,
                            size_t host_row_pitch,
                            size_t host_slice_pitch,
                            void *ptr,
                            cl_uint num_events_in_wait_list,
                            const Event **event_wait_list,
                            cl_int *errcode_ret);

        Type type() const;
};

class WriteBufferRectEvent : public ReadWriteBufferRectEvent
{
    public:
        WriteBufferRectEvent(CommandQueue *parent,
                             MemObject *buffer,
                             const size_t buffer_origin[3],
                             const size_t host_origin[3],
                             const size_t region[3],
                             size_t buffer_row_pitch,
                             size_t buffer_slice_pitch,
                             size_t host_row_pitch,
                             size_t host_slice_pitch,
                             void *ptr,
                             cl_uint num_events_in_wait_list,
                             const Event **event_wait_list,
                             cl_int *errcode_ret);

        Type type() const;
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
        DeviceKernel *deviceKernel() const;

        virtual Type type() const;

    private:
        cl_uint p_work_dim;
        size_t p_global_work_offset[MAX_WORK_DIMS],
               p_global_work_size[MAX_WORK_DIMS],
               p_local_work_size[MAX_WORK_DIMS],
               p_max_work_item_sizes[MAX_WORK_DIMS];
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
