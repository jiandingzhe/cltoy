                        cmake_minimum_required(VERSION 3.0)
project(cltoy)

if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Debug")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(CL REQUIRED)

include_directories(${CL_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})

add_library(utils STATIC
    utils.h
    utils.cpp
    htio2/Cast.h
    htio2/Cast.cpp
    htio2/OptionParser.h
    htio2/OptionParser.cpp
    htio2/RefCounted.h
    htio2/RefCounted.cpp
    htio2/StringUtil.h
    htio2/StringUtil.cpp)
if(NOT MSVC)
    target_compile_options(utils PUBLIC -std=c++11)
endif()

foreach(exec_name
    show_plat_dev
    buffer_delay
)
    add_executable(${exec_name} ${exec_name}.cpp)
    target_link_libraries(${exec_name}
        utils
        ${CL_LIBRARIES}
    )

    if(NOT MSVC)
        target_compile_options(${exec_name} PUBLIC -std=c++11 -mavx -ffast-math)
    endif()

endforeach()

add_executable(simple_tri simple_tri.cpp)


