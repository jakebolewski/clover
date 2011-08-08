#include "CL/cl.h"
#include "core/commandqueue.h"

// Flush and Finish APIs
cl_int
clFlush(cl_command_queue command_queue)
{
    if (!command_queue->isA(Coal::Object::T_CommandQueue))
        return CL_INVALID_COMMAND_QUEUE;

    command_queue->flush();

    return CL_SUCCESS;
}

cl_int
clFinish(cl_command_queue command_queue)
{
    if (!command_queue->isA(Coal::Object::T_CommandQueue))
        return CL_INVALID_COMMAND_QUEUE;

    command_queue->finish();

    return CL_SUCCESS;
}
