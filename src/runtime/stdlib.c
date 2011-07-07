#include "stdlib.h"

/* Global variable that can be used by CPUDevice to store thread-specific data */
__thread void *g_thread_data;

/* Stubs of management functions appending g_thread_data to their arguments */
uint _get_work_dim(void *thread_data);
size_t _get_global_size(void *thread_data, uint dimindx);
size_t _get_global_id(void *thread_data, uint dimindx);
size_t _get_local_size(void *thread_data, uint dimindx);
size_t _get_local_id(void *thread_data, uint dimindx);
size_t _get_num_groups(void *thread_data, uint dimindx);
size_t _get_group_id(void *thread_data, uint dimindx);
size_t _get_global_offset(void *thread_data, uint dimindx);

uint get_work_dim()
{
    return _get_work_dim(g_thread_data);
}

size_t get_global_size(uint dimindx)
{
    return _get_global_size(g_thread_data, dimindx);
}

size_t get_global_id(uint dimindx)
{
    return _get_global_id(g_thread_data, dimindx);
}

size_t get_local_size(uint dimindx)
{
    return _get_local_size(g_thread_data, dimindx);
}

size_t get_local_id(uint dimindx)
{
    return _get_local_id(g_thread_data, dimindx);
}

size_t get_num_groups(uint dimindx)
{
    return _get_num_groups(g_thread_data, dimindx);
}

size_t get_group_id(uint dimindx)
{
    return _get_group_id(g_thread_data, dimindx);
}

size_t get_global_offset(uint dimindx)
{
    return _get_global_offset(g_thread_data, dimindx);
}
