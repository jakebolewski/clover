#include "kernel.h"

__thread Coal::CPUKernelWorkGroup *g_work_group;

static size_t get_global_id(cl_uint dimindx)
{
    return g_work_group->getGlobalId(dimindx);
}

static cl_uint get_work_dim()
{
    return g_work_group->getWorkDim();
}

static size_t get_global_size(uint dimindx)
{
    return g_work_group->getGlobalSize(dimindx);
}

static size_t get_local_size(uint dimindx)
{
    return g_work_group->getLocalSize(dimindx);
}

static size_t get_local_id(uint dimindx)
{
    return g_work_group->getLocalID(dimindx);
}

static size_t get_num_groups(uint dimindx)
{
    return g_work_group->getNumGroups(dimindx);
}

static size_t get_group_id(uint dimindx)
{
    return g_work_group->getGroupID(dimindx);
}

static size_t get_global_offset(uint dimindx)
{
    return g_work_group->getGlobalOffset(dimindx);
}

/*
 * Utility functions
 */
void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current)
{
    g_work_group = current;
}

static void unimplemented_stub()
{
}

void *getBuiltin(const std::string &name)
{
    if (name == "get_global_id")
        return (void *)&get_global_id;
    else if (name == "get_work_dim")
        return (void *)&get_work_dim;
    else if (name == "get_global_size")
        return (void *)&get_global_size;
    else if (name == "get_local_size")
        return (void *)&get_local_size;
    else if (name == "get_local_id")
        return (void *)&get_local_id;
    else if (name == "get_num_groups")
        return (void *)&get_num_groups;
    else if (name == "get_group_id")
        return (void *)&get_group_id;
    else if (name == "get_global_offset")
        return (void *)&get_global_offset;

    // Function not found
    g_work_group->builtinNotFound(name);

    return (void *)&unimplemented_stub;
}
