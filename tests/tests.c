#include "test_platform.h"
#include "test_device.h"
#include "test_context.h"
#include "test_commandqueue.h"
#include "test_mem.h"
#include "test_kernel.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int n_failed_tests;
    Suite *s = NULL;

    if (argc < 2) {
        printf("test <test> [nofork]\n");
        return EXIT_FAILURE;
    }
    
#define TESTSUITE(name, string)                         \
    if (!strcmp(string, argv[1])) {                     \
        s = suite_create(string);                       \
        suite_add_tcase(s, cl_##name##_tcase_create()); \
    }
    
    TESTSUITE(platform, "platform");
    TESTSUITE(device, "device");
    TESTSUITE(context, "context");
    TESTSUITE(commandqueue, "commandqueue");
    TESTSUITE(mem, "mem");
    TESTSUITE(kernel, "kernel");

    if (s == NULL) {
        printf("test case %s does not exist", argv[1]);
        return EXIT_FAILURE;
    }

    SRunner *sr = srunner_create(s);
    
    if (argc == 3 && !strcmp("nofork", argv[2]))
        srunner_set_fork_status (sr, CK_NOFORK);
    
    srunner_run_all(sr, CK_NORMAL);

    n_failed_tests = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (n_failed_tests == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
