cmake_minimum_required(VERSION 3.18)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

project(surena C CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(NOT CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg].*")
    if(supported)
        message(STATUS "IPO/LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
else()
    message(STATUS "IPO/LTO disabled in non-release build")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(INCLUDES
    lib/rosalia/includes

    includes
    src
    "${CMAKE_BINARY_DIR}/build_generated"
)

set(SOURCES
    # src/engines/randomengine.cpp
    # src/engines/singlethreadedmcts.cpp
    # src/engines/uci_wrap.cpp

    # src/games/alhambra.cpp
    # src/games/caesar.cpp
    src/games/chess.cpp
    src/games/havannah.cpp
    # src/games/oshisumo.cpp
    src/games/quasar.cpp
    src/games/rockpaperscissors.cpp
    src/games/tictactoe_ultimate.cpp
    src/games/tictactoe.cpp
    src/games/twixt_pp.cpp

    src/rosa_impl/base64.c
    src/rosa_impl/noise.c
    src/rosa_impl/rand.c
    src/rosa_impl/semver.c
    src/rosa_impl/serialization.c
    src/rosa_impl/timestamp.c

    # src/engine.cpp
    src/game.c
    # src/move_history.c
    src/repl.c

    src/main.cpp
)

add_executable(surena "${SOURCES}" "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")

# export git commit hash and dirty flag to generated file in the build/build_generated/generated dir
find_package(Git)
set(dollar "$") # next file is processed using configure_file so we need to escape dollars to use variables
configure_file(
    cmake/generate_git_commit_hash.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/generate_git_commit_hash.cmake
    ESCAPE_QUOTES
)
add_custom_target(add_generated_dir COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/build_generated/generated")
add_custom_target(generate_git_commit_hash DEPENDS add_generated_dir COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/generate_git_commit_hash.cmake" BYPRODUCTS "${CMAKE_BINARY_DIR}/build_generated/generated/git_commit_hash.h")
add_dependencies(surena generate_git_commit_hash)

target_compile_options(surena PRIVATE
    # "-Wall"
    # "-Wextra"
    # "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
)

target_include_directories(surena PRIVATE ${INCLUDES})

target_link_options(surena PRIVATE -rdynamic)

target_link_libraries(surena dl)
target_link_libraries(surena Threads::Threads)

set_target_properties(surena PROPERTIES EXPORT_COMPILE_COMMANDS true)
