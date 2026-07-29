#include "ethernet_lib/ethernet_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<EthernetTesting>("EthernetTesting", argc, argv);
}
