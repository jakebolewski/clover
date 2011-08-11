#ifndef __CPU_DEVICE_H__
#define __CPU_DEVICE_H__

#include "../deviceinterface.h"

#include <pthread.h>
#include <list>

namespace Coal
{

class MemObject;
class Event;
class Program;
class Kernel;

class CPUDevice : public DeviceInterface
{
    public:
        CPUDevice();
        ~CPUDevice();

        void init();

        cl_int info(cl_device_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;

        DeviceBuffer *createDeviceBuffer(MemObject *buffer, cl_int *rs);
        DeviceProgram *createDeviceProgram(Program *program);
        DeviceKernel *createDeviceKernel(Kernel *kernel,
                                         llvm::Function *function);

        cl_int initEventDeviceData(Event *event);
        void freeEventDeviceData(Event *event);

        void pushEvent(Event *event);
        Event *getEvent(bool &stop);

        unsigned int numCPUs() const;
        float cpuMhz() const;

    private:
        unsigned int p_cores, p_num_events;
        float p_cpu_mhz;
        pthread_t *p_workers;

        std::list<Event *> p_events;
        pthread_cond_t p_events_cond;
        pthread_mutex_t p_events_mutex;
        bool p_stop, p_initialized;
};

}

#endif
