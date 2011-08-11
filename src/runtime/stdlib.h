/* Types */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int *intptr_t;
typedef uint *uintptr_t;

typedef unsigned int sampler_t;
typedef struct image2d *image2d_t;
typedef struct image3d *image3d_t;

/* Standard types from Clang's stddef, Copyright (C) 2008 Eli Friedman */
typedef __typeof__(((int*)0)-((int*)0)) ptrdiff_t;
typedef __typeof__(sizeof(int)) size_t;

/* Vectors */
#define COAL_VECTOR(type, len)                                  \
   typedef type type##len __attribute__((ext_vector_type(len)))
#define COAL_VECTOR_SET(type) \
   COAL_VECTOR(type, 2);      \
   COAL_VECTOR(type, 3);      \
   COAL_VECTOR(type, 4);      \
   COAL_VECTOR(type, 8);      \
   COAL_VECTOR(type, 16);

COAL_VECTOR_SET(char);
COAL_VECTOR_SET(uchar);

COAL_VECTOR_SET(short);
COAL_VECTOR_SET(ushort);

COAL_VECTOR_SET(int);
COAL_VECTOR_SET(uint);

COAL_VECTOR_SET(long);
COAL_VECTOR_SET(ulong);

COAL_VECTOR_SET(float);

#undef COAL_VECTOR_SET
#undef COAL_VECTOR

/* Address spaces */
#define __private __attribute__((address_space(0)))
#define __global __attribute__((address_space(1)))
#define __local __attribute__((address_space(2)))
#define __constant __attribute__((address_space(3)))

#define global __global
#define local __local
#define constant __constant
#define private __private

#define __write_only
#define __read_only const

#define write_only __write_only
#define read_only __read_only

/* Defines */
#define OVERLOAD __attribute__((overloadable))

#define CLK_NORMALIZED_COORDS_FALSE 0x00000000
#define CLK_NORMALIZED_COORDS_TRUE  0x00000001
#define CLK_ADDRESS_NONE            0x00000000
#define CLK_ADDRESS_MIRRORED_REPEAT 0x00000010
#define CLK_ADDRESS_REPEAT          0x00000020
#define CLK_ADDRESS_CLAMP_TO_EDGE   0x00000030
#define CLK_ADDRESS_CLAMP           0x00000040
#define CLK_FILTER_NEAREST          0x00000000
#define CLK_FILTER_LINEAR           0x00000100

#define CLK_LOCAL_MEM_FENCE         0x00000001
#define CLK_GLOBAL_MEM_FENCE        0x00000002

#define CLK_R                        0x10B0
#define CLK_A                        0x10B1
#define CLK_RG                       0x10B2
#define CLK_RA                       0x10B3
#define CLK_RGB                      0x10B4
#define CLK_RGBA                     0x10B5
#define CLK_BGRA                     0x10B6
#define CLK_ARGB                     0x10B7
#define CLK_INTENSITY                0x10B8
#define CLK_LUMINANCE                0x10B9
#define CLK_Rx                       0x10BA
#define CLK_RGx                      0x10BB
#define CLK_RGBx                     0x10BC

#define CLK_SNORM_INT8               0x10D0
#define CLK_SNORM_INT16              0x10D1
#define CLK_UNORM_INT8               0x10D2
#define CLK_UNORM_INT16              0x10D3
#define CLK_UNORM_SHORT_565          0x10D4
#define CLK_UNORM_SHORT_555          0x10D5
#define CLK_UNORM_INT_101010         0x10D6
#define CLK_SIGNED_INT8              0x10D7
#define CLK_SIGNED_INT16             0x10D8
#define CLK_SIGNED_INT32             0x10D9
#define CLK_UNSIGNED_INT8            0x10DA
#define CLK_UNSIGNED_INT16           0x10DB
#define CLK_UNSIGNED_INT32           0x10DC
#define CLK_HALF_FLOAT               0x10DD
#define CLK_FLOAT                    0x10DE

/* Typedefs */
typedef unsigned int cl_mem_fence_flags;

/* Management functions */
uint get_work_dim();
size_t get_global_size(uint dimindx);
size_t get_global_id(uint dimindx);
size_t get_local_size(uint dimindx);
size_t get_local_id(uint dimindx);
size_t get_num_groups(uint dimindx);
size_t get_group_id(uint dimindx);
size_t get_global_offset(uint dimindx);

void barrier(cl_mem_fence_flags flags);

/* Image functions */
float4 OVERLOAD read_imagef(image2d_t image, sampler_t sampler, int2 coord);
float4 OVERLOAD read_imagef(image3d_t image, sampler_t sampler, int4 coord);
float4 OVERLOAD read_imagef(image2d_t image, sampler_t sampler, float2 coord);
float4 OVERLOAD read_imagef(image3d_t image, sampler_t sampler, float4 coord);
int4 OVERLOAD read_imagei(image2d_t image, sampler_t sampler, int2 coord);
int4 OVERLOAD read_imagei(image3d_t image, sampler_t sampler, int4 coord);
int4 OVERLOAD read_imagei(image2d_t image, sampler_t sampler, float2 coord);
int4 OVERLOAD read_imagei(image3d_t image, sampler_t sampler, float4 coord);
uint4 OVERLOAD read_imageui(image2d_t image, sampler_t sampler, int2 coord);
uint4 OVERLOAD read_imageui(image3d_t image, sampler_t sampler, int4 coord);
uint4 OVERLOAD read_imageui(image2d_t image, sampler_t sampler, float2 coord);
uint4 OVERLOAD read_imageui(image3d_t image, sampler_t sampler, float4 coord);

void OVERLOAD write_imagef(image2d_t image, int2 coord, float4 color);
void OVERLOAD write_imagef(image3d_t image, int4 coord, float4 color);
void OVERLOAD write_imagei(image2d_t image, int2 coord, int4 color);
void OVERLOAD write_imagei(image3d_t image, int4 coord, int4 color);
void OVERLOAD write_imageui(image2d_t image, int2 coord, uint4 color);
void OVERLOAD write_imageui(image3d_t image, int4 coord, uint4 color);

int2 OVERLOAD get_image_dim(image2d_t image);
int4 OVERLOAD get_image_dim(image3d_t image);
int OVERLOAD get_image_width(image2d_t image);
int OVERLOAD get_image_width(image3d_t image);
int OVERLOAD get_image_height(image2d_t image);
int OVERLOAD get_image_height(image3d_t image);
int OVERLOAD get_image_depth(image3d_t image);

int OVERLOAD get_image_channel_data_type(image2d_t image);
int OVERLOAD get_image_channel_data_type(image3d_t image);
int OVERLOAD get_image_channel_order(image2d_t image);
int OVERLOAD get_image_channel_order(image3d_t image);
