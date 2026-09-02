#include "rs485_lib/rs485_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<Rs485Testing>("Rs485Testing", argc, argv);
}
