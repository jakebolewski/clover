#ifndef __CPUDEVICE_H__
#define __CPUDEVICE_H__

#include "deviceinterface.h"

namespace Coal
{

class MemObject;

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
        
        static unsigned int numCPUs();
        static float cpuMhz();
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