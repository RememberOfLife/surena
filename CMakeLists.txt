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
    includes
    src
)

set(SOURCES
    # src/engines/randomengine.cpp
    # src/engines/singlethreadedmcts.cpp

    # src/games/alhambra.cpp
    # src/games/caesar.cpp
    src/games/chess.cpp
    src/games/havannah.cpp
    src/games/oshisumo.cpp
    src/games/tictactoe_ultimate.cpp
    src/games/tictactoe.cpp

    src/util/fast_prng.cpp
    src/util/noise.cpp
    src/util/semver.c

    # src/engine.cpp
    src/game.c

    src/main.cpp
)

add_executable(surena ${SOURCES})

target_compile_options(surena PRIVATE
    #"-Wall"
    "-Werror" # warnings as errors
    "-Wfatal-errors" # stop after first error
)

target_include_directories(surena PRIVATE ${INCLUDES})

target_link_libraries(surena dl)
target_link_libraries(surena Threads::Threads)

set_target_properties(surena PROPERTIES EXPORT_COMPILE_COMMANDS true)
