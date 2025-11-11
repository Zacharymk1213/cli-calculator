cmake_minimum_required(VERSION 3.10)

project(c-tools VERSION 0.1 LANGUAGES CXX)

# Recommended: out-of-source builds
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable good warnings for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(src)

option(BUILD_TESTING "Enable testing" OFF)

install(TARGETS divisors calculator
        RUNTIME DESTINATION bin)
