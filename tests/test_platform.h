#ifndef __UTEST_PLATFORM__
#define __UTEST_PLATFORM__

#include <check.h>

#ifdef __cplusplus
extern "C" {
#endif

TCase *cl_platform_tcase_create(void);

#ifdef __cplusplus
}
#endif

#endif

