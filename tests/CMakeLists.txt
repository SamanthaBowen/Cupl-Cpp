enable_testing()

file(GLOB_RECURSE "test-registries-sources" "registries/*.cpp")
add_executable("test-registries" ${test-registries-sources})
set_property(TARGET "test-registries" PROPERTY CXX_STANDARD 23)
target_include_directories("test-registries" PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries("test-registries" tbb)

file(GLOB_RECURSE "test-slicing-sources" "slicing/*.cpp")
add_executable("test-slicing" ${test-slicing-sources})
set_property(TARGET "test-slicing" PROPERTY CXX_STANDARD 23)
target_include_directories("test-slicing" PRIVATE "${PROJECT_SOURCE_DIR}/include")

add_test(NAME "slicing" COMMAND "test_slicing")
