#include "external_connector_lib/external_connector_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<ExternalConnectorTesting>("ExternalConnectorTesting", argc,
                                              argv);
}
