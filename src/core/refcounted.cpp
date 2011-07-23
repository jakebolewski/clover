#include "refcounted.h"

using namespace Coal;

RefCounted::RefCounted() : p_references(1)
{
}

RefCounted::~RefCounted()
{
}

void RefCounted::reference()
{
    p_references++;
}

bool RefCounted::dereference()
{
    p_references--;
    return (p_references == 0);
}

unsigned int RefCounted::references() const
{
    return p_references;
}
