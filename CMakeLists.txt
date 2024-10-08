# dmrosen 4-July-2022

cmake_minimum_required(VERSION 3.19)

# PROJECT CONFIGURATION
project(Preconditioners LANGUAGES C CXX VERSION 1.0.0)
# Conan sets the standard
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON) # We require C++ 17 or later


# Set build type to 'RelWithDebInfo' if one was not specified by the user
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS RelWithDebInfo Release Debug MinSizeRel)
  message(STATUS "Setting build type to ${CMAKE_BUILD_TYPE}, as none was specified\n")
else()
  message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode\n")
endif()


# Build the unit tests?
set(BUILD_PRECONDITIONERS_TESTS OFF CACHE BOOL "Build unit tests? [disabled by default]")

# Directory for built libraries
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib CACHE PATH "The directory in which to place the libraries built by this project")
# Directory for built executables
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "The directory in which to place executables built by this project")

# BUILD CONFIGURATIONS
# Enable faster instruction sets (SIMD/AVX)
set(ENABLE_VECTORIZATION OFF CACHE BOOL "Enable vectorized instruction sets (SIMD/AVX)? [disabled by default]")
# Enable code profiling using gperftools
set(ENABLE_PROFILING OFF CACHE BOOL "Enable code profiling using gperftools")
set(ENABLE_SUITESPARSE OFF CACHE BOOL  "Enable SuiteSparse (disable for LGPL complince)")

# Add the .cmake files that ship with Eigen3 to the CMake module path (useful for finding other stuff)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE STRING "The CMake module path used for this project")


# PRECONDITIONERS DEPENDENCIES
# Find Eigen library
find_package(Eigen3 REQUIRED)
if(EIGEN3_FOUND)
message(STATUS "Found Eigen3 library: ${EIGEN3_INCLUDE_DIR}")
endif()

# FIND ADDITIONAL LIBRARIES
# These next operations make use of the .cmake files shipped with Eigen3
find_package(OpenMP REQUIRED)

# Set location of the include directory for the SYM-ILDL library
set(SymILDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SymILDL/SymILDL)
file(GLOB SymILDL_HDRS  " ${CMAKE_CURRENT_SOURCE_DIR}/SymILDL/SymILDL/*.h")

# Set the various directories for this project
set(ILDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(ILDL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/ILDL)

# Get the set of ILDL header and source files
set(ILDL_HDRS
${ILDL_INCLUDE_DIR}/ILDL/ILDL_utils.h
${ILDL_INCLUDE_DIR}/ILDL/ILDL.h
${ILDL_INCLUDE_DIR}/Preconditioners/Types.h
)

set(ILDL_SRCS
${ILDL_SOURCE_DIR}/ILDL_utils.cpp
${ILDL_SOURCE_DIR}/ILDL.cpp
)

add_library(ILDL SHARED)

target_sources(ILDL
    PRIVATE   ${ILDL_SRCS}
    PUBLIC FILE_SET HEADERS 
    BASE_DIRS ${ILDL_INCLUDE_DIR}
    FILES ${ILDL_HDRS}

    )
set_target_properties(ILDL PROPERTIES PUBLIC_HEADER ${ILDL_HDRS})
target_include_directories(ILDL PRIVATE ${SymILDL_INCLUDE_DIR})
target_link_libraries(ILDL PUBLIC Eigen3::Eigen ${BLAS_LIBRARIES})



if(${ENABLE_SUITESPARSE})
    # SET UP LSChol LIBRARY
    
    # Set the various directories for this project
    set(LSChol_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Preconditioners/LSChol/include)
    set(LSChol_HDR_DIR ${LSChol_INCLUDE_DIR}/LSChol)
    set(LSChol_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Preconditioners/LSChol/src)
    
    # Get the set of LSChol header and source files
    set(LSChol_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/Preconditioners/Types.h
    ${LSChol_HDR_DIR}/LSChol.h
    )
    
    set(LSChol_SRCS
    ${LSChol_SOURCE_DIR}/LSChol.cpp
    )
    
    # Eigen 3 is referenced by the header files of the LSChol library, hence must be PUBLICLY included (i.e. clients using the SYMILDLSupport headers must also include these headers)
    set(LSChol_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${EIGEN3_INCLUDE_DIR} ${LSChol_INCLUDE_DIR} CACHE INTERNAL "")
    
    add_library(LSChol SHARED ${LSChol_HDRS} ${LSChol_SRCS})
    target_include_directories(LSChol PUBLIC ${LSChol_INCLUDES})
    target_link_libraries(LSChol PUBLIC Eigen3::Eigen suitesparse::suitesparse)
    set_target_properties(LSChol PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
    set_target_properties(LSChol PROPERTIES LINK_FLAGS "-fopenmp")
endif(${ENABLE_SUITESPARSE})
# Build unit tests
if(${BUILD_PRECONDITIONERS_TESTS})
    message(STATUS "Adding unit tests to build")
    add_subdirectory(test)
endif()

# EXPORT THIS LIBRARY

# Add add entry for this project into CMake's package registry, so that this project can be found by other CMake projects
set(CMAKE_EXPORT_PACKAGE_REGISTRY FALSE)  # This flag is required to actually export the package for CMake versions >= 3.15
export(PACKAGE ${PROJECT_NAME})
if(${ENABLE_SUITESPARSE})
    # Create a configuration file for this project, so that it can be imported by other CMake projects
    export(TARGETS ILDL LSChol FILE ${PROJECT_NAME}Config.cmake)
else()
    export(TARGETS ILDL FILE ${PROJECT_NAME}Config.cmake)
endif()
