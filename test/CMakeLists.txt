include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.8.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

if(MSVC)
   add_compile_options(/bigobj)
endif()

add_executable(TnTEngine_Tests TnTThreadPoolTests.cpp)
target_link_libraries(TnTEngine_Tests PRIVATE project_warnings project_options gtest_main gmock)
target_include_directories(TnTEngine_Tests PRIVATE ${CMAKE_SOURCE_DIR})

gtest_discover_tests(
  TnTEngine_Tests
  TEST_PREFIX
  "unittests."
  REPORTER
  xml
  OUTPUT_DIR
  .
  OUTPUT_PREFIX
  "unittests."
  OUTPUT_SUFFIX
  .xml
)