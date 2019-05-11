cmake_minimum_required(VERSION 3.11)
project(rembot)

set(BOOST_ROOT "/opt/boost")

set(CPP_VERSION -std=c++14)

set(SFML_STATIC_LIBRARIES TRUE)

set(Boost_USE_STATIC_LIBS ON)

list(APPEND BOOST_COMPONENTS
        thread
        system)

include(FindPkgConfig)
find_package(SFML COMPONENTS system window graphics network audio REQUIRED)
find_package(OpenGL REQUIRED)

find_package(Boost 1.68 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(${Boost_INCLUDE_DIR})


include_directories(libext/asio_bluetooth/asio_bluetooth)

set(LIB_FILES
        libext/imgui.cpp libext/imgui_demo.cpp libext/imgui_draw.cpp libext/imgui-SFML.cpp libext/asio_bluetooth/wrapper.cpp
        src/detail.cpp
        src/queue.h
        src/data.cpp
        src/editor.cpp
        src/main.cpp
        src/core.cpp
        )

set(LIB_TEST_FILES
        libext/asio_bluetooth/wrapper.cpp
        test/main.cpp
        )



include_directories(${SFML_INCLUDE_DIR})

link_libraries(stdc++fs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fpermissive ")

add_executable(rembot ${LIB_FILES})
add_executable(rembot_control ${LIB_TEST_FILES})


target_link_libraries(rembot ${SFML_LIBRARIES} ${OPENGL_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}  -lbluetooth -lsfml-window -lsfml-graphics -lsfml-system)
target_link_libraries(rembot_control ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}  -lbluetooth)

