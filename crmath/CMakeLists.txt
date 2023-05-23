file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

set(SUB_PROJECT_NAME crmath)

if (NOT SOURCES)
    add_library(${SUB_PROJECT_NAME} INTERFACE)
else ()
    add_library(${SUB_PROJECT_NAME} ${SOURCES})
endif ()
# target_link_libraries(${SUB_PROJECT_NAME} PUBLIC )

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
