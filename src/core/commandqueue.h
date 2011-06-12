#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

#include <CL/cl.h>
#include <pthread.h>

#include <map>
#include <list>

namespace Coal
{
    
class Context;
class DeviceInterface;
class Event;

class CommandQueue
{
    public:
        CommandQueue(Context *ctx,
                     DeviceInterface *device,
                     cl_command_queue_properties properties, 
                     cl_int *errcode_ret);
        ~CommandQueue();
        
        void reference();
        bool dereference();     /*!< @return true if reference becomes 0 */
        
        void queueEvent(Event *event);
        
        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);
        
        cl_int setProperty(cl_command_queue_properties properties,
                           cl_bool enable,
                           cl_command_queue_properties *old_properties);
        
        cl_int checkProperties() const;
        void pushEventsOnDevice();
        void cleanEvents();
        
    private:
        Context *p_ctx;
        DeviceInterface *p_device;
        cl_command_queue_properties p_properties;
        
        unsigned int p_references;
        std::list<Event *> p_events;
        pthread_mutex_t p_event_list_mutex;
};

class Event
{
    public:
        Event(CommandQueue *parent, 
              cl_uint num_events_in_wait_list, 
              const Event **event_wait_list,
              cl_int *errcode_ret);
        
        void setReleaseParent(bool release);
        ~Event();
        
        enum EventType
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
            Barrier
        };
        
        enum EventStatus
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
        
        virtual EventType type() const = 0;
        bool isSingleShot() const; /*!< Cannot be split on several execution units */
        bool isDummy() const;      /*!< Doesn't do anything, it's just an event type */
        
        void reference();
        bool dereference();     /*!< @return true if reference becomes 0 */
        
        void setStatus(EventStatus status);
        void setDeviceData(void *data);
        EventStatus status() const;
        void waitForStatus(EventStatus status);
        void *deviceData();
        
        const Event **waitEvents(cl_uint &count) const;
        
        void setCallback(cl_int command_exec_callback_type, 
                         event_callback callback,
                         void *user_data);
        
        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);
    private:
        CommandQueue *p_parent;
        cl_uint p_num_events_in_wait_list;
        const Event **p_event_wait_list;
        
        pthread_cond_t p_state_change_cond;
        pthread_mutex_t p_state_mutex;
        bool p_release_parent;
        
        unsigned int p_references;
        EventStatus p_status;
        void *p_device_data;
        std::multimap<EventStatus, CallbackData> p_callbacks;
};

}

struct _cl_command_queue : public Coal::CommandQueue
{};

struct _cl_event : public Coal::Event
{};

#endif