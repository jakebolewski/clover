#ifndef __CPU_BUFFER_H__
#define __CPU_BUFFER_H__

#include "../deviceinterface.h"

namespace Coal
{

class CPUDevice;
class MemObject;

class CPUBuffer : public DeviceBuffer
{
    public:
        CPUBuffer(CPUDevice *device, MemObject *buffer, cl_int *rs);
        ~CPUBuffer();

        bool allocate();
        DeviceInterface *device() const;
        void *data() const;
        void *nativeGlobalPointer() const;
        bool allocated() const;

    private:
        CPUDevice *p_device;
        MemObject *p_buffer;
        void *p_data;
        bool p_data_malloced;
};

}

#endif
