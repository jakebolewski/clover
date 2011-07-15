#include "kernel.h"

__thread Coal::CPUKernelWorkGroup *g_work_group;

size_t get_global_id(cl_uint dimindx)
{
    return g_work_group->getGlobalId(dimindx);
}

void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current)
{
    g_work_group = current;
}

void *getBuiltin(const std::string &name)
{
    if (name == "get_global_id")
        return (void *)&get_global_id;

    return 0;
}
