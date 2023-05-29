# add glfw
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

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

set(SUB_PROJECT_NAME crui)

if (NOT SOURCES)
    add_library(${SUB_PROJECT_NAME} INTERFACE)
else ()
    add_library(${SUB_PROJECT_NAME} ${SOURCES})
endif ()
target_link_libraries(${SUB_PROJECT_NAME} PUBLIC glfw libglew_static GL crmath crutil freetype)

target_include_directories(${SUB_PROJECT_NAME} INTERFACE include)
if (SOURCES)
    target_include_directories(${SUB_PROJECT_NAME} PRIVATE src PRIVATE include/${SUB_PROJECT_NAME})
endif ()

# only if test sources are found
if (TEST_SOURCES)
    enable_testing()
    add_test(NAME ${SUB_PROJECT_NAME}_test COMMAND ${SUB_PROJECT_NAME}_test)
    add_executable(${SUB_PROJECT_NAME}_test ${TEST_SOURCES})
    target_link_libraries(${SUB_PROJECT_NAME}_test ${SUB_PROJECT_NAME})
endif ()
