#ifndef __SAMPLER_H__
#define __SAMPLER_H__

#include <CL/cl.h>
#include "object.h"

#define CLK_NORMALIZED_COORDS_FALSE 0x00000000
#define CLK_NORMALIZED_COORDS_TRUE  0x00000001
#define CLK_ADDRESS_NONE            0x00000000
#define CLK_ADDRESS_MIRRORED_REPEAT 0x00000010
#define CLK_ADDRESS_REPEAT          0x00000020
#define CLK_ADDRESS_CLAMP_TO_EDGE   0x00000030
#define CLK_ADDRESS_CLAMP           0x00000040
#define CLK_FILTER_NEAREST          0x00000000
#define CLK_FILTER_LINEAR           0x00000100

#define CLK_NORMALIZED_COORDS_MASK  0x0000000f
#define CLK_ADDRESS_MODE_MASK       0x000000f0
#define CLK_FILTER_MASK             0x00000f00

namespace Coal
{

class Context;

class Sampler : public Object
{
    public:
        Sampler(Context *ctx,
                cl_bool normalized_coords,
                cl_addressing_mode addressing_mode,
                cl_filter_mode filter_mode,
                cl_int *errcode_ret);
        Sampler(Context *ctx,
                unsigned int bitfield);

        unsigned int bitfield() const;

        cl_int info(cl_sampler_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;

    private:
        unsigned int p_bitfield;

        cl_int checkImageAvailability() const;
};

}

struct _cl_sampler : public Coal::Sampler
{};

#endif
