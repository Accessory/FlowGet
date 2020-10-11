cmake_minimum_required(VERSION 3.4)
project(FlowGet)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../Flow)
include_directories(..)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(FlowGet main.cpp)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.66.0 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(FlowGet ${Boost_LIBRARIES})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(FlowGet ${OPENSSL_LIBRARIES})

find_package(Threads)
target_link_libraries(FlowGet ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(FlowGet FlowHttp_Core)

find_package(Brotli)
target_link_libraries(FlowGet Brotli::brotlienc Brotli::brotlicommon)

if (WIN32)
    target_link_libraries(FlowGet ws2_32)
    target_link_libraries(FlowGet wsock32)
    target_link_libraries(FlowGet bcrypt)
endif ()

if (UNIX)
    install(TARGETS FlowGet
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            INCLUDES DESTINATION include
            )
endif ()