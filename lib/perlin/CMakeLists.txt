cmake_minimum_required(VERSION 3.9)
project(perlin)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif()

add_library(perlin INTERFACE)
target_sources(perlin INTERFACE
        ${PROJECT_SOURCE_DIR}/include/perlin/seamless_noise_generator_2d.h
        ${PROJECT_SOURCE_DIR}/include/perlin/fractal_noise_generator.h
        ${PROJECT_SOURCE_DIR}/include/perlin/perlin_noise_generator.h
        ${PROJECT_SOURCE_DIR}/include/perlin/point.h
        ${PROJECT_SOURCE_DIR}/include/perlin/vector.h
        ${PROJECT_SOURCE_DIR}/include/perlin/math.h)
target_include_directories(perlin INTERFACE include)

find_package(PNG)
if (PNG_FOUND)
    add_executable(perlin_test main.cpp)
    target_link_libraries(perlin_test ${PNG_LIBRARY} perlin m)
else()
    message(info "Did not find libpng. Not building test executable.")
endif ()
