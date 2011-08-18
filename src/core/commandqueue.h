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

#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

#include "object.h"

#include <CL/cl.h>
#include <pthread.h>

#include <map>
#include <list>

namespace Coal
{

class Context;
class DeviceInterface;
class Event;

class CommandQueue : public Object
{
    public:
        CommandQueue(Context *ctx,
                     DeviceInterface *device,
                     cl_command_queue_properties properties,
                     cl_int *errcode_ret);
        ~CommandQueue();

        cl_int queueEvent(Event *event);

        cl_int info(cl_command_queue_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;

        cl_int setProperty(cl_command_queue_properties properties,
                           cl_bool enable,
                           cl_command_queue_properties *old_properties);

        cl_int checkProperties() const;
        void pushEventsOnDevice();
        void cleanEvents();

        void flush();
        void finish();

        Event **events(unsigned int &count); /*!< @note Retains all the events */

    private:
        DeviceInterface *p_device;
        cl_command_queue_properties p_properties;

        std::list<Event *> p_events;
        pthread_mutex_t p_event_list_mutex;
        pthread_cond_t p_event_list_cond;
        bool p_flushed;
};

class Event : public Object
{
    public:
        enum Type
        {
            NDRangeKernel = CL_COMMAND_NDRANGE_KERNEL,
            TaskKernel = CL_COMMAND_TASK,
            NativeKernel = CL_COMMAND_NATIVE_KERNEL,
            ReadBuffer = CL_COMMAND_READ_BUFFER,
            WriteBuffer = CL_COMMAND_WRITE_BUFFER,
            CopyBuffer = CL_COMMAND_COPY_BUFFER,
            ReadImage = CL_COMMAND_READ_IMAGE,
            WriteImage = CL_COMMAND_WRITE_IMAGE,
            CopyImage = CL_COMMAND_COPY_IMAGE,
            CopyImageToBuffer = CL_COMMAND_COPY_IMAGE_TO_BUFFER,
            CopyBufferToImage = CL_COMMAND_COPY_BUFFER_TO_IMAGE,
            MapBuffer = CL_COMMAND_MAP_BUFFER,
            MapImage = CL_COMMAND_MAP_IMAGE,
            UnmapMemObject = CL_COMMAND_UNMAP_MEM_OBJECT,
            Marker = CL_COMMAND_MARKER,
            AcquireGLObjects = CL_COMMAND_ACQUIRE_GL_OBJECTS,
            ReleaseGLObjects = CL_COMMAND_RELEASE_GL_OBJECTS,
            ReadBufferRect = CL_COMMAND_READ_BUFFER_RECT,
            WriteBufferRect = CL_COMMAND_WRITE_BUFFER_RECT,
            CopyBufferRect = CL_COMMAND_COPY_BUFFER_RECT,
            User = CL_COMMAND_USER,
            Barrier,
            WaitForEvents
        };

        enum Status
        {
            Queued = CL_QUEUED,
            Submitted = CL_SUBMITTED,
            Running = CL_RUNNING,
            Complete = CL_COMPLETE
        };

        typedef void (CL_CALLBACK *event_callback)(cl_event, cl_int, void *);

        struct CallbackData
        {
            event_callback callback;
            void *user_data;
        };

        enum Timing
        {
            Queue,
            Submit,
            Start,
            End,
            Max
        };

    public:
        Event(CommandQueue *parent,
              Status status,
              cl_uint num_events_in_wait_list,
              const Event **event_wait_list,
              cl_int *errcode_ret);

        void freeDeviceData();
        virtual ~Event();

        virtual Type type() const = 0;
        bool isDummy() const;      /*!< Doesn't do anything, it's just an event type */

        void setStatus(Status status);
        void setDeviceData(void *data);
        void updateTiming(Timing timing);
        Status status() const;
        void waitForStatus(Status status);
        void *deviceData();

        const Event **waitEvents(cl_uint &count) const;

        void setCallback(cl_int command_exec_callback_type,
                         event_callback callback,
                         void *user_data);

        cl_int info(cl_event_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;
        cl_int profilingInfo(cl_profiling_info param_name,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret) const;
    private:
        cl_uint p_num_events_in_wait_list;
        const Event **p_event_wait_list;

        pthread_cond_t p_state_change_cond;
        pthread_mutex_t p_state_mutex;

        Status p_status;
        void *p_device_data;
        std::multimap<Status, CallbackData> p_callbacks;

        cl_uint p_timing[Max];
};

}

struct _cl_command_queue : public Coal::CommandQueue
{};

struct _cl_event : public Coal::Event
{};

#endif
