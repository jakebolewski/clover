#include "object.h"

using namespace Coal;

std::list<Object *> known_objects;

Object::Object(Type type, Object *parent)
: p_references(1), p_parent(parent), p_type(type), p_release_parent(true)
{
    if (parent)
        parent->reference();

    // Add object in the list of known objects
    known_objects.push_front(this);
    p_it = known_objects.begin();
}

Object::~Object()
{
    if (p_parent && p_parent->dereference() && p_release_parent)
        delete p_parent;

    // Remove object from the list of known objects
    known_objects.erase(p_it);
}

void Object::reference()
{
    p_references++;
}

bool Object::dereference()
{
    p_references--;
    return (p_references == 0);
}

void Object::setReleaseParent (bool release)
{
    p_release_parent = release;
}

unsigned int Object::references() const
{
    return p_references;
}

Object *Object::parent() const
{
    return p_parent;
}

Object::Type Object::type() const
{
    return p_type;
}

bool Object::isA(Object::Type type) const
{
    // Check for null values
    if (this == 0)
        return false;

    // Check that the value isn't garbage or freed pointer
    std::list<Object *>::const_iterator it = known_objects.begin(),
                                        e = known_objects.end();
    while (it != e)
    {
        if (*it == this)
            return this->type() == type;

        ++it;
    }

    return false;
}
