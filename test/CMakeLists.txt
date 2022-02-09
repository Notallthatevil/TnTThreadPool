if(MSVC)
   add_compile_options(/bigobj)
endif()

add_executable(TnTEngine_Tests TnTThreadPoolTests.cpp)
include(GoogleTest)
target_link_libraries(TnTEngine_Tests PRIVATE project_warnings project_options gtest_main gmock)
target_include_directories(TnTEngine_Tests PRIVATE ${CMAKE_SOURCE_DIR})

gtest_discover_tests(
  TnTEngine_Tests
)