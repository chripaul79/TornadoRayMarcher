# CMakeList.txt : CMake project for TornadoSceneProject, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (tornado)

# TODO: Add tests and install targets if needed.

#
# Set python cmd, preferring python 3
#

find_program(PYTHON_3_LOCATION py)
if(NOT "${PYTHON_3_LOCATION}" STREQUAL "PYTHON_3_LOCATION-NOTFOUND") # windows shim
	execute_process(COMMAND py -3 --version RESULT_VARIABLE has_python_3)
	if("${has_python_3}" STREQUAL "0")
		set (PYTHON_CMD py -3)
	else()
		set (PYTHON_CMD py) # may use python 2
	endif()
else()
    find_program(PYTHON_3_LOCATION python3)
    if(NOT "${PYTHON_3_LOCATION}" STREQUAL "PYTHON_3_LOCATION-NOTFOUND") # windows shim
    	set (PYTHON_CMD python3)
    else()
    	set (PYTHON_CMD python)
    endif()
endif()
message("python used: ${PYTHON_CMD}")

#
# CMake setup
#
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_VERBOSE_MAKEFILE 0) # 1 should be used for debugging
set (CMAKE_SUPPRESS_REGENERATION TRUE) # Suppresses ZERO_CHECK
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  if(NOT WIN32)
    set(GLAD_LIBRARIES dl)
  endif()
endif()


#
# GLFW
#
option (GLFW_INSTALL OFF)
option (GLFW_BUILD_DOCS OFF)
option (GLFW_BUILD_EXAMPLES OFF)
option (GLFW_BUILD_TESTS OFF)
add_subdirectory (lib/glfw)

#
# FMT
#
add_subdirectory (lib/fmt)

#
# GLAD
#
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include)
    message("Generating glad library files, using ${PYTHON_CMD}")
	execute_process(
		COMMAND ${PYTHON_CMD} -m glad --profile core --out-path . --generator c --spec gl
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad
	)
	message("Finished generating glad library files")
endif()

#
# Set include paths
#
include_directories (src/
                     lib/glad/include/
                     lib/glfw/include/
                     lib/lodepng/
                     lib/glm/
                     lib/stb/
                     lib/arrrgh/
                     lib/perlin/include/)

#
# Add files
#
file (GLOB         VENDORS_SOURCES lib/glad/src/glad.c
								   lib/lodepng/lodepng.cpp)
file (GLOB_RECURSE PROJECT_HEADERS src/*.hpp
                                   src/*.h)
file (GLOB_RECURSE PROJECT_SOURCES src/*.cpp
                                   src/*.cxx
                                   src/*.cc
                                   src/*.c)
file (GLOB_RECURSE PROJECT_SHADERS res/shaders/*.comp
                                   res/shaders/*.frag
                                   res/shaders/*.geom
                                   res/shaders/*.vert)
file (GLOB         PROJECT_CONFIGS CMakeLists.txt
                                   README.rst
                                  .gitignore
                                  .gitmodules)
file(COPY ${CMAKE_SOURCE_DIR}/res
     DESTINATION ${CMAKE_BINARY_DIR})

#
# Organizing files
#
source_group ("headers" FILES ${PROJECT_HEADERS})
source_group ("shaders" FILES ${PROJECT_SHADERS})
source_group ("sources" FILES ${PROJECT_SOURCES})
source_group ("libraries" FILES ${VENDORS_SOURCES})

#
# Set executable and target link libraries
#
add_definitions (-DGLFW_INCLUDE_NONE
                 -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable (${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
                                ${PROJECT_SHADERS} ${PROJECT_CONFIGS}
                                ${VENDORS_SOURCES} ${PROJECT_RES})
target_link_libraries (${PROJECT_NAME}
                       glfw
                       fmt::fmt
                       ${GLFW_LIBRARIES}
                       ${GLAD_LIBRARIES})

option(TORNADO_ENABLE_IMGUI "Enable Dear ImGui overlay" ON)
set(TORNADO_IMGUI_AVAILABLE OFF)
if(TORNADO_ENABLE_IMGUI)
    set(IMGUI_LOCAL_DIR "${CMAKE_SOURCE_DIR}/lib/imgui")
    if(EXISTS "${IMGUI_LOCAL_DIR}/imgui.cpp")
        target_sources(${PROJECT_NAME} PRIVATE
            "${IMGUI_LOCAL_DIR}/imgui.cpp"
            "${IMGUI_LOCAL_DIR}/imgui_draw.cpp"
            "${IMGUI_LOCAL_DIR}/imgui_tables.cpp"
            "${IMGUI_LOCAL_DIR}/imgui_widgets.cpp"
            "${IMGUI_LOCAL_DIR}/backends/imgui_impl_glfw.cpp"
            "${IMGUI_LOCAL_DIR}/backends/imgui_impl_opengl3.cpp"
        )
        include_directories("${IMGUI_LOCAL_DIR}" "${IMGUI_LOCAL_DIR}/backends")
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            TORNADO_ENABLE_IMGUI
            IMGUI_IMPL_OPENGL_LOADER_GLAD
        )
        set(TORNADO_IMGUI_AVAILABLE ON)
        message(STATUS "Dear ImGui enabled from lib/imgui")
    else()
        message(WARNING "Dear ImGui not found in lib/imgui. Building without UI.")
    endif()
endif()
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tornado)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

# Copy resources (shaders/textures) into the build directory every build
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/res"
            "${CMAKE_BINARY_DIR}/res"
    COMMENT "Copying res/ -> build/res/"
)

# Make your executable depend on the copy target so resources are present before build/run.
# Replace 'your_exe_target' with your actual target name.
add_dependencies(${PROJECT_NAME} copy_resources)

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()


