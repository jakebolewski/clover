#include <CL/cl.h>

#include <core/events.h>

// Enqueued Commands APIs
cl_int
clEnqueueReadBuffer(cl_command_queue    command_queue,
                    cl_mem              buffer,
                    cl_bool             blocking_read,
                    size_t              offset,
                    size_t              cb,
                    void *              ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event)
{
    cl_int rs = CL_SUCCESS;

    if (!command_queue)
        return CL_INVALID_COMMAND_QUEUE;

    Coal::ReadBufferEvent *command = new Coal::ReadBufferEvent(
        (Coal::CommandQueue *)command_queue,
        (Coal::MemObject *)buffer,
        offset, cb, ptr,
        num_events_in_wait_list, (const Coal::Event **)event_wait_list, &rs
    );

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    rs = command_queue->queueEvent(command);

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    if (event)
    {
        // TODO: Ok to reference ?
        *event = (cl_event)command;
        command->reference();
    }

    if (blocking_read)
        return clWaitForEvents(1, (cl_event *)&command);

    return CL_SUCCESS;
}

cl_int
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             cb,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event)
{
    cl_int rs = CL_SUCCESS;

    if (!command_queue)
        return CL_INVALID_COMMAND_QUEUE;

    Coal::WriteBufferEvent *command = new Coal::WriteBufferEvent(
        (Coal::CommandQueue *)command_queue,
        (Coal::MemObject *)buffer,
        offset, cb, (void *)ptr,
        num_events_in_wait_list, (const Coal::Event **)event_wait_list, &rs
    );

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    rs = command_queue->queueEvent(command);

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    if (event)
    {
        *event = (cl_event)command;
        command->reference();
    }

    if (blocking_write)
        return clWaitForEvents(1, (cl_event *)&command);

    return CL_SUCCESS;
}

cl_int
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              cb,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event)
{
    return 0;
}

cl_int
clEnqueueReadImage(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read,
                   const size_t *       origin,
                   const size_t *       region,
                   size_t               row_pitch,
                   size_t               slice_pitch,
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event)
{
    return 0;
}

cl_int
clEnqueueWriteImage(cl_command_queue    command_queue,
                    cl_mem              image,
                    cl_bool             blocking_write,
                    const size_t *      origin,
                    const size_t *      region,
                    size_t              row_pitch,
                    size_t              slice_pitch,
                    const void *        ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event)
{
    return 0;
}

cl_int
clEnqueueCopyImage(cl_command_queue     command_queue,
                   cl_mem               src_image,
                   cl_mem               dst_image,
                   const size_t *       src_origin,
                   const size_t *       dst_origin,
                   const size_t *       region,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event)
{
    return 0;
}

cl_int
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem           src_image,
                           cl_mem           dst_buffer,
                           const size_t *   src_origin,
                           const size_t *   region,
                           size_t           dst_offset,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event)
{
    return 0;
}

cl_int
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem           src_buffer,
                           cl_mem           dst_image,
                           size_t           src_offset,
                           const size_t *   dst_origin,
                           const size_t *   region,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event)
{
    return 0;
}

void *
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           cb,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret)
{
    cl_int rs;

    if (!errcode_ret)
        errcode_ret = &rs;

    *errcode_ret = CL_SUCCESS;

    if (!command_queue)
    {
        *errcode_ret = CL_INVALID_COMMAND_QUEUE;
        return 0;
    }

    Coal::MapBufferEvent *command = new Coal::MapBufferEvent(
        (Coal::CommandQueue *)command_queue,
        (Coal::MemObject *)buffer,
        offset, cb, map_flags,
        num_events_in_wait_list, (const Coal::Event **)event_wait_list, errcode_ret
    );

    if (*errcode_ret != CL_SUCCESS)
    {
        delete command;
        return 0;
    }

    *errcode_ret = command_queue->queueEvent(command);

    if (*errcode_ret != CL_SUCCESS)
    {
        delete command;
        return 0;
    }

    if (event)
    {
        *event = (cl_event)command;
        command->reference();
    }

    if (blocking_map)
    {
        *errcode_ret = clWaitForEvents(1, (cl_event *)&command);

        if (*errcode_ret != CL_SUCCESS)
        {
            clReleaseEvent((cl_event)command);
        }
    }

    return command->ptr();
}

void *
clEnqueueMapImage(cl_command_queue  command_queue,
                  cl_mem            image,
                  cl_bool           blocking_map,
                  cl_map_flags      map_flags,
                  const size_t *    origin,
                  const size_t *    region,
                  size_t *          image_row_pitch,
                  size_t *          image_slice_pitch,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event,
                  cl_int *          errcode_ret)
{
    return 0;
}

cl_int
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event *  event_wait_list,
                        cl_event *        event)
{
    cl_int rs = CL_SUCCESS;

    if (!command_queue)
    {
        return CL_INVALID_COMMAND_QUEUE;
    }

    Coal::UnmapBufferEvent *command = new Coal::UnmapBufferEvent(
        (Coal::CommandQueue *)command_queue,
        (Coal::MemObject *)memobj,
        mapped_ptr,
        num_events_in_wait_list, (const Coal::Event **)event_wait_list, &rs
    );

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    rs = command_queue->queueEvent(command);

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    if (event)
    {
        *event = (cl_event)command;
        command->reference();
    }

    return rs;
}

cl_int
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event)
{
    return 0;
}

cl_int
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event)
{
    return 0;
}

cl_int
clEnqueueNativeKernel(cl_command_queue  command_queue,
                      void (*user_func)(void *),
                      void *            args,
                      size_t            cb_args,
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event)
{
    cl_int rs = CL_SUCCESS;

    if (!command_queue)
        return CL_INVALID_COMMAND_QUEUE;

    Coal::NativeKernelEvent *command = new Coal::NativeKernelEvent(
        (Coal::CommandQueue *)command_queue,
        user_func, args, cb_args, num_mem_objects,
        (const Coal::MemObject **)mem_list, args_mem_loc,
        num_events_in_wait_list, (const Coal::Event **)event_wait_list, &rs
    );

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    rs = command_queue->queueEvent(command);

    if (rs != CL_SUCCESS)
    {
        delete command;
        return rs;
    }

    if (event)
    {
        *event = (cl_event)command;
        command->reference();
    }

    return CL_SUCCESS;
}

cl_int
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event)
{
    return 0;
}

cl_int
clEnqueueWaitForEvents(cl_command_queue command_queue,
                       cl_uint          num_events,
                       const cl_event * event_list)
{
    return 0;
}

cl_int
clEnqueueBarrier(cl_command_queue command_queue)
{
    return 0;
}
