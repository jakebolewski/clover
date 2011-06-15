#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "commandqueue.h"

#include <vector>

namespace Coal
{

class MemObject;

class BufferEvent : public Event
{
    public:
        BufferEvent(CommandQueue *parent, 
                    MemObject *buffer,
                    size_t offset,
                    size_t cb,
                    void *ptr,
                    cl_map_flags map_flags,
                    EventType type,
                    cl_uint num_events_in_wait_list, 
                    const Event **event_wait_list,
                    cl_int *errcode_ret);
        
        EventType type() const;
        
        MemObject *buffer() const;
        size_t offset() const;
        size_t cb() const;
        void *ptr() const;
        void setPtr(void *ptr);
        
    private:
        MemObject *p_buffer;
        size_t p_offset, p_cb;
        void *p_ptr;
        cl_map_flags p_map_flags;
        EventType p_type;
};

class UserEvent : public Event
{
    public:
        UserEvent(Context *context, cl_int *errcode_ret);
        
        EventType type() const;
        Context *context() const;
        void addDependentCommandQueue(CommandQueue *queue);  // We need to call pushOnDevice somewhere
        void flushQueues();
        
    private:
        Context *p_context;
        std::vector<CommandQueue *> p_dependent_queues;
};

}

#endif