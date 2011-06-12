#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "commandqueue.h"

namespace Coal
{

class MemObject;

class ReadBufferEvent : public Event
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
        
        EventType type() const;
        
        MemObject *buffer() const;
        size_t offset() const;
        size_t cb() const;
        void *ptr() const;
        
    private:
        MemObject *p_buffer;
        size_t p_offset, p_cb;
        void *p_ptr;
};

}

#endif