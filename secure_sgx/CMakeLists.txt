cmake_minimum_required(VERSION 2.8)
project(IoT_TEE)

list(APPEND CMAKE_MODULE_PATH "/${CMAKE_INSTALL_LIBDIR}/cmake")

#include(AwsCFlags)
#include(AwsCheckHeaders)
#include(AwsSharedLibSetup)
#include(AwsSanitizers)
#include(AwsFindPackage)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/lib/cmake")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_PREFIX_PATH "/usr")

#aws_set_common_properties(enclave)

#include_directories(${PahoMqttC_INCLUDE_DIRS})
#link_directories(${PahoMqttC_LIBRARY_DIRS})

#find_package(gRPC REQUIRED)
#find_package(Protobuf REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCe_DIR})
#link_directories(${gRPC_LIBRARY_DIRS} ${Protobuf_LIBRARY_DIRS})

add_executable(enclave enclave.c gateway_client.c transmit.c)
target_link_libraries(enclave aws-nitro-enclaves-sdk-c aws-c-auth aws-c-http aws-c-compression aws-c-io aws-c-sdkutils aws-c-cal aws-c-common json-c s2n m nsm crypto pthread dl)

add_executable(iiot_device iiot_device.c gateway_client.c transmit.c attsdoc.c)
target_link_libraries(iiot_device aws-nitro-enclaves-sdk-c aws-c-auth aws-c-http aws-c-compression aws-c-io aws-c-sdkutils aws-c-cal aws-c-common json-c s2n m nsm crypto pthread dl cbor)

add_executable(gateway gateway.c transmit.c)

install(TARGETS enclave DESTINATION bin)

