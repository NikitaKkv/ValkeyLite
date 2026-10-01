# include(FetchContent)

# FetchContent_Declare(
#     googletest
#     URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
#     DOWNLOAD_EXTRACT_TIMESTAMP ON
#     FIND_PACKAGE_ARGS NAMES GTest
# )

# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# enable_testing()

# add_executable(
#     valkey_tests
#     test.cpp
# )

# target_link_libraries(
#     valkey_tests
#     labwork_lib
#     GTest::gtest_main
#     GTest::gmock_main
# )

# target_include_directories(valkey_tests PUBLIC ${PROJECT_SOURCE_DIR})

# include(GoogleTest)

# gtest_discover_tests(valkey_tests)