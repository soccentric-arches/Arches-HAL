# protos/CMakeLists.txt
#
# Full Protobuf + gRPC build for the HAL proto layer. Both Protobuf and gRPC are
# REQUIRED — the build fails fast if either is absent.
#
# Produced targets
# ─────────────────────────────────────────────────────────────
# hal_protos          STATIC  generated protobuf/gRPC C++ hal_proto_<module>
# STATIC/SHARED generated module contract libraries
#
# Generated header include paths (propagated automatically): #include
# "common.pb.h" #include "hal_service.grpc.pb.h" #include "modules/cpu.pb.h"

cmake_minimum_required(VERSION 3.10)

# ============================================================
# 1. Protobuf — REQUIRED
# ============================================================
find_package(Protobuf REQUIRED)
message(
  STATUS "[hal_protos] protobuf ${Protobuf_VERSION}  (${Protobuf_INCLUDE_DIRS})"
)
message(STATUS "[hal_protos] protoc   ${Protobuf_PROTOC_EXECUTABLE}")

find_library(
  HAL_PROTOBUF_SHARED_LIBRARY
  NAMES protobuf
  PATHS "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" /usr/lib
  NO_DEFAULT_PATH)
if(NOT HAL_PROTOBUF_SHARED_LIBRARY)
  message(
    FATAL_ERROR
      "[hal_protos] A shared libprotobuf is required for shared proto libraries"
  )
endif()
message(STATUS "[hal_protos] shared protobuf ${HAL_PROTOBUF_SHARED_LIBRARY}")

# ============================================================
# 1. gRPC — REQUIRED (CMake config preferred; pkg-config fallback)
# ============================================================
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
  message(STATUS "[hal_protos] gRPC ${gRPC_VERSION} via CMake config")
  set(HAL_GRPC_LIBRARIES gRPC::grpc++ gRPC::grpc)
  get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GRPCPP REQUIRED grpc++)
  message(STATUS "[hal_protos] gRPC via pkg-config")
  set(HAL_GRPC_LIBRARIES ${GRPCPP_LIBRARIES})
  find_program(
    GRPC_CPP_PLUGIN grpc_cpp_plugin
    HINTS /usr/local/bin /usr/bin /usr/lib/grpc /usr/lib/aarch64-linux-gnu
          /usr/lib/x86_64-linux-gnu ${GRPCPP_PREFIX}/bin
    DOC "Path to the grpc_cpp_plugin protoc plugin")
  if(NOT GRPC_CPP_PLUGIN)
    message(
      FATAL_ERROR
        "[hal_protos] grpc_cpp_plugin not found.\n"
        "Rebuild the Docker image (the Dockerfile now builds the plugin from source), "
        "or set -DGRPC_CPP_PLUGIN=/path/to/grpc_cpp_plugin on the cmake command line."
    )
  endif()
endif()

message(STATUS "[hal_protos] grpc_cpp_plugin: ${GRPC_CPP_PLUGIN}")

# ============================================================
# 1. Output directories for generated sources
# ============================================================
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_BINARY_DIR}")
file(MAKE_DIRECTORY "${PROTO_BINARY_DIR}/modules")

# ============================================================
# 1. Collect proto files
# ============================================================
set(COMMON_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/common.proto")
set(SERVICE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/hal_service.proto")
set(CLIENT_SERVICE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/client_service.proto")
file(GLOB MODULE_PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.proto")
set(ALL_PROTOS ${COMMON_PROTO} ${SERVICE_PROTO} ${CLIENT_SERVICE_PROTO}
               ${MODULE_PROTOS})

# ============================================================
# 1. Generate protobuf C++ sources for every proto file
# ============================================================
set(PROTO_SRCS "")
set(PROTO_HDRS "")

foreach(PROTO_FILE IN LISTS ALL_PROTOS)
  file(RELATIVE_PATH PROTO_REL "${CMAKE_CURRENT_SOURCE_DIR}" "${PROTO_FILE}")
  get_filename_component(PROTO_REL_DIR "${PROTO_REL}" DIRECTORY)
  get_filename_component(PROTO_NAME_WE "${PROTO_FILE}" NAME_WE)

  if(PROTO_REL_DIR)
    set(OUT_SUBDIR "${PROTO_BINARY_DIR}/${PROTO_REL_DIR}")
  else()
    set(OUT_SUBDIR "${PROTO_BINARY_DIR}")
  endif()
  file(MAKE_DIRECTORY "${OUT_SUBDIR}")

  set(PROTO_SRC "${OUT_SUBDIR}/${PROTO_NAME_WE}.pb.cc")
  set(PROTO_HDR "${OUT_SUBDIR}/${PROTO_NAME_WE}.pb.h")

  add_custom_command(
    OUTPUT "${PROTO_SRC}" "${PROTO_HDR}"
    COMMAND
      "${Protobuf_PROTOC_EXECUTABLE}" "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
      "--proto_path=${Protobuf_INCLUDE_DIRS}" "--cpp_out=${PROTO_BINARY_DIR}"
      "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}" "${COMMON_PROTO}"
    COMMENT "[protoc pb]   ${PROTO_REL}"
    VERBATIM)
  list(APPEND PROTO_SRCS "${PROTO_SRC}")
  list(APPEND PROTO_HDRS "${PROTO_HDR}")
endforeach()

# ============================================================
# 1. Generate gRPC stubs for the HAL and client service contracts
# ============================================================
set(GRPC_SRC "${PROTO_BINARY_DIR}/hal_service.grpc.pb.cc")
set(GRPC_HDR "${PROTO_BINARY_DIR}/hal_service.grpc.pb.h")

add_custom_command(
  OUTPUT "${GRPC_SRC}" "${GRPC_HDR}"
  COMMAND
    "${Protobuf_PROTOC_EXECUTABLE}" "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
    "--proto_path=${Protobuf_INCLUDE_DIRS}" "--grpc_out=${PROTO_BINARY_DIR}"
    "--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}" "${SERVICE_PROTO}"
  DEPENDS "${SERVICE_PROTO}" "${COMMON_PROTO}"
  COMMENT "[protoc grpc] hal_service.proto"
  VERBATIM)

set(CLIENT_GRPC_SRC "${PROTO_BINARY_DIR}/client_service.grpc.pb.cc")
set(CLIENT_GRPC_HDR "${PROTO_BINARY_DIR}/client_service.grpc.pb.h")

add_custom_command(
  OUTPUT "${CLIENT_GRPC_SRC}" "${CLIENT_GRPC_HDR}"
  COMMAND
    "${Protobuf_PROTOC_EXECUTABLE}" "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
    "--proto_path=${Protobuf_INCLUDE_DIRS}" "--grpc_out=${PROTO_BINARY_DIR}"
    "--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}" "${CLIENT_SERVICE_PROTO}"
  DEPENDS "${CLIENT_SERVICE_PROTO}" "${COMMON_PROTO}"
  COMMENT "[protoc grpc] client_service.proto"
  VERBATIM)

# ============================================================
# 1. hal_protos — generated protobuf + gRPC sources
# ============================================================
add_library(hal_protos STATIC ${PROTO_SRCS} ${GRPC_SRC} ${CLIENT_GRPC_SRC})
add_library(hal_protos_shared SHARED ${PROTO_SRCS} ${GRPC_SRC}
                                     ${CLIENT_GRPC_SRC})

# Suppress warnings in protoc-generated code.
target_compile_options(hal_protos PRIVATE $<$<CXX_COMPILER_ID:GNU>: -w>
                                          $<$<CXX_COMPILER_ID:Clang>: -w>)
target_compile_options(hal_protos_shared PRIVATE $<$<CXX_COMPILER_ID:GNU>:-w>
                                                 $<$<CXX_COMPILER_ID:Clang>:-w>)

target_include_directories(
  hal_protos
  PUBLIC "${PROTO_BINARY_DIR}" # #include "common.pb.h", "modules/cpu.pb.h"
         "${Protobuf_INCLUDE_DIRS}" # google/protobuf/*.h
)
target_include_directories(hal_protos_shared PUBLIC "${PROTO_BINARY_DIR}"
                                                    "${Protobuf_INCLUDE_DIRS}")

target_link_libraries(hal_protos PUBLIC protobuf::libprotobuf
                                        ${HAL_GRPC_LIBRARIES})
target_link_libraries(hal_protos_shared PUBLIC "${HAL_PROTOBUF_SHARED_LIBRARY}"
                                               ${HAL_GRPC_LIBRARIES})

# ============================================================
# 1. Per-module generated proto libraries
# ============================================================
add_library(hal_proto_common STATIC "${PROTO_BINARY_DIR}/common.pb.cc")
add_library(hal_proto_common_shared SHARED "${PROTO_BINARY_DIR}/common.pb.cc")

foreach(PROTO_FILE IN LISTS MODULE_PROTOS)
  get_filename_component(PROTO_NAME_WE "${PROTO_FILE}" NAME_WE)
  set(MODULE_PROTO_SRC "${PROTO_BINARY_DIR}/modules/${PROTO_NAME_WE}.pb.cc")

  add_library("hal_proto_${PROTO_NAME_WE}" STATIC "${MODULE_PROTO_SRC}")
  add_library("hal_proto_${PROTO_NAME_WE}_shared" SHARED "${MODULE_PROTO_SRC}")

  foreach(PROTO_TARGET "hal_proto_${PROTO_NAME_WE}"
                       "hal_proto_${PROTO_NAME_WE}_shared")
    target_include_directories(
      "${PROTO_TARGET}" PUBLIC "${PROTO_BINARY_DIR}" "${Protobuf_INCLUDE_DIRS}")
    target_compile_options(
      "${PROTO_TARGET}" PRIVATE $<$<CXX_COMPILER_ID:GNU>:-w>
                                $<$<CXX_COMPILER_ID:Clang>:-w>)
  endforeach()

  target_link_libraries("hal_proto_${PROTO_NAME_WE}" PUBLIC hal_proto_common)
  target_link_libraries("hal_proto_${PROTO_NAME_WE}_shared"
                        PUBLIC hal_proto_common_shared)

  install(
    TARGETS "hal_proto_${PROTO_NAME_WE}" "hal_proto_${PROTO_NAME_WE}_shared"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach()

target_include_directories(hal_proto_common PUBLIC "${PROTO_BINARY_DIR}"
                                                   "${Protobuf_INCLUDE_DIRS}")
target_include_directories(
  hal_proto_common_shared PUBLIC "${PROTO_BINARY_DIR}"
                                 "${Protobuf_INCLUDE_DIRS}")
target_link_libraries(hal_proto_common PUBLIC protobuf::libprotobuf)
target_link_libraries(hal_proto_common_shared
                      PUBLIC "${HAL_PROTOBUF_SHARED_LIBRARY}")
target_compile_options(hal_proto_common PRIVATE $<$<CXX_COMPILER_ID:GNU>:-w>
                                                $<$<CXX_COMPILER_ID:Clang>:-w>)
target_compile_options(
  hal_proto_common_shared PRIVATE $<$<CXX_COMPILER_ID:GNU>:-w>
                                  $<$<CXX_COMPILER_ID:Clang>:-w>)

install(
  TARGETS hal_protos hal_proto_common hal_proto_common_shared
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY "${PROTO_BINARY_DIR}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hal_protos"
  FILES_MATCHING
  PATTERN "*.pb.h")
