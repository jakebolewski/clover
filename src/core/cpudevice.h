#ifndef __CPUDEVICE_H__
#define __CPUDEVICE_H__

#include "deviceinterface.h"

#include <pthread.h>
#include <list>

namespace Coal
{

class MemObject;
class Event;

class CPUDevice : public DeviceInterface
{
    public:
        CPUDevice();
        ~CPUDevice();
        
        cl_int info(cl_device_info param_name, 
                    size_t param_value_size, 
                    void *param_value, 
                    size_t *param_value_size_ret);
        DeviceBuffer *createDeviceBuffer(MemObject *buffer, cl_int *rs);
        void pushEvent(Event *event);
        
        Event *getEvent(bool &stop);
        
        unsigned int numCPUs();
        float cpuMhz();
        
    private:
        unsigned int p_cores, p_num_events;
        pthread_t *p_workers;
        
        std::list<Event *> p_events;
        pthread_cond_t p_events_cond;
        pthread_mutex_t p_events_mutex;
        bool p_stop;
};

class CPUBuffer : public DeviceBuffer
{
    public:
        CPUBuffer(CPUDevice *device, MemObject *buffer, cl_int *rs);
        ~CPUBuffer();
        
        cl_int allocate();
        DeviceInterface *device() const;
        void *data() const;
        bool allocated() const;
        
    private:
        CPUDevice *p_device;
        MemObject *p_buffer;
        void *p_data;
        bool p_data_malloced;
};

}

#endif