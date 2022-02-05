cmake_minimum_required(VERSION 3.18)
project(surena)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(INCLUDES
    includes
    src
)

set(SOURCES
    src/engines/randomengine.cpp
    src/engines/singlethreadedmcts.cpp

    src/games/alhambra.cpp
    src/games/caesar.cpp
    src/games/havannah.cpp
    src/games/tictactoe.cpp

    src/util/fast_prng.cpp

    src/main.cpp
)

add_executable(surena ${SOURCES})

target_include_directories(surena PRIVATE ${INCLUDES})

target_link_libraries(surena PUBLIC Threads::Threads)

set_target_properties(surena PROPERTIES EXPORT_COMPILE_COMMANDS true)
