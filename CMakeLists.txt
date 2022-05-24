cmake_minimum_required(VERSION 3.21)

# Set the project name to your project name, my project isn't very descriptive
project(TnTEngine CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)

target_compile_features(project_options INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
  if(ENABLE_BUILD_WITH_TIME_TRACE)
    target_compile_options(project_options INTERFACE -ftime-trace)
  endif()
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# Add linker configuration
include(cmake/Linker.cmake)
configure_linker(project_options)

# standard compiler warnings
include(cmake/CompilerOptions.cmake)
set_project_warnings(project_warnings)
ignore_warnings_in_external(project_options)
set_standard_compiler_settings(project_options)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()


include(FetchContent)
if (NOT googletest_POPULATED)
   message(STATUS "Pulling Google Test into the project...")
   FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        release-1.11.0
   )
   message(STATUS "Making Google Test available...")
   FetchContent_MakeAvailable(googletest)
   
   message(STATUS "Google Test Added!")
endif()

enable_testing()
message(STATUS "Testing Enabled")
add_subdirectory(test)



