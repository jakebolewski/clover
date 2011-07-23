#ifndef __REFCOUNTED_H__
#define __REFCOUNTED_H__

#include <pthread.h>

namespace Coal
{

class RefCounted
{
    public:
        RefCounted();
        ~RefCounted();

        void reference();
        bool dereference();
        unsigned int references() const;

    private:
        unsigned int p_references;
};

}

#endif