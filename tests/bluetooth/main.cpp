#include "bluetooth_lib/bluetooth_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<BluetoothTesting>("BluetoothTesting", argc, argv);
}
