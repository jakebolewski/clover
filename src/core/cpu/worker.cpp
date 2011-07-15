#include "worker.h"
#include "device.h"
#include "buffer.h"
#include "kernel.h"

#include "../commandqueue.h"
#include "../events.h"
#include "../memobject.h"
#include "../kernel.h"

#include <cstring>

using namespace Coal;

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

        last_run = true;
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
                last_run = ke->lastNoLock();

                // Take an instance
                CPUKernelWorkGroup *instance = ke->takeInstance();

                if (!instance->run())
                    errcode = CL_INVALID_PROGRAM_EXECUTABLE;

                delete instance;

                break;
            }
            default:
                break;
        }

        // Cleanups
        if (errcode = CL_SUCCESS)
        {
            if (last_run)
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
