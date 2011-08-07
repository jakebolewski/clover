#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "object.h"

#include <CL/cl.h>

namespace Coal
{

class DeviceInterface;

class Context : public Object
{
    public:
        Context(const cl_context_properties *properties,
                cl_uint num_devices,
                const cl_device_id *devices,
                void (CL_CALLBACK *pfn_notify)(const char *, const void *,
                                               size_t, void *),
                void *user_data,
                cl_int *errcode_ret);
        ~Context();

        cl_int info(cl_context_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;

        bool hasDevice(DeviceInterface *device) const;

    private:
        cl_context_properties *p_properties;
        void (CL_CALLBACK *p_pfn_notify)(const char *, const void *,
                                               size_t, void *);
        void *p_user_data;

        DeviceInterface **p_devices;
        unsigned int p_num_devices, p_props_len;
        cl_platform_id p_platform;
};

}

struct _cl_context : public Coal::Context
{};

#endif