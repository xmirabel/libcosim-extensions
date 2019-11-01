#include <cse.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WINDOWS
#    include <windows.h>
#else
#    include <unistd.h>
#    define Sleep(x) usleep((x)*1000)
#endif

#define MAX_NUMBER_OF_SLAVES 10

void print_last_error()
{
    fprintf(
        stderr,
        "Error code %d: %s\n",
        cse_last_error_code(), cse_last_error_message());
}

int main()
{
    cse_log_setup_simple_console_logging();
    cse_log_set_output_level(CSE_LOG_SEVERITY_INFO);

    int exitCode = 0;
    cse_execution* execution = NULL;
    cse_observer* observer = NULL;

    const char* dataDir = getenv("TEST_DATA_DIR");
    if (!dataDir) {
        fprintf(stderr, "Environment variable TEST_DATA_DIR not set\n");
        goto Lfailure;
    }

    char msmiDir[1024];
    int rc = snprintf(msmiDir, sizeof msmiDir, "%s/msmi/OspSystemStructure.xml", dataDir);
    if (rc < 0) {
        perror(NULL);
        goto Lfailure;
    }

    execution = cse_config_execution_create(msmiDir, false, 0);
    if (!execution) { goto Lerror; }

    observer = cse_last_value_observer_create();
    if (!observer) { goto Lerror; }
    cse_execution_add_observer(execution, observer);

    rc = cse_execution_step(execution, 3);
    if (rc < 0) { goto Lerror; }

    size_t numSlaves = cse_execution_get_num_slaves(execution);
    if (numSlaves > MAX_NUMBER_OF_SLAVES) {
        printf("Number of slaves in configuration exceeds max number of slaves(%d) supported in test", MAX_NUMBER_OF_SLAVES);
        goto Lfailure;
    }

    cse_slave_info infos[MAX_NUMBER_OF_SLAVES];
    rc = cse_execution_get_slave_infos(execution, &infos[0], numSlaves);
    if (rc < 0) { goto Lerror; }

    for (size_t i = 0; i < numSlaves; i++) {
        if (0 == strncmp(infos[i].name, "KnuckleBoomCrane", SLAVE_NAME_MAX_SIZE)) {
            double value = -1;
            cse_slave_index slaveIndex = infos[i].index;
            cse_value_reference varIndex = 2;
            rc = cse_observer_slave_get_real(observer, slaveIndex, &varIndex, 1, &value);
            if (rc < 0) {
                goto Lerror;
            }
            if (value != 0.05) {
                fprintf(stderr, "Expected value 0.05, got %f\n", value);
                goto Lfailure;
            }
        }
    }

    cse_execution_start(execution);
    Sleep(200);
    cse_execution_stop(execution);


    goto Lcleanup;

Lerror:
    print_last_error();

Lfailure:
    exitCode = 1;

Lcleanup:
    cse_observer_destroy(observer);
    cse_execution_destroy(execution);
    return exitCode;
}