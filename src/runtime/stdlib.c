/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
int __cpu_is_image_3d(void *image);
void *__cpu_image_data(void *image, int x, int y, int z, int *order, int *type);

void __cpu_write_imagef(void *image, int x, int y, int z, float4 *color);
void __cpu_write_imagei(void *image, int x, int y, int z, int4 *color);
void __cpu_write_imageui(void *image, int x, int y, int z, uint4 *color);

int4 handle_address_mode(image3d_t image, int4 coord, sampler_t sampler)
{
    coord.w = 0;

    int w = get_image_width(image),
        h = get_image_height(image),
        d = get_image_depth(image);

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
    }

    if (coord.x == w ||
        coord.y == h ||
        coord.z == d)
    {
        coord.w = 1;
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

int4 f2i_floor(float4 value)
{
    int4 result = __builtin_ia32_cvtps2dq(value);
    value = __builtin_ia32_psrldi128((int4)value, 31);
    result -= (int4)value;
    return result;
}

float4 f2f_floor(float4 value)
{
    return __builtin_ia32_cvtdq2ps(f2i_floor(value));
}

#define LINEAR_3D(t_max, suf)                           \
    (t_max - a) * (t_max - b) - (t_max - c) *           \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 1, 2, 3)) +  \
    a * (t_max - b) * (t_max - c) *                     \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 1, 2, 3)) +  \
    (t_max - a) * b * (t_max - c) *                     \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 5, 2, 3)) +  \
    a * b * (t_max - c) *                               \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 5, 2, 3)) +  \
    (t_max - a) * (t_max - b) * c *                     \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 1, 6, 3)) +  \
    a * (t_max - b) * c *                               \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 1, 6, 3)) +  \
    (t_max - a) * b * c *                               \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 5, 6, 3)) +  \
    a * b * c *                                         \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 5, 6, 3))

#define LINEAR_2D(t_max, suf)                           \
    (t_max - a) * (t_max - b) *                         \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 1, 2, 2)) +  \
    a * (t_max - b) *                                   \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 1, 2, 2)) +  \
    (t_max - a) * b *                                   \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 0, 5, 2, 2)) +  \
    a * b *                                             \
    read_image##suf(image, sampler,                     \
        __builtin_shufflevector(V0, V1, 4, 5, 2, 2));

#define READ_IMAGE(type, suf, type_max)                                        \
    type##4 result;                                                            \
                                                                               \
    switch (sampler & 0xf0)                                                    \
    {                                                                          \
        case CLK_ADDRESS_NONE:                                                 \
        case CLK_ADDRESS_CLAMP:                                                \
        case CLK_ADDRESS_CLAMP_TO_EDGE:                                        \
            /* Denormalize coords */                                           \
            if ((sampler & 0xf) == CLK_NORMALIZED_COORDS_TRUE)                 \
                coord *= __builtin_ia32_cvtdq2ps(get_image_dim(image));        \
                                                                               \
            switch (sampler & 0xf00)                                           \
            {                                                                  \
                case CLK_FILTER_NEAREST:                                       \
                {                                                              \
                    int4 c = f2i_floor(coord);                                 \
                                                                               \
                    return read_image##suf(image, sampler, c);                 \
                }                                                              \
                case CLK_FILTER_LINEAR:                                        \
                {                                                              \
                    type a, b, c;                                              \
                                                                               \
                    coord -= 0.5f;                                             \
                                                                               \
                    int4 V0, V1;                                               \
                                                                               \
                    V0 = f2i_floor(coord);                                     \
                    V1 = f2i_floor(coord) + 1;                                 \
                                                                               \
                    coord -= f2f_floor(coord);                                 \
                                                                               \
                    a = (type)(coord.x * type_max);                            \
                    b = (type)(coord.y * type_max);                            \
                    c = (type)(coord.z * type_max);                            \
                                                                               \
                    if (__cpu_is_image_3d(image))                              \
                    {                                                          \
                        result = LINEAR_3D(type_max, suf);                     \
                    }                                                          \
                    else                                                       \
                    {                                                          \
                        result = LINEAR_2D(type_max, suf);                     \
                    }                                                          \
                }                                                              \
            }                                                                  \
            break;                                                             \
        case CLK_ADDRESS_REPEAT:                                               \
            switch (sampler & 0xf00)                                           \
            {                                                                  \
                case CLK_FILTER_NEAREST:                                       \
                {                                                              \
                    int4 dim = get_image_dim(image);                           \
                    coord = (coord - f2f_floor(coord)) *                       \
                                __builtin_ia32_cvtdq2ps(dim);                  \
                                                                               \
                    int4 c = f2i_floor(coord);                                 \
                                                                               \
                    /* if (c > dim - 1) c = c - dim */                         \
                    int4 mask = __builtin_ia32_pcmpgtd128(c, dim - 1);         \
                    int4 repl = c - dim;                                       \
                    c = (repl & mask) | (c & ~mask);                           \
                                                                               \
                    return read_image##suf(image, sampler, c);                 \
                }                                                              \
                case CLK_FILTER_LINEAR:                                        \
                {                                                              \
                    type a, b, c;                                              \
                                                                               \
                    int4 dim = get_image_dim(image);                           \
                    coord = (coord - f2f_floor(coord)) *                       \
                                __builtin_ia32_cvtdq2ps(dim);                  \
                                                                               \
                    float4 tmp = coord;                                        \
                    tmp -= 0.5f;                                               \
                    tmp -= f2f_floor(tmp);                                     \
                                                                               \
                    a = (type)(tmp.x * type_max);                              \
                    b = (type)(tmp.y * type_max);                              \
                    c = (type)(tmp.z * type_max);                              \
                                                                               \
                    int4 V0, V1;                                               \
                                                                               \
                    V0 = f2i_floor(coord - 0.5f);                              \
                    V1 = V0 + 1;                                               \
                                                                               \
                    /* if (0 > V0) V0 = dim + V0 */                            \
                    int4 zero = 0;                                             \
                    int4 mask = __builtin_ia32_pcmpgtd128(zero, V0);           \
                    int4 repl = dim + V0;                                      \
                    V0 = (repl & mask) | (V0 & ~mask);                         \
                                                                               \
                    /* if (V1 > dim - 1) V1 = V1 - dim */                      \
                    mask = __builtin_ia32_pcmpgtd128(V1, dim);                 \
                    repl = V1 - dim;                                           \
                    V1 = (repl & mask) | (V0 & ~mask);                         \
                                                                               \
                    if (__cpu_is_image_3d(image))                              \
                    {                                                          \
                        result = LINEAR_3D(type_max, suf);                     \
                    }                                                          \
                    else                                                       \
                    {                                                          \
                        result = LINEAR_2D(type_max, suf);                     \
                    }                                                          \
                }                                                              \
            }                                                                  \
            break;                                                             \
        case CLK_ADDRESS_MIRRORED_REPEAT:                                      \
            switch (sampler & 0xf00)                                           \
            {                                                                  \
                case CLK_FILTER_NEAREST:                                       \
                {                                                              \
                    int4 dim = get_image_dim(image);                           \
                    float4 two = 2.0f;                                         \
                    float4 prim = two * __builtin_ia32_cvtdq2ps(               \
                        __builtin_ia32_cvtps2dq(0.5f * coord));                \
                    prim -= coord;                                             \
                                                                               \
                    /* abs(x) = x & ~{-0, -0, -0, -0} */                       \
                    float4 nzeroes = -0.0f;                                    \
                    prim = (float4)((int4)prim & ~(int4)nzeroes);              \
                                                                               \
                    coord = prim * __builtin_ia32_cvtdq2ps(dim);               \
                    int4 c = f2i_floor(coord);                                 \
                                                                               \
                    /* if (c > dim - 1) c = dim - 1 */                         \
                    int4 repl = dim - 1;                                       \
                    int4 mask = __builtin_ia32_pcmpgtd128(c, repl);            \
                    c = (repl & mask) | (c & ~mask);                           \
                                                                               \
                    return read_image##suf(image, sampler, c);                 \
                }                                                              \
                case CLK_FILTER_LINEAR:                                        \
                {                                                              \
                    type a, b, c;                                              \
                                                                               \
                    int4 dim = get_image_dim(image);                           \
                    float4 two = 2.0f;                                         \
                    float4 prim = two * __builtin_ia32_cvtdq2ps(               \
                        __builtin_ia32_cvtps2dq(0.5f * coord));                \
                    prim -= coord;                                             \
                                                                               \
                    /* abs(x) = x & ~{-0, -0, -0, -0} */                       \
                    float4 nzeroes = -0.0f;                                    \
                    prim = (float4)((int4)prim & ~(int4)nzeroes);              \
                                                                               \
                    coord = prim * __builtin_ia32_cvtdq2ps(dim);               \
                                                                               \
                    float4 tmp = coord;                                        \
                    tmp -= 0.5f;                                               \
                    tmp -= f2f_floor(tmp);                                     \
                                                                               \
                    a = (type)(tmp.x * type_max);                              \
                    b = (type)(tmp.y * type_max);                              \
                    c = (type)(tmp.z * type_max);                              \
                                                                               \
                    int4 V0, V1, zero = 0;                                     \
                                                                               \
                    V0 = f2i_floor(coord - 0.5f);                              \
                    V1 = V0 + 1;                                               \
                                                                               \
                    /* if (0 > V0) V0 = 0 */                                   \
                    int4 mask = __builtin_ia32_pcmpgtd128(V0, zero);           \
                    V0 &= ~mask;                                               \
                                                                               \
                    /* if (V1 > dim - 1) V1 = dim - 1 */                       \
                    int4 repl = dim - 1;                                       \
                    mask = __builtin_ia32_pcmpgtd128(V1, repl);                \
                    V1 = (repl & mask) | (V1 & ~mask);                         \
                                                                               \
                    if (__cpu_is_image_3d(image))                              \
                    {                                                          \
                        result = LINEAR_3D(type_max, suf);                     \
                    }                                                          \
                    else                                                       \
                    {                                                          \
                        result = LINEAR_2D(type_max, suf);                     \
                    }                                                          \
                }                                                              \
            }                                                                  \
            break;                                                             \
    }                                                                          \
                                                                               \
    return result;

float4 OVERLOAD read_imagef(image3d_t image, sampler_t sampler, float4 coord)
{
    READ_IMAGE(float, f, 1.0f)
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
    READ_IMAGE(int, i, 0x7fffffff)
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
    READ_IMAGE(uint, ui, 0xffffffff)
}

#undef UNSWIZZLE_8
#undef UNSWIZZLE_16
#undef UNSWIZZLE_32

void OVERLOAD write_imagef(image2d_t image, int2 coord, float4 color)
{
    __cpu_write_imagef(image, coord.x, coord.y, 0, &color);
}

void OVERLOAD write_imagef(image3d_t image, int4 coord, float4 color)
{
    __cpu_write_imagef(image, coord.x, coord.y, coord.z, &color);
}

void OVERLOAD write_imagei(image2d_t image, int2 coord, int4 color)
{
    __cpu_write_imagei(image, coord.x, coord.y, 0, &color);
}

void OVERLOAD write_imagei(image3d_t image, int4 coord, int4 color)
{
    __cpu_write_imagei(image, coord.x, coord.y, coord.z, &color);
}

void OVERLOAD write_imageui(image2d_t image, int2 coord, uint4 color)
{
   __cpu_write_imageui(image, coord.x, coord.y, 0, &color);
}

void OVERLOAD write_imageui(image3d_t image, int4 coord, uint4 color)
{
    __cpu_write_imageui(image, coord.x, coord.y, coord.z, &color);
}

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
    result.w = 0;

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

