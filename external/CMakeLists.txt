find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# Static Library: ImGui
add_library(${CG_IMGUI_LIB}
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        )
target_link_libraries(${CG_IMGUI_LIB}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENGL_LIBRARIES}
        ${GLFW_LIBRARIES}
        ${GLFW_DEPS}
        )
target_include_directories(${CG_IMGUI_LIB} PRIVATE imgui)