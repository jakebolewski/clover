#include "cpudevice.h"

using namespace Coal;

CPUDevice::CPUDevice()
{
    
}

CPUDevice::~CPUDevice()
{
    
}
        
cl_int CPUDevice::info(cl_device_info param_name, 
                       size_t param_value_size, 
                       void *param_value, 
                       size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}