/* Types */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int *intptr_t;
typedef uint *uintptr_t;

typedef unsigned int sampler_t;

/* Standard types from Clang's stddef, Copyright (C) 2008 Eli Friedman */
typedef __typeof__(((int*)0)-((int*)0)) ptrdiff_t;
typedef __typeof__(sizeof(int)) size_t;

/* Vectors */
#define COAL_VECTOR(type, len)                                  \
   typedef type type##len __attribute__((ext_vector_type(len)))
#define COAL_VECTOR_SET(type) \
   COAL_VECTOR(type, 2);      \
   COAL_VECTOR(type, 3);      \
   COAL_VECTOR(type, 4)

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
#define CLK_NORMALIZED_COORDS_FALSE 0x00000000
#define CLK_NORMALIZED_COORDS_TRUE  0x00000001
#define CLK_ADDRESS_NONE            0x00000000
#define CLK_ADDRESS_MIRRORED_REPEAT 0x00000010
#define CLK_ADDRESS_REPEAT          0x00000020
#define CLK_ADDRESS_CLAMP_TO_EDGE   0x00000030
#define CLK_ADDRESS_CLAMP           0x00000040
#define CLK_FILTER_NEAREST          0x00000000
#define CLK_FILTER_LINEAR           0x00000100

/* Management functions */
uint get_work_dim();
size_t get_global_size(uint dimindx);
size_t get_global_id(uint dimindx);
size_t get_local_size(uint dimindx);
size_t get_local_id(uint dimindx);
size_t get_num_groups(uint dimindx);
size_t get_group_id(uint dimindx);
size_t get_global_offset(uint dimindx);
