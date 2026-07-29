#include "ethernet_switch_lib/ethernet_switch_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<EthernetSwitchTesting>("EthernetSwitchTesting", argc, argv);
}
