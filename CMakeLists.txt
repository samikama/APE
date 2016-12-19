cmake_minimum_required (VERSION 2.8)
project (APE)
set(APE_MAJOR_VERSION 1)
set(APE_MINOR_VERSION 2)
set(APE_PATCH_VERSION 0)
set(APE_VERSION ${APE_MAJOR_VERSION}.${APE_MINOR_VERSION}.${APE_PATCH_VERSION} )
macro (dump_var arg )
  message(STATUS "${arg}=${${arg}}")
endmacro (dump_var)

set(targets_export_name "${PROJECT_NAME}Targets")

function (checkModulesDir arg)
  message("Checking arg=${arg}")
  if( NOT ${arg} STREQUAL "" )
    get_filename_component(MODULES_ABS_PATH ${arg} ABSOLUTE)
    message(STATUS "Checking MODULES_ABS_PATH=${MODULES_ABS_PATH}")
    if( EXISTS "${MODULES_ABS_PATH}/CMakeLists.txt")
      message(STATUS "ADDING directory ${MODULES_ABS_PATH} as subdir")
      add_subdirectory (${MODULES_ABS_PATH} "${CMAKE_CURRENT_BINARY_DIR}/Modules")
      set (MODULES_FOUND "YES")
    else()
      message(WARNING "${MODULES_ABS_PATH} do not contain CMakeLists.txt file.")
    endif()
  endif()
  if (NOT MODULES_FOUND)
    message(FATAL_ERROR "Could NOT find MODULES in given path.")
  endif(NOT MODULES_FOUND)
endfunction(checkModulesDir)

dump_var("CMAKE_BUILD_TYPE")
dump_var("CMAKE_CXX_FLAGS")
if( "${CMAKE_BUILD_TYPE}" STREQUAL "" )
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
dump_var("CMAKE_GENERATOR_CXX")
dump_var("CMAKE_CXX_COMPILER")
dump_var("CMAKE_HOST_UNIX")
dump_var("CMAKE_SYSTEM")
dump_var("CMAKE_SYSTEM_NAME")
dump_var("CMAKE_SYSTEM_PROCESSOR")
dump_var("CMAKE_SYSTEM_VERSION")
dump_var("CMAKE_BUILD_TYPE")
dump_var("CMAKE_CXX_COMPILER_ID")
dump_var("CMAKE_CXX_COMPILER_ID_RUN")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(COMP "llvm")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -ftree-vectorize -ftree-vectorizer-verbose=1 ")
  set(COMP "gcc")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -vec-report=1 ")
  set(COMP "icc")
endif()

if ( "$ENV{CMTCONFIG}" STREQUAL "")
  if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    set(REL "dbg")
  else()
    set(REL "opt")
  endif()
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE COMP_VERSION)
  string(REGEX MATCHALL "[0-9]+" COMP_VERSION_COMPONENTS ${COMP_VERSION})
  list(GET COMP_VERSION_COMPONENTS 0 COMP_MAJOR)
  list(GET COMP_VERSION_COMPONENTS 1 COMP_MINOR)
  set(CMTCONFIG "${CMAKE_SYSTEM_PROCESSOR}-slc6-${COMP}${COMP_MAJOR}${COMP_MINOR}-${REL}")
else("$ENV{CMTCONFIG}" STREQUAL "")
  set(CMTCONFIG $ENV{CMTCONFIG})
endif("$ENV{CMTCONFIG}" STREQUAL "")
dump_var("COMP_VERSION")
dump_var("CMTCONFIG")
dump_var("CMAKE_INSTALL_PREFIX")


set (CMAKE_VERBOSE_MAKEFILE ON)
dump_var("CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT")
set(CMAKE_INSTALL_PREFIX 
  $ENV{ATLAS_EXTERNAL}/APE/${APE_VERSION}/${CMTCONFIG} CACHE PATH "Developer install path")
dump_var("CMAKE_INSTALL_PREFIX")
dump_var("CMAKE_BINARY_DIR")
include_directories ("${PROJECT_SOURCE_DIR}/" )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

find_package(TBB REQUIRED)
find_package(Yampl REQUIRED)
find_package(Yamlcpp REQUIRED)
include_directories(${TBB_INCLUDE_DIRS} ${YAMPL_INCLUDE_DIRS})

if((${CMAKE_VERSION} VERSION_GREATER "3.1.0") OR (${CMAKE_VERSION} VERSION_EQUAL "3.1.0"))
  set( CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard used for the build" )
  set( CMAKE_CXX_EXTENSIONS FALSE CACHE BOOL "(Dis)allow using GNU extensions" )
else()
  add_definitions(-std=c++11)
endif()

add_library(APEContainer SHARED src/BufferContainer.cxx)

if(HAVE_YAMLCPP05)
  add_library(APEConfig SHARED src/ConfigTree.cxx src/ConfigIO.cxx src/YamlConfig.cxx)
  message(STATUS "USING YAMLCPP v 05 ")
else()
  add_library(APEConfig SHARED src/ConfigTree.cxx src/ConfigIO.cxx src/YamlConfig.cxx)
endif()
add_library(APEUtils SHARED src/APEHist.cxx src/Log.cxx)
add_executable(apeMod src/APE.cxx src/Server.cxx)

message(STATUS " TBB Library is ${TBB_LIBRARY}")
dump_var("TBB_INCLUDE_DIRS")
target_link_libraries(apeMod 
  ${YAMPL_LIBRARIES}
  #${TBB_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}tbb${CMAKE_SHARED_LIBRARY_SUFFIX} 
  ${TBB_LIBRARY}
  APEContainer APEConfig APEUtils
  rt dl
  )
target_include_directories(APEConfig BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${YAMLCPP_INCLUDE_DIRS}"
  PRIVATE "${YAMPL_INCLUDE_DIRS}")

target_link_libraries(APEConfig 
  ${YAMLCPP_LIBRARIES}
  )

install(TARGETS apeMod APEContainer APEConfig APEUtils
  EXPORT "${targets_export_name}"
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib"
  RUNTIME DESTINATION "bin"
  )
install (DIRECTORY APE DESTINATION include 
  PATTERN ".svn" EXCLUDE 
  PATTERN ".git" EXCLUDE 
  PATTERN "*.hpp")


option(BUILD_EXAMPLES "Build example modules and client" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory("examples")
endif()
dump_var("CMAKE_BUILD_TYPE")
dump_var("CMAKE_CXX_FLAGS")
dump_var("CMAKE_CXX_COMPILER_ID")
dump_var("CMAKE_INSTALL_PREFIX")
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
set(INCLUDE_INSTALL_DIR "include/")
set(LIB_INSTALL_DIR "lib/")
set(TBB_INCLUDES_DIR ${TBB_INCLUDE_DIRS})
set(TBB_LIBS_DIR ${TBB_LIBRARY_DIR})
set(YAMPL_INCS ${YAMPL_INCLUDE_DIRS})
set(YAMPL_LIBS ${YAMPL_LIBRARIES})

set(CONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}" CACHE PATH "install dir for APEConfig.cmake")

set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

configure_package_config_file(cmake/APEConfig.cmake.in 
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
  PATH_VARS INCLUDE_INSTALL_DIR 
  LIB_INSTALL_DIR TBB_INCLUDES_DIR 
  TBB_LIBS_DIR YAMPL_LIBS YAMPL_INCS)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake 
  VERSION "${APE_VERSION}" COMPATIBILITY SameMajorVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/APEConfig.cmake 
  ${CMAKE_CURRENT_BINARY_DIR}/APEConfigVersion.cmake
  DESTINATION ${CONFIG_INSTALL_DIR} )

install(
  EXPORT "${targets_export_name}"
  NAMESPACE "${namespace}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
  )
