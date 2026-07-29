#include "serializer_deserializer_lib/serializer_deserializer_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SerializerDeserializerTesting>(
      "SerializerDeserializerTesting", argc, argv);
}
