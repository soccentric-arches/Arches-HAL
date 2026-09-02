#include "sd_card_lib/sd_card_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SdCardTesting>("SdCardTesting", argc, argv);
}
