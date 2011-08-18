#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <string>

namespace Coal {
    class CPUKernelWorkGroup;
}

void setThreadLocalWorkGroup(Coal::CPUKernelWorkGroup *current);
void *getBuiltin(const std::string &name);
void *getWorkItemsData(size_t &size);
void setWorkItemsData(void *ptr, size_t size);

template<typename T>
bool incVec(unsigned long dims, T *vec, T *maxs)
{
    bool overflow = false;

    for (unsigned int i=0; i<dims; ++i)
    {
        vec[i] += 1;

        if (vec[i] > maxs[i])
        {
            vec[i] = 0;
            overflow = true;
        }
        else
        {
            overflow = false;
            break;
        }
    }

    return overflow;
}

unsigned char *imageData(unsigned char *base, size_t x, size_t y, size_t z,
                         size_t row_pitch, size_t slice_pitch,
                         unsigned int bytes_per_pixel);

#endif