cmake_minimum_required(VERSION 3.10)
project(SylvanOdyssey)

set(CMAKE_CXX_STANDARD 17)

# Include dirs
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/glad
    ${CMAKE_SOURCE_DIR}/include/GLFW
    ${CMAKE_SOURCE_DIR}/include/glm
    ${CMAKE_SOURCE_DIR}/include/KHR
)

# Link dirs
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Sources
add_executable(SylvanOdyssey
    main.cpp
    glad.c
)

# Link libraries
if (WIN32)
    target_link_libraries(SylvanOdyssey
        glfw3dll
        opengl32
    )
else()
    target_link_libraries(SylvanOdyssey
        glfw3
        GL
        dl
        pthread
    )
endif()

# Copy shaders to build dir
configure_file(shader.vert ${CMAKE_BINARY_DIR}/shader.vert COPYONLY)
configure_file(shader.frag ${CMAKE_BINARY_DIR}/shader.frag COPYONLY)

# Copy model files to build dir
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})
