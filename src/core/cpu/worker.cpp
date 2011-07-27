#include "worker.h"
#include "device.h"
#include "buffer.h"
#include "kernel.h"

#include "../commandqueue.h"
#include "../events.h"
#include "../memobject.h"
#include "../kernel.h"

#include <cstring>
#include <iostream>

using namespace Coal;

static unsigned char *imageData(unsigned char *base, size_t x, size_t y,
                                size_t z, size_t row_pitch, size_t slice_pitch,
                                unsigned int bytes_per_pixel)
{
    unsigned char *result = base;

    result += (z * slice_pitch) +
              (y * row_pitch) +
              (x * bytes_per_pixel);

    return result;
}

void *worker(void *data)
{
    CPUDevice *device = (CPUDevice *)data;
    bool stop = false, last_run;
    cl_int errcode;
    Event *event;

    while (true)
    {
        event = device->getEvent(stop);

        // Ensure we have a good event and we don't have to stop
        if (stop) break;
        if (!event) continue;

        // Get info about the event and its command queue
        Event::Type t = event->type();
        CommandQueue *queue = 0;
        cl_command_queue_properties queue_props = 0;

        errcode = CL_SUCCESS;

        event->info(CL_EVENT_COMMAND_QUEUE, sizeof(CommandQueue *), &queue, 0);

        if (queue)
            queue->info(CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties),
                        &queue_props, 0);

        if (queue_props & CL_QUEUE_PROFILING_ENABLE)
            event->updateTiming(Event::Start);

        // Execute the action
        switch (t)
        {
            case Event::ReadBuffer:
            case Event::WriteBuffer:
            {
                ReadWriteBufferEvent *e = (ReadWriteBufferEvent *)event;
                CPUBuffer *buf = (CPUBuffer *)e->buffer()->deviceBuffer(device);
                char *data = (char *)buf->data();

                data += e->offset();

                if (t == Event::ReadBuffer)
                    std::memcpy(e->ptr(), data, e->cb());
                else
                    std::memcpy(data, e->ptr(), e->cb());

                break;
            }
            case Event::CopyBuffer:
            {
                CopyBufferEvent *e = (CopyBufferEvent *)event;
                CPUBuffer *src = (CPUBuffer *)e->source()->deviceBuffer(device);
                CPUBuffer *dst = (CPUBuffer *)e->destination()->deviceBuffer(device);

                std::memcpy(dst->data(), src->data(), e->cb());

                break;
            }
            case Event::ReadBufferRect:
            case Event::WriteBufferRect:
            {
                ReadWriteBufferRectEvent *e = (ReadWriteBufferRectEvent *)event;
                CPUBuffer *buf = (CPUBuffer *)e->buffer()->deviceBuffer(device);

                unsigned char *host = (unsigned char *)e->ptr();
                unsigned char *mem = (unsigned char *)buf->data();

                // Iterate over the lines to copy and use memcpy
                for (size_t z=0; z<e->region(2); ++z)
                {
                    for (size_t y=0; y<e->region(1); ++y)
                    {
                        unsigned char *h;
                        unsigned char *b;

                        h = imageData(host,
                                      e->host_origin(0),
                                      y + e->host_origin(1),
                                      z + e->host_origin(2),
                                      e->host_row_pitch(),
                                      e->host_slice_pitch(),
                                      1);

                        b = imageData(mem,
                                      e->buffer_origin(0),
                                      y + e->buffer_origin(1),
                                      z + e->buffer_origin(2),
                                      e->buffer_row_pitch(),
                                      e->buffer_slice_pitch(),
                                      1);
                        if (t == Event::ReadBufferRect)
                            std::memcpy(h, b, e->region(0));
                        else
                            std::memcpy(b, h, e->region(0));
                    }
                }

                break;
            }
            case Event::MapBuffer:
                // All was already done in CPUBuffer::initEventDeviceData()
                break;

            case Event::NativeKernel:
            {
                NativeKernelEvent *e = (NativeKernelEvent *)event;
                void (*func)(void *) = (void (*)(void *))e->function();
                void *args = e->args();

                func(args);

                break;
            }
            case Event::NDRangeKernel:
            case Event::TaskKernel:
            {
                KernelEvent *e = (KernelEvent *)event;
                CPUKernelEvent *ke = (CPUKernelEvent *)e->deviceData();

                // Take an instance
                CPUKernelWorkGroup *instance = ke->takeInstance();
                ke = 0;     // Unlocked, don't use anymore

                if (!instance->run())
                    errcode = CL_INVALID_PROGRAM_EXECUTABLE;

                delete instance;

                break;
            }
            default:
                break;
        }

        // Cleanups
        if (errcode == CL_SUCCESS)
        {
            bool finished = true;

            if (event->type() == Event::NDRangeKernel ||
                event->type() == Event::TaskKernel)
            {
                CPUKernelEvent *ke = (CPUKernelEvent *)event->deviceData();
                finished = ke->finished();
            }

            if (finished)
            {
                event->setStatus(Event::Complete);

                if (queue_props & CL_QUEUE_PROFILING_ENABLE)
                    event->updateTiming(Event::End);

                // Clean the queue
                if (queue)
                    queue->cleanEvents();
            }
        }
        else
        {
            // The event failed
            event->setStatus((Event::Status)errcode);

            if (queue_props & CL_QUEUE_PROFILING_ENABLE)
                    event->updateTiming(Event::End);
        }
    }

    return 0;
}
