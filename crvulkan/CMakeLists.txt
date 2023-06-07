
# use vulkan and glfw (CPM if possible)
CPMAddPackage(
        NAME GLFW
        GITHUB_REPOSITORY glfw/glfw
        VERSION 3.3.4
)

add_crproject(NAME crvulkan
        LIBRARY
        DEPENDENCIES glfw vulkan crutil crmath)
