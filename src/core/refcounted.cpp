#include "refcounted.h"

using namespace Coal;

RefCounted::RefCounted() : p_references(1)
{
    pthread_mutex_init(&p_mutex, 0);
}

RefCounted::~RefCounted()
{
    pthread_mutex_destroy(&p_mutex);
}

void RefCounted::reference()
{
    pthread_mutex_lock(&p_mutex);
    
    p_references++;
    
    pthread_mutex_unlock(&p_mutex);
}

bool RefCounted::dereference()
{
    bool rs;

    pthread_mutex_lock(&p_mutex);

    p_references--;
    rs = (p_references == 0);

    pthread_mutex_unlock(&p_mutex);

    return rs;
}

unsigned int RefCounted::references()
{
    unsigned int rs;

    pthread_mutex_lock(&p_mutex);

    rs = p_references;

    pthread_mutex_unlock(&p_mutex);

    return rs;
}
