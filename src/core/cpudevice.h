#ifndef __CPUDEVICE_H__
#define __CPUDEVICE_H__

#include "deviceinterface.h"

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

        cl_int info(cl_device_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret);

        DeviceBuffer *createDeviceBuffer(MemObject *buffer, cl_int *rs);
        DeviceProgram *createDeviceProgram(Program *program);
        DeviceKernel *createDeviceKernel(Kernel *kernel);

        cl_int initEventDeviceData(Event *event);

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

class CPUProgram : public DeviceProgram
{
    public:
        CPUProgram(CPUDevice *device, Program *program);
        ~CPUProgram();

        bool linkStdLib() const;
        void createOptimizationPasses(llvm::PassManager *manager, bool optimize);
        bool build(const llvm::Module *module);

    private:
        CPUDevice *p_device;
        Program *p_program;
};

class CPUKernel : public DeviceKernel
{
    public:
        CPUKernel(CPUDevice *device, Kernel *kernel);
        ~CPUKernel();

        size_t workGroupSize() const;
		cl_ulong localMemSize() const;
        cl_ulong privateMemSize() const;
		size_t preferredWorkGroupSizeMultiple() const;

    private:
        CPUDevice *p_device;
        Kernel *p_kernel;
};

}

#endif
