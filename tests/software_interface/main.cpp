#include "software_interface_lib/software_interface_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SoftwareInterfaceTesting>("SoftwareInterfaceTesting", argc,
                                              argv);
}
