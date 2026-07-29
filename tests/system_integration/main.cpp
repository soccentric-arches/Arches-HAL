#include "system_integration_lib/system_integration_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SystemIntegrationTesting>("SystemIntegrationTesting", argc,
                                              argv);
}
