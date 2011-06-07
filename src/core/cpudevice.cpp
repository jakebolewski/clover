#include "cpudevice.h"
#include "memobject.h"
#include "config.h"
#include "propertylist.h"
#include "commandqueue.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace Coal;
using namespace std;

/*
 * Worker code
 */

static void *worker(void *data)
{
    CPUDevice *device = (CPUDevice *)data;
    bool stop = false;
    Event *event;
    
    while (true)
    {
        event = device->getEvent(stop);
        
        if (stop) break;
        if (!event) continue;
        
        // Run the event
    }
}

/*
 * CPUDevice
 */

CPUDevice::CPUDevice() 
: DeviceInterface::DeviceInterface(), p_cores(0), p_workers(0), p_stop(false),
  p_num_events(0)
{
    // Initialize the locking machinery
    pthread_cond_init(&p_events_cond, 0);
    pthread_mutex_init(&p_events_mutex, 0);
    
    // Create 4 worker threads
    p_workers = (pthread_t *)malloc(numCPUs() * sizeof(pthread_t));
    
    for (int i=0; i<numCPUs(); ++i)
    {
        pthread_create(&p_workers[i], 0, &worker, this);
    }
}

CPUDevice::~CPUDevice()
{
    // Terminate the workers and wait for them
    pthread_mutex_lock(&p_events_mutex);
    
    p_stop = true;
    
    pthread_cond_broadcast(&p_events_cond);
    pthread_mutex_unlock(&p_events_mutex);
    
    for (int i=0; i<numCPUs(); ++i)
    {
        pthread_join(p_workers[i], 0);
    }
    
    // Free allocated memory
    free((void *)p_workers);
    pthread_mutex_destroy(&p_events_mutex);
    pthread_cond_destroy(&p_events_cond);
}

DeviceBuffer *CPUDevice::createDeviceBuffer(MemObject *buffer, cl_int *rs)
{
    return (DeviceBuffer *)new CPUBuffer(this, buffer, rs);
}

void CPUDevice::pushEvent(Event *event)
{
    // Add an event in the list
    pthread_mutex_lock(&p_events_mutex);
    
    p_events.push_back(event);
    p_num_events++;                 // Way faster than STL list::size() !
    
    pthread_cond_broadcast(&p_events_cond);
    pthread_mutex_unlock(&p_events_mutex);
}

Event *CPUDevice::getEvent(bool &stop)
{
    // Return the first event in the list, if any. Remove it if it is a
    // single-shot event.
    pthread_mutex_lock(&p_events_mutex);
    
    while (p_num_events == 0)
        pthread_cond_wait(&p_events_cond, &p_events_mutex);
    
    Event *event = p_events.front();
    
    // If event is single-shot, remove it
    if (event->isSingleShot())
    {
        p_num_events--;
        p_events.pop_front();
    }
    
    pthread_mutex_unlock(&p_events_mutex);
    
    return event;
}

unsigned int CPUDevice::numCPUs()
{
    if (p_cores) return p_cores;
    
    return (p_cores = sysconf(_SC_NPROCESSORS_ONLN));
}

float CPUDevice::cpuMhz()
{
    filebuf fb;
    fb.open("/proc/cpuinfo", ios::in);
    istream is(&fb);

    float cpuMhz = 0.0;

    while (!is.eof())
    {
        string key, value;

        getline(is, key, ':');
        is.ignore(1);
        getline(is, value);

        if (key.compare(0, 7, "cpu MHz") == 0)
        {
            istringstream ss(value);
            ss >> cpuMhz;
            break;
        }
    }

    return cpuMhz;
}

cl_int CPUDevice::info(cl_device_info param_name, 
                       size_t param_value_size, 
                       void *param_value, 
                       size_t *param_value_size_ret)
{
    void *value = 0;
    int value_length = 0;
    
    union {
        cl_device_type cl_device_type_var;
        cl_uint cl_uint_var;
        size_t size_t_var;
        cl_ulong cl_ulong_var;
        cl_bool cl_bool_var;
        cl_device_fp_config cl_device_fp_config_var;
        cl_device_mem_cache_type cl_device_mem_cache_type_var;
        cl_device_local_mem_type cl_device_local_mem_type_var;
        cl_device_exec_capabilities cl_device_exec_capabilities_var;
        cl_command_queue_properties cl_command_queue_properties_var;
        cl_platform_id cl_platform_id_var;
        size_t three_size_t[3];
    };
    
    switch (param_name)
    {
        case CL_DEVICE_TYPE:
            SIMPLE_ASSIGN(cl_device_type, CL_DEVICE_TYPE_CPU);
            break;
        
        case CL_DEVICE_VENDOR_ID:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;
        
        case CL_DEVICE_MAX_COMPUTE_UNITS:
            SIMPLE_ASSIGN(cl_uint, numCPUs());
            break;
        
        case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
            // TODO: Spec minimum
            SIMPLE_ASSIGN(cl_uint, 3);
            break;
        
        case CL_DEVICE_MAX_WORK_GROUP_SIZE:
            // TODO: Spec minimum
            SIMPLE_ASSIGN(size_t, 1);
            break;
        
        case CL_DEVICE_MAX_WORK_ITEM_SIZES:
            three_size_t[0] = 1;
            three_size_t[1] = 1;
            three_size_t[2] = 1;
            value_length = 3 * sizeof(size_t);
            value = &three_size_t;
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
            SIMPLE_ASSIGN(cl_uint, 8);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;
        
        case CL_DEVICE_MAX_CLOCK_FREQUENCY:
            SIMPLE_ASSIGN(cl_uint, cpuMhz() * 1000000);
            break;
        
        case CL_DEVICE_ADDRESS_BITS:
            SIMPLE_ASSIGN(cl_uint, 32);
            break;
        
        case CL_DEVICE_MAX_READ_IMAGE_ARGS:
            SIMPLE_ASSIGN(cl_uint, 65536);
            break;
        
        case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
            SIMPLE_ASSIGN(cl_uint, 65536);
            break;
        
        case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
            SIMPLE_ASSIGN(cl_ulong, 128*1024*1024);
            break;
        
        case CL_DEVICE_IMAGE2D_MAX_WIDTH:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_IMAGE3D_MAX_WIDTH:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_IMAGE3D_MAX_DEPTH:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_IMAGE_SUPPORT:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;
        
        case CL_DEVICE_MAX_PARAMETER_SIZE:
            SIMPLE_ASSIGN(size_t, 65536);
            break;
        
        case CL_DEVICE_MAX_SAMPLERS:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_SINGLE_FP_CONFIG:
            // TODO: Check what an x86 SSE engine can support.
            SIMPLE_ASSIGN(cl_device_fp_config, 
                          CL_FP_DENORM |
                          CL_FP_INF_NAN |
                          CL_FP_ROUND_TO_NEAREST);
            break;
        
        case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
            SIMPLE_ASSIGN(cl_device_mem_cache_type,
                          CL_READ_WRITE_CACHE);
            break;
        
        case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
            // TODO: Get this information from the processor
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
            // TODO: Get this information from the processor
            SIMPLE_ASSIGN(cl_ulong, 512*1024*1024);
            break;
        
        case CL_DEVICE_GLOBAL_MEM_SIZE:
            // TODO: 1 Gio seems to be enough for software acceleration
            SIMPLE_ASSIGN(cl_ulong, 1*1024*1024*1024);
            break;
        
        case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
            SIMPLE_ASSIGN(cl_ulong, 1*1024*1024*1024);
            break;
        
        case CL_DEVICE_MAX_CONSTANT_ARGS:
            SIMPLE_ASSIGN(cl_uint, 65536);
            break;
        
        case CL_DEVICE_LOCAL_MEM_TYPE:
            SIMPLE_ASSIGN(cl_device_local_mem_type, CL_GLOBAL);
            break;
        
        case CL_DEVICE_LOCAL_MEM_SIZE:
            SIMPLE_ASSIGN(cl_ulong, 1*1024*1024*1024);
            break;
        
        case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
            SIMPLE_ASSIGN(cl_bool, CL_FALSE);
            break;
        
        case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
            SIMPLE_ASSIGN(size_t, 1000);        // 1000 nanoseconds = 1 ms
            break;
        
        case CL_DEVICE_ENDIAN_LITTLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;
        
        case CL_DEVICE_AVAILABLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;
        
        case CL_DEVICE_COMPILER_AVAILABLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;
        
        case CL_DEVICE_EXECUTION_CAPABILITIES:
            SIMPLE_ASSIGN(cl_device_exec_capabilities, CL_EXEC_KERNEL);
            break;
        
        case CL_DEVICE_QUEUE_PROPERTIES:
            SIMPLE_ASSIGN(cl_command_queue_properties,
                          CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                          CL_QUEUE_PROFILING_ENABLE);
            break;
        
        case CL_DEVICE_NAME:
            STRING_ASSIGN("CPU");
            break;
        
        case CL_DEVICE_VENDOR:
            STRING_ASSIGN("Mesa");
            break;
        
        case CL_DRIVER_VERSION:
            STRING_ASSIGN("" COAL_VERSION);
            break;
        
        case CL_DEVICE_PROFILE:
            STRING_ASSIGN("FULL_PROFILE");
            break;
        
        case CL_DEVICE_VERSION:
            STRING_ASSIGN("OpenCL 1.1 Mesa " COAL_VERSION);
            break;
        
        case CL_DEVICE_EXTENSIONS:
            STRING_ASSIGN("cl_khr_global_int32_base_atomics"
                          " cl_khr_global_int32_extended_atomics"
                          " cl_khr_local_int32_base_atomics"
                          " cl_khr_local_int32_extended_atomics"
                          " cl_khr_byte_addressable_store"
                          
                          " cl_khr_fp64"
                          " cl_khr_int64_base_atomics"
                          " cl_khr_int64_extended_atomics")

            break;
        
        case CL_DEVICE_PLATFORM:
            SIMPLE_ASSIGN(cl_platform_id, 0);
            break;
        
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;
        
        case CL_DEVICE_HOST_UNIFIED_MEMORY:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
            SIMPLE_ASSIGN(cl_uint, 8);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;
        
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;
        
        case CL_DEVICE_OPENCL_C_VERSION:
            STRING_ASSIGN("OpenCL C 1.1 LLVM " LLVM_VERSION);
            break;
            
        default:
            return CL_INVALID_VALUE;
    }
    
    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;
    
    if (param_value_size_ret)
        *param_value_size_ret = value_length;
        
    if (param_value)
        memcpy(param_value, value, value_length);
    
    return CL_SUCCESS;
}

/*
 * CPUBuffer
 */
CPUBuffer::CPUBuffer(CPUDevice *device, MemObject *buffer, cl_int *rs)
: p_device(device), p_buffer(buffer), p_data(0), p_data_malloced(false)
{
    if (buffer->type() == MemObject::SubBuffer)
    {
        // We need to create this CPUBuffer based on the CPUBuffer of the
        // parent buffer
        SubBuffer *subbuf = (SubBuffer *)buffer;
        MemObject *parent = subbuf->parent();
        CPUBuffer *parentcpubuf = (CPUBuffer *)parent->deviceBuffer(device);
        
        char *tmp_data = (char *)parentcpubuf->data();
        tmp_data += subbuf->offset();
        
        p_data = (void *)tmp_data;
    }
    else if (buffer->flags() & CL_MEM_USE_HOST_PTR)
    {
        // We use the host ptr, we are already allocated
        p_data = buffer->host_ptr();
    }
}

CPUBuffer::~CPUBuffer()
{
    if (p_data_malloced)
    {
        free((void *)p_data);
    }
}

void *CPUBuffer::data() const
{
    return p_data;
}
        
cl_int CPUBuffer::allocate()
{
    size_t buf_size = p_buffer->size();
    
    if (buf_size == 0)
        // Something went wrong...
        return CL_MEM_OBJECT_ALLOCATION_FAILURE;
    
    if (!p_data)
    {
        // We don't use a host ptr, we need to allocate a buffer
        p_data = malloc(buf_size);
        
        if (!p_data)
            return CL_MEM_OBJECT_ALLOCATION_FAILURE;
        
        p_data_malloced = true;
    }
    
    if (p_buffer->type() != MemObject::SubBuffer &&
        p_buffer->flags() & CL_MEM_COPY_HOST_PTR)
    {
        memcpy(p_data, p_buffer->host_ptr(), buf_size);
    }
    
    // Say to the memobject that we are allocated
    p_buffer->deviceAllocated(this);
    
    return CL_SUCCESS;
}

DeviceInterface *CPUBuffer::device() const
{
    return p_device;
}

bool CPUBuffer::allocated() const
{
    return p_data != 0;
}