include(FindPackageMessage)
include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

set(CL_SEARCH_PREFIX "" CACHE PATH "Additional search path for OpenCL.")

#
# header dir
#
find_path(CL_INCLUDE_DIRS
    NAMES
        CL/cl.h
        CL/opencl.h
    HINTS
        "${CL_SEARCH_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}"
        "${CL_SEARCH_PREFIX}/include"
)

find_library(CL_LIBRARIES
    NAMES
        OpenCL
    HINTS
        "${CL_SEARCH_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        "${CL_SEARCH_PREFIX}/lib"
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CL
    REQUIRED_VARS
        CL_INCLUDE_DIRS
        CL_LIBRARIES
)
