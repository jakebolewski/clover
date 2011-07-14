#include "kernel.h"

__thread Coal::CPUKernelWorkGroup *g_work_group;

void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current)
{
    g_work_group = current;
}

void *getBuiltin(const std::string &name)
{
    return 0;
}
