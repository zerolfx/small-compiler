find_package(GTest REQUIRED)

enable_testing()

add_executable(test test.cpp ../env.cpp ../ast.cpp)
target_link_libraries(test gtest gtest_main fmt::fmt)
