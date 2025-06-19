cmake_minimum_required(VERSION 3.15)

project(DiskNode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gen")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(GENERATED_SOURCES
    ${GEN_DIR}/tec_mfs.pb.cc
    ${GEN_DIR}/tec_mfs.grpc.pb.cc
)

set(GENERATED_HEADERS
    ${GEN_DIR}/tec_mfs.pb.h
    ${GEN_DIR}/tec_mfs.grpc.pb.h
)

file(GLOB_RECURSE PROJECT_SOURCES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/**/*.cpp"
)

file(GLOB_RECURSE PROJECT_HEADERS
    "${INCLUDE_DIR}/*.h"
    "${INCLUDE_DIR}/**/*.h"
)

add_executable(DiskNode 
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${GENERATED_SOURCES}
    ${GENERATED_HEADERS}
)

target_include_directories(DiskNode
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/DiskNode
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ConfigLoader
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/DiskNode
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigLoader
        ${CMAKE_CURRENT_SOURCE_DIR}/gen
)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

target_link_libraries(DiskNode
	gRPC::grpc++
	protobuf::libprotobuf
    tinyxml2::tinyxml2 
)

