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
 *     * Neither the name of the copyright holderi nor the
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

#ifndef __MEMOBJECT_H__
#define __MEMOBJECT_H__

#include "object.h"

#include <CL/cl.h>

namespace Coal
{

class DeviceBuffer;
class Context;
class DeviceInterface;

class MemObject : public Object
{
    public:
        enum Type
        {
            Buffer,
            SubBuffer,
            Image2D,
            Image3D
        };

        /**
         * @note Doesn't do any initialization here, but in init. We only fill
         *       the private variables and check the values passed in argument.
         * @sa init
         */
        MemObject(Context *ctx, cl_mem_flags flags, void *host_ptr,
                  cl_int *errcode_ret);
        ~MemObject();

        virtual cl_int init();
        virtual bool allocate(DeviceInterface *device);
        virtual size_t size() const = 0; /*!< @warning this is a device-independent size */
        virtual Type type() const = 0;

        cl_mem_flags flags() const;
        void *host_ptr() const;
        DeviceBuffer *deviceBuffer(DeviceInterface *device) const;

        void deviceAllocated(DeviceBuffer *buffer); /*!< @note called by the DeviceBuffers */

        void setDestructorCallback(void (CL_CALLBACK *pfn_notify)(cl_mem memobj,
                                                             void *user_data),
                                   void *user_data);
        cl_int info(cl_mem_info param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret) const;

    private:
        unsigned int p_num_devices, p_devices_to_allocate;
        cl_mem_flags p_flags;
        void *p_host_ptr;
        DeviceBuffer **p_devicebuffers;

        void (CL_CALLBACK *p_dtor_callback)(cl_mem memobj, void *user_data);
        void *p_dtor_userdata;
};

class Buffer : public MemObject
{
    public:
        Buffer(Context *ctx, size_t size, void *host_ptr, cl_mem_flags flags,
               cl_int *errcode_ret);

        size_t size() const;
        Type type() const;
    private:
        size_t p_size;

};

class SubBuffer : public MemObject
{
    public:
        SubBuffer(class Buffer *parent, size_t offset, size_t size,
                  cl_mem_flags flags, cl_int *errcode_ret);

        size_t size() const;
        Type type() const;
        bool allocate(DeviceInterface *device);

        size_t offset() const;
        class Buffer *parent() const;

    private:
        size_t p_offset, p_size;
        class Buffer *p_parent;
};

class Image2D : public MemObject
{
    public:
        Image2D(Context *ctx, size_t width, size_t height, size_t row_pitch,
                const cl_image_format *format, void *host_ptr,
                cl_mem_flags flags, cl_int *errcode_ret);

        virtual size_t size() const;
        virtual Type type() const;

        size_t width() const;
        size_t height() const;
        size_t row_pitch() const;
        virtual size_t slice_pitch() const;
        const cl_image_format &format() const;

        cl_int imageInfo(cl_image_info param_name,
                         size_t param_value_size,
                         void *param_value,
                         size_t *param_value_size_ret) const;

        static size_t element_size(const cl_image_format &format);
        static size_t pixel_size(const cl_image_format &format);
        size_t pixel_size() const;

    private:
        size_t p_width, p_height, p_row_pitch;
        cl_image_format p_format;
};

class Image3D : public Image2D
{
    public:
        Image3D(Context *ctx, size_t width, size_t height, size_t depth,
                size_t row_pitch, size_t slice_pitch,
                const cl_image_format *format, void *host_ptr,
                cl_mem_flags flags, cl_int *errcode_ret);

        size_t size() const;
        Type type() const;

        size_t depth() const;
        size_t slice_pitch() const;

    private:
        size_t p_depth, p_slice_pitch;
};

}

struct _cl_mem : public Coal::MemObject
{};

#endif
