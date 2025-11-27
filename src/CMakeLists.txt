cmake_minimum_required(VERSION 3.10)

set(CORE_SOURCES
    core/divisors_lib.cpp
    core/equations.cpp
    core/expression.cpp
    core/input.cpp
    core/math_utils.cpp
    core/numeral_conversion.cpp
)

set(APP_SOURCES
    main.cpp
    app/cli_actions.cpp
    app/menu_handlers.cpp
)

set(TOOLS_SOURCES
    tools/divisors.cpp
)

set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

add_library(calculator_core ${CORE_SOURCES})
target_include_directories(calculator_core PUBLIC ${COMMON_INCLUDES})

add_executable(divisors ${TOOLS_SOURCES})
add_executable(calculator ${APP_SOURCES})

target_link_libraries(divisors PRIVATE calculator_core)
target_link_libraries(calculator PRIVATE calculator_core)

if(MSVC)
  target_compile_options(divisors PRIVATE /W4)
  target_compile_options(calculator PRIVATE /W4)
endif()
enable_testing()
