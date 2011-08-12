#include "stdlib.h"

int debug(const char *format, ...);

/* WARNING: Due to some device-specific things in stdlib.h, the bitcode stdlib
 * must only be used by CPUDevice, as it's targeted to the host CPU at Clover's
 * compilation! */

/*
 * Image functions
 */

int clamp(int a, int b, int c)
{
    return (a < b) ? b : ((a > c) ? c : a);
}

int __cpu_get_image_width(void *image);
int __cpu_get_image_height(void *image);
int __cpu_get_image_depth(void *image);
int __cpu_get_image_channel_data_type(void *image);
int __cpu_get_image_channel_order(void *image);
void *__cpu_image_data(void *image, int x, int y, int z, int *order, int *type);

int4 handle_address_mode(image3d_t image, int4 coord, sampler_t sampler)
{
    if ((sampler & 0xf0) == CLK_ADDRESS_NONE)
        return coord;

    int w = get_image_width(image),
        h = get_image_height(image),
        d = get_image_depth(image);
    coord.w = 0;

    // Handle address mode
    if ((sampler & 0xf0) ==  CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        coord.x = clamp(coord.x, 0, w - 1);
        coord.y = clamp(coord.y, 0, h - 1);
        coord.z = clamp(coord.z, 0, d - 1);
    }
    else if ((sampler & 0xf0) == CLK_ADDRESS_CLAMP)
    {
        coord.x = clamp(coord.x, 0, w);
        coord.y = clamp(coord.y, 0, h);
        coord.z = clamp(coord.z, 0, d);

        if (coord.x == w ||
            coord.y == h ||
            coord.z == d)
        {
            coord.w = 1;
        }
    }

    return coord;
}

float4 OVERLOAD read_imagef(image2d_t image, sampler_t sampler, int2 coord)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    return read_imagef((image3d_t)image, sampler, c);
}

float4 OVERLOAD read_imagef(image3d_t image, sampler_t sampler, int4 coord)
{
    float4 result;

    // Handle address mode
    coord = handle_address_mode(image, coord, sampler);

    if (coord.w != 0)
    {
        // Border color
        switch (get_image_channel_order(image))
        {
            case CLK_R:
            case CLK_RG:
            case CLK_RGB:
            case CLK_LUMINANCE:
                result.xyz = 0.0f;
                result.w = 1.0f;
                return result;
            default:
                result.xyzw = 0.0f;
                return result;
        }
    }

    int order, type;
    void *v_source = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

#define UNSWIZZLE(order, source, data, m)\
    switch (order)                      \
    {                                   \
        case CLK_R:                     \
        case CLK_Rx:                    \
            data.x = (*source).x;       \
            data.yz = 0;                \
            data.w = m;                 \
            break;                      \
        case CLK_A:                     \
            data.w = (*source).x;       \
            data.xyz = 0;               \
            break;                      \
        case CLK_RG:                    \
        case CLK_RGx:                   \
            data.xy = (*source).xy;     \
            data.z = 0;                 \
            data.w = m;                 \
            break;                      \
        case CLK_RA:                    \
            data.xw = (*source).xy;     \
            data.yz = 0;                \
            break;                      \
        case CLK_RGBA:                  \
            data = *source;             \
            break;                      \
        case CLK_BGRA:                  \
            data.zyxw = (*source).xyzw; \
            break;                      \
        case CLK_ARGB:                  \
            data.wxyz = (*source).xyzw; \
            break;                      \
        case CLK_INTENSITY:             \
            data.xyzw = (*source).x;    \
            break;                      \
        case CLK_LUMINANCE:             \
            data.xyz = (*source).x;     \
            data.w = m;                 \
            break;                      \
    }

    switch (type)
    {
        case CLK_UNORM_INT8:
        {
            uchar4 *source = v_source;
            uchar4 data;

            UNSWIZZLE(order, source, data, 0xff)

            result.x = (float)data.x / 255.0f;
            result.y = (float)data.y / 255.0f;
            result.z = (float)data.z / 255.0f;
            result.w = (float)data.w / 255.0f;
            break;
        }
        case CLK_UNORM_INT16:
        {
            ushort4 *source = v_source;
            ushort4 data;

            UNSWIZZLE(order, source, data, 0xffff)

            result.x = (float)data.x / 65535.0f;
            result.y = (float)data.y / 65535.0f;
            result.z = (float)data.z / 65535.0f;
            result.w = (float)data.w / 65535.0f;
            break;
        }
        case CLK_SNORM_INT8:
        {
            char4 *source = v_source;
            char4 data;

            UNSWIZZLE(order, source, data, 0x7f)

            result.x = (float)data.x / 127.0f;
            result.y = (float)data.y / 127.0f;
            result.z = (float)data.z / 127.0f;
            result.w = (float)data.w / 127.0f;
            break;
        }
        case CLK_SNORM_INT16:
        {
            short4 *source = v_source;
            short4 data;

            UNSWIZZLE(order, source, data, 0x7fff)

            result.x = (float)data.x / 32767.0f;
            result.y = (float)data.y / 32767.0f;
            result.z = (float)data.z / 32767.0f;
            result.w = (float)data.w / 32767.0f;
            break;
        }
        case CLK_FLOAT:
        {
            float4 *source = v_source;
            UNSWIZZLE(order, source, result, 1.0f)
            break;
        }
    }

#undef UNSWIZZLE

    return result;
}

float4 OVERLOAD read_imagef(image2d_t image, sampler_t sampler, float2 coord)
{
    float4 c;

    c.xy = coord;
    c.zw = 0;

    return read_imagef((image3d_t)image, sampler, c);
}

float4 OVERLOAD read_imagef(image3d_t image, sampler_t sampler, float4 coord)
{
    
}

#define UNSWIZZLE_8(source, data, m)    \
        case CLK_ARGB:                  \
            data.wxyz = (*source).xyzw; \
            break;                      \
        case CLK_BGRA:                  \
            data.zyxw = (*source).xyzw; \
            break;

#define UNSWIZZLE_16(source, data, m)   \
        case CLK_INTENSITY:             \
            data.xyzw = (*source).x;    \
            break;                      \
        case CLK_LUMINANCE:             \
            data.xyz = (*source).x;     \
            data.w = m;                 \
            break;

#define UNSWIZZLE_32(source, data, m)   \
        case CLK_R:                     \
        case CLK_Rx:                    \
            data.x = (*source).x;       \
            data.yz = 0;                \
            data.w = m;                 \
            break;                      \
        case CLK_A:                     \
            data.w = (*source).x;       \
            data.xyz = 0;               \
            break;                      \
        case CLK_RG:                    \
        case CLK_RGx:                   \
            data.xy = (*source).xy;     \
            data.z = 0;                 \
            data.w = m;                 \
            break;                      \
        case CLK_RA:                    \
            data.xw = (*source).xy;     \
            data.yz = 0;                \
            break;                      \
        case CLK_RGBA:                  \
            data = *source;             \
            break;

int4 OVERLOAD read_imagei(image2d_t image, sampler_t sampler, int2 coord)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    return read_imagei((image3d_t)image, sampler, c);
}

int4 OVERLOAD read_imagei(image3d_t image, sampler_t sampler, int4 coord)
{
    int4 result;

    // Handle address mode
    coord = handle_address_mode(image, coord, sampler);

    if (coord.w != 0)
    {
        // Border color
        switch (get_image_channel_order(image))
        {
            case CLK_R:
            case CLK_RG:
            case CLK_RGB:
            case CLK_LUMINANCE:
                result.xyz = 0;
                result.w = 0x7fffffff;
                return result;
            default:
                result.xyzw = 0;
                return result;
        }
    }

    int order, type;
    void *v_source = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

    switch (type)
    {
        case CLK_SIGNED_INT8:
        {
            char4 *source = v_source;
            char4 data;

            switch (order)
            {
                UNSWIZZLE_8(source, data, 0x7f)
                UNSWIZZLE_16(source, data, 0x7f)
                UNSWIZZLE_32(source, data, 0x7f)
            }

            result.x = data.x;
            result.y = data.y;
            result.z = data.z;
            result.w = data.w;
            break;
        }
        case CLK_SIGNED_INT16:
        {
            short4 *source = v_source;
            short4 data;

            switch (order)
            {
                UNSWIZZLE_8(source, data, 0x7fff)
                UNSWIZZLE_16(source, data, 0x7fff)
                UNSWIZZLE_32(source, data, 0x7fff)
            }

            result.x = data.x;
            result.y = data.y;
            result.z = data.z;
            result.w = data.w;
            break;
        }
        case CLK_SIGNED_INT32:
        {
            int4 *source = v_source;

            switch (order)
            {
                UNSWIZZLE_8(source, result, 0x7fffffff)
                UNSWIZZLE_16(source, result, 0x7fffffff)
                UNSWIZZLE_32(source, result, 0x7fffffff)
            }
            break;
        }
    }

    return result;
}

int4 OVERLOAD read_imagei(image2d_t image, sampler_t sampler, float2 coord)
{
    float4 c;

    c.xy = coord;
    c.zw = 0;

    return read_imagei((image3d_t)image, sampler, c);
}

int4 OVERLOAD read_imagei(image3d_t image, sampler_t sampler, float4 coord)
{
    
}

uint4 OVERLOAD read_imageui(image2d_t image, sampler_t sampler, int2 coord)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    return read_imageui((image3d_t)image, sampler, c);
}

uint4 OVERLOAD read_imageui(image3d_t image, sampler_t sampler, int4 coord)
{
    uint4 result;

    // Handle address mode
    coord = handle_address_mode(image, coord, sampler);

    if (coord.w != 0)
    {
        // Border color
        switch (get_image_channel_order(image))
        {
            case CLK_R:
            case CLK_RG:
            case CLK_RGB:
            case CLK_LUMINANCE:
                result.xyz = 0;
                result.w = 0xffffffff;
                return result;
            default:
                result.xyzw = 0;
                return result;
        }
    }

    int order, type;
    void *v_source = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

    switch (type)
    {
        case CLK_UNSIGNED_INT8:
        {
            uchar4 *source = v_source;
            uchar4 data;

            switch (order)
            {
                UNSWIZZLE_8(source, data, 0xff)
                UNSWIZZLE_16(source, data, 0xff)
                UNSWIZZLE_32(source, data, 0xff)
            }

            result.x = data.x;
            result.y = data.y;
            result.z = data.z;
            result.w = data.w;
            break;
        }
        case CLK_UNSIGNED_INT16:
        {
            ushort4 *source = v_source;
            ushort4 data;

            switch (order)
            {
                UNSWIZZLE_8(source, data, 0xffff)
                UNSWIZZLE_16(source, data, 0xffff)
                UNSWIZZLE_32(source, data, 0xffff)
            }

            result.x = data.x;
            result.y = data.y;
            result.z = data.z;
            result.w = data.w;
            break;
        }
        case CLK_UNSIGNED_INT32:
        {
            uint4 *source = v_source;

            switch (order)
            {
                UNSWIZZLE_8(source, result, 0xffffffff)
                UNSWIZZLE_16(source, result, 0xffffffff)
                UNSWIZZLE_32(source, result, 0xffffffff)
            }
            break;
        }
    }

    return result;
}

uint4 OVERLOAD read_imageui(image2d_t image, sampler_t sampler, float2 coord)
{
    float4 c;

    c.xy = coord;
    c.zw = 0;

    return read_imageui((image3d_t)image, sampler, c);
}

uint4 OVERLOAD read_imageui(image3d_t image, sampler_t sampler, float4 coord)
{
    
}

void OVERLOAD write_imagef(image2d_t image, int2 coord, float4 color)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    write_imagef((image3d_t)image, c, color);
}

void OVERLOAD write_imagef(image3d_t image, int4 coord, float4 color)
{
    int order, type;
    void *v_target = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

#define SWIZZLE(order, target, data)    \
    switch (order)                      \
    {                                   \
        case CLK_R:                     \
        case CLK_Rx:                    \
            (*target).x = data.x;       \
            break;                      \
        case CLK_A:                     \
            (*target).x = data.w;       \
            break;                      \
        case CLK_RG:                    \
        case CLK_RGx:                   \
            (*target).xy = data.xy;     \
            break;                      \
        case CLK_RA:                    \
            (*target).xy = data.xw;     \
            break;                      \
        case CLK_RGBA:                  \
            *target = data;             \
            break;                      \
        case CLK_BGRA:                  \
            (*target).zyxw = data.xyzw; \
            break;                      \
        case CLK_ARGB:                  \
            (*target).wxyz = data.xyzw; \
            break;                      \
        case CLK_INTENSITY:             \
        case CLK_LUMINANCE:             \
            (*target).x = data.x;       \
            break;                      \
    }

    switch (type)
    {
        case CLK_SNORM_INT8:
        {
            char4 *target = v_target;
            char4 data;

            // Denormalize
            data.x = (color.x * 127.0f);
            data.y = (color.y * 127.0f);
            data.z = (color.z * 127.0f);
            data.w = (color.w * 127.0f);

            SWIZZLE(order, target, data)
            break;
        }
        case CLK_UNORM_INT8:
        {
            uchar4 *target = v_target;
            uchar4 data;

            // Denormalize
            data.x = (color.x * 255.0f);
            data.y = (color.y * 255.0f);
            data.z = (color.z * 255.0f);
            data.w = (color.w * 255.0f);

            SWIZZLE(order, target, data)
            break;
        }
        case CLK_SNORM_INT16:
        {
            short4 *target = v_target;
            short4 data;

            // Denormalize
            data.x = (color.x * 32767.0f);
            data.y = (color.y * 32767.0f);
            data.z = (color.z * 32767.0f);
            data.w = (color.w * 32767.0f);

            SWIZZLE(order, target, data)
            break;
        }
        case CLK_UNORM_INT16:
        {
            ushort4 *target = v_target;
            ushort4 data;

            data.x = (color.x * 65535.0f);
            data.y = (color.y * 65535.0f);
            data.z = (color.z * 65535.0f);
            data.w = (color.w * 65535.0f);

            SWIZZLE(order, target, data)
            break;
        }
        case CLK_FLOAT:
        {
            float4 *target = v_target;

            SWIZZLE(order, target, color)
            break;
        }
    }

#undef SWIZZLE
}

#define SWIZZLE_8(target, data)         \
        case CLK_ARGB:                  \
            (*target).wxyz = data.xyzw; \
            break;                      \
        case CLK_BGRA:                  \
            (*target).zyxw = data.xyzw; \
            break;

#define SWIZZLE_16(target, data)        \
        case CLK_LUMINANCE:             \
        case CLK_INTENSITY:             \
            (*target).x = data.x;       \
            break;

#define SWIZZLE_32(target, data)        \
        case CLK_R:                     \
        case CLK_Rx:                    \
            (*target).x = data.x;       \
            break;                      \
        case CLK_A:                     \
            (*target).x = data.w;       \
            break;                      \
        case CLK_RG:                    \
        case CLK_RGx:                   \
            (*target).xy = data.xy;     \
            break;                      \
        case CLK_RA:                    \
            (*target).xy = data.xw;     \
            break;                      \
        case CLK_RGBA:                  \
            *target = data;             \
            break;

void OVERLOAD write_imagei(image2d_t image, int2 coord, int4 color)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    write_imagei((image3d_t)image, c, color);
}

void OVERLOAD write_imagei(image3d_t image, int4 coord, int4 color)
{
    int order, type;
    void *v_target = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

    switch (type)
    {
        case CLK_SIGNED_INT8:
        {
            char4 *target = v_target;
            char4 data;

            data.x = color.x;
            data.y = color.y;
            data.z = color.z;
            data.w = color.w;

            switch (order)
            {
                SWIZZLE_8(target, data)
                SWIZZLE_16(target, data)
                SWIZZLE_32(target, data)
            }

            break;
        }
        case CLK_SIGNED_INT16:
        {
            short4 *target = v_target;
            short4 data;

            data.x = color.x;
            data.y = color.y;
            data.z = color.z;
            data.w = color.w;

            switch (order)
            {
                SWIZZLE_16(target, data)
                SWIZZLE_32(target, data)
            }

            break;
        }
        case CLK_SIGNED_INT32:
        {
            int4 *target = v_target;

            switch (order)
            {
                SWIZZLE_32(target, color)
            }

            break;
        }
    }
}

void OVERLOAD write_imageui(image2d_t image, int2 coord, uint4 color)
{
    int4 c;
    c.xy = coord;
    c.zw = 0;

    write_imageui((image3d_t)image, c, color);
}

void OVERLOAD write_imageui(image3d_t image, int4 coord, uint4 color)
{
    int order, type;
    void *v_target = __cpu_image_data(image, coord.x, coord.y, coord.z, &order, &type);

    switch (type)
    {
        case CLK_UNSIGNED_INT8:
        {
            uchar4 *target = v_target;
            uchar4 data;

            data.x = color.x;
            data.y = color.y;
            data.z = color.z;
            data.w = color.w;

            switch (order)
            {
                SWIZZLE_8(target, data)
                SWIZZLE_16(target, data)
                SWIZZLE_32(target, data)
            }
        }
        case CLK_UNSIGNED_INT16:
        {
            ushort4 *target = v_target;
            ushort4 data;

            data.x = color.x;
            data.y = color.y;
            data.z = color.z;
            data.w = color.w;

            switch (order)
            {
                SWIZZLE_16(target, data)
                SWIZZLE_32(target, data)
            }
        }
        case CLK_UNSIGNED_INT32:
        {
            uint4 *target = v_target;

            switch (order)
            {
                SWIZZLE_32(target, color)
            }
        }
    }
}

#undef SWIZZLE_8
#undef SWIZZLE_16
#undef SWIZZLE_32

int2 OVERLOAD get_image_dim(image2d_t image)
{
    int2 result;

    result.x = get_image_width(image);
    result.y = get_image_height(image);

    return result;
}

int4 OVERLOAD get_image_dim(image3d_t image)
{
    int4 result;

    result.x = get_image_width(image);
    result.y = get_image_height(image);
    result.z = get_image_depth(image);

    return result;
}

int OVERLOAD get_image_width(image2d_t image)
{
    return __cpu_get_image_width(image);
}

int OVERLOAD get_image_width(image3d_t image)
{
    return __cpu_get_image_width(image);
}

int OVERLOAD get_image_height(image2d_t image)
{
    return __cpu_get_image_height(image);
}

int OVERLOAD get_image_height(image3d_t image)
{
    return __cpu_get_image_height(image);
}

int OVERLOAD get_image_depth(image3d_t image)
{
    return __cpu_get_image_depth(image);
}

int OVERLOAD get_image_channel_data_type(image2d_t image)
{
    return __cpu_get_image_channel_data_type(image);
}

int OVERLOAD get_image_channel_data_type(image3d_t image)
{
    return __cpu_get_image_channel_data_type(image);
}

int OVERLOAD get_image_channel_order(image2d_t image)
{
    return __cpu_get_image_channel_order(image);
}

int OVERLOAD get_image_channel_order(image3d_t image)
{
    return __cpu_get_image_channel_order(image);
}

