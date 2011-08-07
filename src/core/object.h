#ifndef __REFCOUNTED_H__
#define __REFCOUNTED_H__

#include <list>

namespace Coal
{

class Object
{
    public:
        enum Type
        {
            T_CommandQueue,
            T_Event,
            T_Context,
            T_Kernel,
            T_MemObject,
            T_Program,
            T_Sampler
        };

        Object(Type type, Object *parent = 0);
        virtual ~Object();

        void reference();
        bool dereference();
        unsigned int references() const;
        void setReleaseParent(bool release);

        Object *parent() const;
        Type type() const;
        bool isA(Type type) const;

    private:
        unsigned int p_references;
        Object *p_parent;
        Type p_type;
        std::list<Object *>::iterator p_it;
        bool p_release_parent;
};

}

#endif