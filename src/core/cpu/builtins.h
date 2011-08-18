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