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

add_executable(divisors ${TOOLS_SOURCES} core/divisors_lib.cpp)
add_executable(calculator ${APP_SOURCES} ${CORE_SOURCES})

set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

target_include_directories(divisors PRIVATE ${COMMON_INCLUDES})
target_include_directories(calculator PRIVATE ${COMMON_INCLUDES})

if(MSVC)
  target_compile_options(divisors PRIVATE /W4)
  target_compile_options(calculator PRIVATE /W4)
endif()
