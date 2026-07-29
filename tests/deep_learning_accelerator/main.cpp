#include "deep_learning_accelerator_lib/deep_learning_accelerator_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<DeepLearningAcceleratorTesting>(
      "DeepLearningAcceleratorTesting", argc, argv);
}
