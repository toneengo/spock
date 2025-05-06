cmake_minimum_required(VERSION 3.12)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS OFF)

option(SPOCK_BUILD_ENGINE "build executable" ON)

project("spock")
add_library(spock)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "spock")

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(spock PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(spock PUBLIC Vulkan::Vulkan)

find_package(glslang REQUIRED)
target_link_libraries(spock PRIVATE glslang::glslang)
target_link_libraries(spock PRIVATE glslang::glslang-default-resource-limits)

find_package(assimp REQUIRED)
target_link_libraries(spock PRIVATE assimp::assimp)

find_package(assimp REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap)
target_link_libraries(spock PRIVATE vk-bootstrap::vk-bootstrap)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "Configuring spock in Debug with CMake")
    add_compile_definitions(DBG)
else()
    add_compile_options(-O3)
    message(STATUS "Configuring spock in Release with CMake")
endif()

list(APPEND Targets spock)

foreach(TARGET IN LISTS Targets)
    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/")
    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/")
    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include")
    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include")
endforeach()
