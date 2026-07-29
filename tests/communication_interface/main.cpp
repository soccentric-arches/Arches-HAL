#include "communication_interface_lib/communication_interface_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<CommunicationInterfaceTesting>(
      "CommunicationInterfaceTesting", argc, argv);
}
