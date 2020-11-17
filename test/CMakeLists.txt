find_package(GTest REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
enable_testing()

add_executable(test test.cpp ../env.cpp ../ast.cpp)
target_link_libraries(test gtest gtest_main fmt::fmt Threads::Threads)
