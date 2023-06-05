CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.3.4
        OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_INSTALL OFF"
)

# add glew
CPMAddPackage(
        NAME glew
        GITHUB_REPOSITORY Perlmint/glew-cmake
        GIT_TAG glew-cmake-2.2.0
        OPTIONS
        "GLEW_BUILD_UTILS OFF"
        "GLEW_BUILD_EXAMPLES OFF"
        "BUILD_SHARED_LIBS OFF"
)

#add freetype
CPMAddPackage(
        NAME freetype
        GITHUB_REPOSITORY freetype/freetype
        GIT_TAG VER-2-10-4
        OPTIONS
        "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}"
        "CMAKE_CXX_FLAGS -fPIC"
        "CMAKE_C_FLAGS -fPIC"
        "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

add_crproject(NAME crui LIBRARY DEPENDENCIES glfw libglew_static GL crmath crutil freetype)
