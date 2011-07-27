#include "events.h"
#include "commandqueue.h"
#include "memobject.h"
#include "kernel.h"
#include "deviceinterface.h"

#include <cstdlib>
#include <cstring>

using namespace Coal;

/*
 * Read/Write buffers
 */

BufferEvent::BufferEvent(CommandQueue *parent,
                         MemObject *buffer,
                         cl_uint num_events_in_wait_list,
                         const Event **event_wait_list,
                         cl_int *errcode_ret)
: Event(parent, Queued, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_buffer(buffer)
{
    // Correct buffer
    if (!buffer)
    {
        *errcode_ret = CL_INVALID_MEM_OBJECT;
        return;
    }

    // Buffer's context must match the CommandQueue one
    Context *ctx = 0;
    *errcode_ret = parent->info(CL_QUEUE_CONTEXT, sizeof(Context *), &ctx, 0);

    if (*errcode_ret != CL_SUCCESS) return;

    if (buffer->context() != ctx)
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return;
    }

    // Alignment of SubBuffers
    DeviceInterface *device = 0;
    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *),
                                &device, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    if (!isSubBufferAligned(buffer, device))
    {
        *errcode_ret = CL_MISALIGNED_SUB_BUFFER_OFFSET;
        return;
    }

    // Allocate the buffer for the device
    if (!buffer->allocate(device))
    {
        *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE;
        return;
    }
}

MemObject *BufferEvent::buffer() const
{
    return p_buffer;
}

bool BufferEvent::isSubBufferAligned(const MemObject *buffer,
                                     const DeviceInterface *device)
{
    cl_uint align;
    cl_int rs;

    if (buffer->type() != MemObject::SubBuffer)
        return true;

    rs = device->info(CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(uint),
                      &align, 0);

    if (rs != CL_SUCCESS)
        return false;

    size_t mask = 0;

    for (int i=0; i<align; ++i)
        mask = 1 | (mask << 1);

    if (((SubBuffer *)buffer)->offset() & mask)
        return false;

    return true;
}

ReadWriteBufferEvent::ReadWriteBufferEvent(CommandQueue *parent,
                                           MemObject *buffer,
                                           size_t offset,
                                           size_t cb,
                                           void *ptr,
                                           cl_uint num_events_in_wait_list,
                                           const Event **event_wait_list,
                                           cl_int *errcode_ret)
: BufferEvent(parent, buffer, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_offset(offset), p_cb(cb), p_ptr(ptr)
{
    // Check for out-of-bounds reads
    if (!ptr)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (offset + cb > buffer->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
}

size_t ReadWriteBufferEvent::offset() const
{
    return p_offset;
}

size_t ReadWriteBufferEvent::cb() const
{
    return p_cb;
}

void *ReadWriteBufferEvent::ptr() const
{
    return p_ptr;
}

ReadBufferEvent::ReadBufferEvent(CommandQueue *parent,
                                 MemObject *buffer,
                                 size_t offset,
                                 size_t cb,
                                 void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 const Event **event_wait_list,
                                 cl_int *errcode_ret)
: ReadWriteBufferEvent(parent, buffer, offset, cb, ptr, num_events_in_wait_list,
                       event_wait_list, errcode_ret)
{}

Event::Type ReadBufferEvent::type() const
{
    return Event::ReadBuffer;
}

WriteBufferEvent::WriteBufferEvent(CommandQueue *parent,
                                   MemObject *buffer,
                                   size_t offset,
                                   size_t cb,
                                   void *ptr,
                                   cl_uint num_events_in_wait_list,
                                   const Event **event_wait_list,
                                   cl_int *errcode_ret)
: ReadWriteBufferEvent(parent, buffer, offset, cb, ptr, num_events_in_wait_list,
                       event_wait_list, errcode_ret)
{}

Event::Type WriteBufferEvent::type() const
{
    return Event::WriteBuffer;
}

MapBufferEvent::MapBufferEvent(CommandQueue *parent,
                               MemObject *buffer,
                               size_t offset,
                               size_t cb,
                               cl_map_flags map_flags,
                               cl_uint num_events_in_wait_list,
                               const Event **event_wait_list,
                               cl_int *errcode_ret)
: BufferEvent(parent, buffer, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_offset(offset), p_cb(cb), p_map_flags(map_flags)
{
    // Check flags
    if (map_flags & ~(CL_MAP_READ | CL_MAP_WRITE))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    // Check for out-of-bounds values
    if (offset + cb > buffer->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
}

Event::Type MapBufferEvent::type() const
{
    return Event::MapBuffer;
}

size_t MapBufferEvent::offset() const
{
    return p_offset;
}

size_t MapBufferEvent::cb() const
{
    return p_cb;
}

void *MapBufferEvent::ptr() const
{
    return p_ptr;
}

void MapBufferEvent::setPtr(void *ptr)
{
    p_ptr = ptr;
}

UnmapBufferEvent::UnmapBufferEvent(CommandQueue *parent,
                                   MemObject *buffer,
                                   void *mapped_addr,
                                   cl_uint num_events_in_wait_list,
                                   const Event **event_wait_list,
                                   cl_int *errcode_ret)
: BufferEvent(parent, buffer, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_mapping(mapped_addr)
{
    // TODO: Check that p_mapping is ok (will be done in the drivers)
    if (!mapped_addr)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
}

Event::Type UnmapBufferEvent::type() const
{
    return Event::UnmapMemObject;
}

void *UnmapBufferEvent::mapping() const
{
    return p_mapping;
}

CopyBufferEvent::CopyBufferEvent(CommandQueue *parent,
                                 MemObject *source,
                                 MemObject *destination,
                                 size_t src_offset,
                                 size_t dst_offset,
                                 size_t cb,
                                 cl_uint num_events_in_wait_list,
                                 const Event **event_wait_list,
                                 cl_int *errcode_ret)
: BufferEvent(parent, source, num_events_in_wait_list, event_wait_list,
              errcode_ret), p_destination(destination), p_src_offset(src_offset),
  p_dst_offset(dst_offset), p_cb(cb)
{
    if (!destination)
    {
        *errcode_ret = CL_INVALID_MEM_OBJECT;
        return;
    }

    // Check for out-of-bounds
    if (src_offset + cb > source->size() ||
        dst_offset + cb > destination->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    // Check for overlap
    if (source == destination)
    {
        if ((src_offset < dst_offset && src_offset + cb > dst_offset) ||
            (dst_offset < src_offset && dst_offset + cb > src_offset))
        {
            *errcode_ret = CL_MEM_COPY_OVERLAP;
            return;
        }
    }

    // Check alignement of destination
    DeviceInterface *device = 0;
    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *),
                                &device, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    if (!isSubBufferAligned(destination, device))
    {
        *errcode_ret = CL_MISALIGNED_SUB_BUFFER_OFFSET;
        return;
    }

    // Allocate the buffer for the device
    if (!destination->allocate(device))
    {
        *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE;
        return;
    }
}

MemObject *CopyBufferEvent::source() const
{
    return buffer();
}

MemObject *CopyBufferEvent::destination() const
{
    return p_destination;
}

size_t CopyBufferEvent::src_offset() const
{
    return p_src_offset;
}

size_t CopyBufferEvent::dst_offset() const
{
    return p_dst_offset;
}

size_t CopyBufferEvent::cb() const
{
    return p_cb;
}

Event::Type CopyBufferEvent::type() const
{
    return Event::CopyBuffer;
}

/*
 * Native kernel
 */
NativeKernelEvent::NativeKernelEvent(CommandQueue *parent,
                                     void (*user_func)(void *),
                                     void *args,
                                     size_t cb_args,
                                     cl_uint num_mem_objects,
                                     const MemObject **mem_list,
                                     const void **args_mem_loc,
                                     cl_uint num_events_in_wait_list,
                                     const Event **event_wait_list,
                                     cl_int *errcode_ret)
: Event (parent, Queued, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_user_func((void *)user_func), p_args(0)
{
    // Parameters sanity
    if (!user_func)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (!args && (cb_args || num_mem_objects))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (args && !cb_args)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (num_mem_objects && (!mem_list || !args_mem_loc))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (!num_mem_objects && (mem_list || args_mem_loc))
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    // Check that the device can execute a native kernel
    DeviceInterface *device;
    cl_device_exec_capabilities caps;

    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *),
                                &device, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    *errcode_ret = device->info(CL_DEVICE_EXECUTION_CAPABILITIES,
                                sizeof(cl_device_exec_capabilities), &caps, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    if ((caps & CL_EXEC_NATIVE_KERNEL) == 0)
    {
        *errcode_ret = CL_INVALID_OPERATION;
        return;
    }

    // Copy the arguments in a new list
    if (cb_args)
    {
        p_args = std::malloc(cb_args);

        if (!p_args)
        {
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return;
        }

        std::memcpy((void *)p_args, (void *)args, cb_args);

        // Replace memory objects with global pointers
        for (int i=0; i<num_mem_objects; ++i)
        {
            const MemObject *buffer = mem_list[i];
            const char *loc = (const char *)args_mem_loc[i];

            if (!buffer)
            {
                *errcode_ret = CL_INVALID_MEM_OBJECT;
                return;
            }

            // We need to do relocation : loc is in args, we need it in p_args
            size_t delta = (char *)p_args - (char *)args;
            loc += delta;

            *(void **)loc = buffer->deviceBuffer(device)->nativeGlobalPointer();
        }
    }
}

NativeKernelEvent::~NativeKernelEvent()
{
    if (p_args)
        std::free((void *)p_args);
}

Event::Type NativeKernelEvent::type() const
{
    return Event::NativeKernel;
}

void *NativeKernelEvent::function() const
{
    return p_user_func;
}

void *NativeKernelEvent::args() const
{
    return p_args;
}

/*
 * Kernel event
 */
KernelEvent::KernelEvent(CommandQueue *parent,
                         Kernel *kernel,
                         cl_uint work_dim,
                         const size_t *global_work_offset,
                         const size_t *global_work_size,
                         const size_t *local_work_size,
                         cl_uint num_events_in_wait_list,
                         const Event **event_wait_list,
                         cl_int *errcode_ret)
: Event(parent, Queued, num_events_in_wait_list, event_wait_list, errcode_ret),
  p_kernel(kernel), p_work_dim(work_dim)
{
    *errcode_ret = CL_SUCCESS;

    // Sanity checks
    if (!kernel)
    {
        *errcode_ret = CL_INVALID_KERNEL;
        return;
    }

    // Check that the kernel was built for parent's device.
    DeviceInterface *device;
    Context *k_ctx, *q_ctx;
    size_t max_work_group_size;
    cl_uint max_dims = 0;

    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *),
                                &device, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    *errcode_ret = parent->info(CL_QUEUE_CONTEXT, sizeof(Context *), &q_ctx, 0);
    *errcode_ret |= kernel->info(CL_KERNEL_CONTEXT, sizeof(Context *), &k_ctx, 0);
    *errcode_ret |= device->info(CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                                &max_work_group_size, 0);
    *errcode_ret |= device->info(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t),
                                &max_dims, 0);
    *errcode_ret |= device->info(CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                max_dims * sizeof(size_t), p_max_work_item_sizes, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    p_dev_kernel = kernel->deviceDependentKernel(device);

    if (!p_dev_kernel)
    {
        *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE;
        return;
    }

    // Check that contexts match
    if (k_ctx != q_ctx)
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return;
    }

    // Check args
    if (!kernel->argsSpecified())
    {
        *errcode_ret = CL_INVALID_KERNEL_ARGS;
        return;
    }

    // Check dimension
    if (work_dim == 0 || work_dim > max_dims)
    {
        *errcode_ret = CL_INVALID_WORK_DIMENSION;
        return;
    }

    // Populate work_offset, work_size and local_work_size
    size_t work_group_size = 1;

    for (int i=0; i<work_dim; ++i)
    {
        if (global_work_offset)
        {
            p_global_work_offset[i] = global_work_offset[i];
        }
        else
        {
            p_global_work_offset[i] = 0;
        }

        if (!global_work_size || !global_work_size[i])
        {
            *errcode_ret = CL_INVALID_GLOBAL_WORK_SIZE;
        }
        p_global_work_size[i] = global_work_size[i];

        if (!local_work_size)
        {
            // Guess the best value according to the device
            p_local_work_size[i] =
                p_dev_kernel->guessWorkGroupSize(work_dim, i, global_work_size[i]);

            // TODO: CL_INVALID_WORK_GROUP_SIZE if
            // __attribute__((reqd_work_group_size(X, Y, Z))) is set
        }
        else
        {
            // Check divisibility
            if ((global_work_size[i] % local_work_size[i]) != 0)
            {
                *errcode_ret = CL_INVALID_WORK_GROUP_SIZE;
                return;
            }

            // Not too big ?
            if (local_work_size[i] > p_max_work_item_sizes[i])
            {
                *errcode_ret = CL_INVALID_WORK_ITEM_SIZE;
                return;
            }

            // TODO: CL_INVALID_WORK_GROUP_SIZE if
            // __attribute__((reqd_work_group_size(X, Y, Z))) doesn't match

            p_local_work_size[i] = local_work_size[i];
            work_group_size *= local_work_size[i];
        }
    }

    // Check we don't ask too much to the device
    if (work_group_size > max_work_group_size)
    {
        *errcode_ret = CL_INVALID_WORK_GROUP_SIZE;
        return;
    }

    // Check arguments (buffer alignment, image size, ...)
    for (int i=0; i<kernel->numArgs(); ++i)
    {
        const Kernel::Arg &a = kernel->arg(i);

        if (a.kind() == Kernel::Arg::Buffer)
        {
            const MemObject *buffer = *(const MemObject **)(a.value(0));

            if (!BufferEvent::isSubBufferAligned(buffer, device))
            {
                *errcode_ret = CL_MISALIGNED_SUB_BUFFER_OFFSET;
                return;
            }
        }
        else if (a.kind() == Kernel::Arg::Image2D)
        {
            const Image2D *image = *(const Image2D **)(a.value(0));
            size_t maxWidth, maxHeight;

            *errcode_ret = device->info(CL_DEVICE_IMAGE2D_MAX_WIDTH,
                                        sizeof(size_t), &maxWidth, 0);
            *errcode_ret |= device->info(CL_DEVICE_IMAGE2D_MAX_HEIGHT,
                                         sizeof(size_t), &maxHeight, 0);

            if (*errcode_ret != CL_SUCCESS)
                return;

            if (image->width() > maxWidth || image->height() > maxHeight)
            {
                *errcode_ret = CL_INVALID_IMAGE_SIZE;
                return;
            }
        }
        else if (a.kind() == Kernel::Arg::Image3D)
        {
            const Image3D *image = *(const Image3D **)a.value(0);
            size_t maxWidth, maxHeight, maxDepth;

            *errcode_ret = device->info(CL_DEVICE_IMAGE3D_MAX_WIDTH,
                                        sizeof(size_t), &maxWidth, 0);
            *errcode_ret |= device->info(CL_DEVICE_IMAGE3D_MAX_HEIGHT,
                                         sizeof(size_t), &maxHeight, 0);
            *errcode_ret |= device->info(CL_DEVICE_IMAGE3D_MAX_DEPTH,
                                         sizeof(size_t), &maxDepth, 0);

            if (*errcode_ret != CL_SUCCESS)
                return;

            if (image->width() > maxWidth || image->height() > maxHeight ||
                image->depth() > maxDepth)
            {
                *errcode_ret = CL_INVALID_IMAGE_SIZE;
                return;
            }
        }
    }
}

KernelEvent::~KernelEvent()
{

}

cl_uint KernelEvent::work_dim() const
{
    return p_work_dim;
}

size_t KernelEvent::global_work_offset(cl_uint dim) const
{
    return p_global_work_offset[dim];
}

size_t KernelEvent::global_work_size(cl_uint dim) const
{
    return p_global_work_size[dim];
}

size_t KernelEvent::local_work_size(cl_uint dim) const
{
    return p_local_work_size[dim];
}

Kernel *KernelEvent::kernel() const
{
    return p_kernel;
}

DeviceKernel *KernelEvent::deviceKernel() const
{
    return p_dev_kernel;
}

Event::Type KernelEvent::type() const
{
    return Event::NDRangeKernel;
}

static size_t one = 1;

TaskEvent::TaskEvent(CommandQueue *parent,
                     Kernel *kernel,
                     cl_uint num_events_in_wait_list,
                     const Event **event_wait_list,
                     cl_int *errcode_ret)
: KernelEvent(parent, kernel, 1, 0, &one, &one, num_events_in_wait_list,
              event_wait_list, errcode_ret)
{
    // TODO: CL_INVALID_WORK_GROUP_SIZE if
    // __attribute__((reqd_work_group_size(X, Y, Z))) != (1, 1, 1)
}

Event::Type TaskEvent::type() const
{
    return Event::TaskKernel;
}

/*
 * User event
 */
UserEvent::UserEvent(Context *context, cl_int *errcode_ret)
: Event(0, Submitted, 0, 0, errcode_ret), p_context(context)
{}

Event::Type UserEvent::type() const
{
    return Event::User;
}

Context *UserEvent::context() const
{
    return p_context;
}

void UserEvent::addDependentCommandQueue(CommandQueue *queue)
{
    std::vector<CommandQueue *>::const_iterator it;

    for (it = p_dependent_queues.begin(); it != p_dependent_queues.end(); ++it)
        if (*it == queue)
            return;

    p_dependent_queues.push_back(queue);
}

void UserEvent::flushQueues()
{
    std::vector<CommandQueue *>::const_iterator it;

    for (it = p_dependent_queues.begin(); it != p_dependent_queues.end(); ++it)
        (*it)->pushEventsOnDevice();
}

/*
 * ReadWriteBufferRectEvent
 */
ReadWriteCopyBufferRectEvent::ReadWriteCopyBufferRectEvent(CommandQueue *parent,
                                                           MemObject *source,
                                                           const size_t src_origin[3],
                                                           const size_t dst_origin[3],
                                                           const size_t region[3],
                                                           size_t src_row_pitch,
                                                           size_t src_slice_pitch,
                                                           size_t dst_row_pitch,
                                                           size_t dst_slice_pitch,
                                                           cl_uint num_events_in_wait_list,
                                                           const Event **event_wait_list,
                                                           cl_int *errcode_ret)
: BufferEvent (parent, source, num_events_in_wait_list, event_wait_list,
               errcode_ret)
{
    // Copy the vectors
    std::memcpy(&p_src_origin, src_origin, 3 * sizeof(size_t));
    std::memcpy(&p_dst_origin, dst_origin, 3 * sizeof(size_t));

    for (unsigned int i=0; i<3; ++i)
    {
        if (!region[i])
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }

        p_region[i] = region[i];
    }
    // Compute the pitches
    p_src_row_pitch = region[0];

    if (src_row_pitch)
    {
        if (src_row_pitch < p_src_row_pitch)
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }

        p_src_row_pitch = src_row_pitch;
    }

    p_src_slice_pitch = region[1] * p_src_row_pitch;

    if (src_slice_pitch)
    {
        if (src_slice_pitch < p_src_slice_pitch)
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }

        p_src_slice_pitch = src_slice_pitch;
    }

    p_dst_row_pitch = region[0];

    if (dst_row_pitch)
    {
        if (dst_row_pitch < p_dst_row_pitch)
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }

        p_dst_row_pitch = dst_row_pitch;
    }

    p_dst_slice_pitch = region[1] * p_dst_row_pitch;

    if (dst_slice_pitch)
    {
        if (dst_slice_pitch < p_dst_slice_pitch)
        {
            *errcode_ret = CL_INVALID_VALUE;
            return;
        }

        p_dst_slice_pitch = dst_slice_pitch;
    }
}

size_t ReadWriteCopyBufferRectEvent::src_origin(unsigned int index) const
{
    return p_src_origin[index];
}

size_t ReadWriteCopyBufferRectEvent::dst_origin(unsigned int index) const
{
    return p_dst_origin[index];
}

size_t ReadWriteCopyBufferRectEvent::region(unsigned int index) const
{
    return p_region[index];
}

size_t ReadWriteCopyBufferRectEvent::src_row_pitch() const
{
    return p_src_row_pitch;
}

size_t ReadWriteCopyBufferRectEvent::src_slice_pitch() const
{
    return p_src_slice_pitch;
}

size_t ReadWriteCopyBufferRectEvent::dst_row_pitch() const
{
    return p_dst_row_pitch;
}

size_t ReadWriteCopyBufferRectEvent::dst_slice_pitch() const
{
    return p_dst_slice_pitch;
}

MemObject *ReadWriteCopyBufferRectEvent::source() const
{
    return buffer();
}

CopyBufferRectEvent::CopyBufferRectEvent(CommandQueue *parent,
                                         MemObject *source,
                                         MemObject *destination,
                                         const size_t src_origin[3],
                                         const size_t dst_origin[3],
                                         const size_t region[3],
                                         size_t src_row_pitch,
                                         size_t src_slice_pitch,
                                         size_t dst_row_pitch,
                                         size_t dst_slice_pitch,
                                         cl_uint num_events_in_wait_list,
                                         const Event **event_wait_list,
                                         cl_int *errcode_ret)
: ReadWriteCopyBufferRectEvent(parent, source, src_origin, dst_origin, region,
                               src_row_pitch, src_slice_pitch, dst_row_pitch,
                               dst_slice_pitch, num_events_in_wait_list,
                               event_wait_list, errcode_ret),
  p_destination(destination)
{
    if (!destination)
    {
        *errcode_ret = CL_INVALID_MEM_OBJECT;
        return;
    }

    // Check for out-of-bounds
    if (src_origin[0] + region[0] > this->src_row_pitch() ||
        (src_origin[1] + region[1]) * this->src_row_pitch() > this->src_slice_pitch() ||
        (src_origin[2] + region[2]) * this->src_slice_pitch() > source->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    if (dst_origin[0] + region[0] > this->dst_row_pitch() ||
        (dst_origin[1] + region[1]) * this->dst_row_pitch() > this->dst_slice_pitch() ||
        (dst_origin[2] + region[2]) * this->dst_slice_pitch() > destination->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    // Check alignment of destination (source already checked by BufferEvent)
    DeviceInterface *device = 0;
    *errcode_ret = parent->info(CL_QUEUE_DEVICE, sizeof(DeviceInterface *),
                                &device, 0);

    if (*errcode_ret != CL_SUCCESS)
        return;

    if (!isSubBufferAligned(destination, device))
    {
        *errcode_ret = CL_MISALIGNED_SUB_BUFFER_OFFSET;
        return;
    }

    // Allocate the buffer for the device
    if (!destination->allocate(device))
    {
        *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE;
        return;
    }
}

Event::Type CopyBufferRectEvent::type() const
{
    return Event::CopyBufferRect;
}

MemObject *CopyBufferRectEvent::destination() const
{
    return p_destination;
}

ReadWriteBufferRectEvent::ReadWriteBufferRectEvent(CommandQueue *parent,
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
                                                   cl_int *errcode_ret)
: ReadWriteCopyBufferRectEvent(parent, buffer, buffer_origin, host_origin, region,
                               buffer_row_pitch, buffer_slice_pitch,
                               host_row_pitch, host_slice_pitch,
                               num_events_in_wait_list, event_wait_list, errcode_ret),
  p_ptr(ptr)
{
    if (!ptr)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }

    // Check for out-of-bounds
    if (buffer_origin[0] + region[0] > src_row_pitch() ||
        (buffer_origin[1] + region[1]) * src_row_pitch() > src_slice_pitch() ||
        (buffer_origin[2] + region[2]) * src_slice_pitch() > buffer->size())
    {
        *errcode_ret = CL_INVALID_VALUE;
        return;
    }
}

void *ReadWriteBufferRectEvent::ptr() const
{
    return p_ptr;
}

ReadBufferRectEvent::ReadBufferRectEvent (CommandQueue *parent,
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
                                          cl_int *errcode_ret)
: ReadWriteBufferRectEvent(parent, buffer, buffer_origin, host_origin, region,
                           buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
                           host_slice_pitch, ptr, num_events_in_wait_list,
                           event_wait_list, errcode_ret)
{
}

Event::Type ReadBufferRectEvent::type() const
{
    return ReadBufferRect;
}

WriteBufferRectEvent::WriteBufferRectEvent (CommandQueue *parent,
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
                                            cl_int *errcode_ret)
: ReadWriteBufferRectEvent (parent, buffer, buffer_origin, host_origin, region,
                            buffer_row_pitch, buffer_slice_pitch, host_row_pitch,
                            host_slice_pitch, ptr, num_events_in_wait_list,
                            event_wait_list, errcode_ret)
{
}

Event::Type WriteBufferRectEvent::type() const
{
    return WriteBufferRect;
}

