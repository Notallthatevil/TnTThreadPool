if(MSVC)
   add_compile_options(/bigobj)
endif()

add_executable(TnTTests TnTThreadPoolTests.cpp)
include(GoogleTest)
target_link_libraries(TnTTests PRIVATE project_warnings project_options gtest_main gmock)
target_include_directories(TnTTests PRIVATE ${CMAKE_SOURCE_DIR})

gtest_discover_tests(
  TnTTests
)