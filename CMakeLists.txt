cmake_minimum_required(VERSION 3.12)
project(surena C CXX)
set(CMAKE_CXX_STANDARD 11)

set(INCLUDES
    src
)

set(SOURCES
    src/games/alhambra.cpp
    src/games/alhambra.hpp
    src/games/caesar.cpp
    src/games/caesar.hpp
    src/games/havannah.cpp
    src/games/havannah.hpp
    src/games/tictactoe.cpp
    src/games/tictactoe.hpp

    src/util/fast_prng.cpp
    src/util/fast_prng.hpp

    src/engines/randomengine.cpp
    src/engines/randomengine.hpp
    src/engines/singlethreadedmcts.cpp
    src/engines/singlethreadedmcts.hpp

    src/engine.hpp
    src/game.hpp
    src/main.cpp
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_COMPILE_FEATURES "${CMAKE_CXX_COMPILE_FEATURES} -Wall -Wextra") # enable warnings
find_package(Threads REQUIRED)
add_executable(surena ${SOURCES})
target_include_directories(surena PRIVATE ${INCLUDES})
target_link_libraries(surena PRIVATE Threads::Threads)

set_target_properties(surena PROPERTIES EXPORT_COMPILE_COMMANDS true)
