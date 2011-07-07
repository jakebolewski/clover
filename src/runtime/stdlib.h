/* Types */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long size_t;   // NOTE: Doesn't match the spec
typedef signed long ptrdiff_t;
typedef int *intptr_t;
typedef uint *uintptr_t;

/* Vectors */

//typedef float float4 __attribute__((ext_vector_type(4)));
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
#define __global __attribute__((address_space(1)))
#define __local __attribute__((address_space(2)))
#define __constant __attribute__((address_space(3)))
#define __private __attribute__((address_space(4)))

#define global __global
#define local __local
#define constant __constant
#define private __private

#define __write_only
#define __read_only const

#define write_only __write_only
#define read_only __read_only

/* Management functions */
uint get_work_dim();
size_t get_global_size(uint dimindx);
size_t get_global_id(uint dimindx);
size_t get_local_size(uint dimindx);
size_t get_local_id(uint dimindx);
size_t get_num_groups(uint dimindx);
size_t get_group_id(uint dimindx);
size_t get_global_offset(uint dimindx);
