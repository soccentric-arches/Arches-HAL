#include "storage_lib/storage_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<StorageTesting>("StorageTesting", argc, argv);
}
