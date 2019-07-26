cmake_minimum_required(VERSION 3.0)
include(CheckIncludeFileCXX)

project (ogm-project-gles)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror=return-type")

add_definitions(-DQUEUE_COLLISION_UPDATES)

file (GLOB_RECURSE _COMMON_SRC
    "src/common/*.cpp"
)

file (GLOB_RECURSE _AST_SRC
    "src/ast/*.cpp"
)

file (GLOB_RECURSE _OGM_BYTECODE_SRC
    "src/bytecode/*.cpp"
)

file (GLOB_RECURSE _OGM_BEAUTIFY_SRC
    "src/beautify/*.cpp"
)

file (GLOB_RECURSE _OGM_PROJECT_SRC
    "src/project/*.cpp"
)

file(GLOB _GIG_SRC
    "src/gig/*.cpp"
)

file(GLOB_RECURSE _TEST_SRC
    "test/*.cpp"
)

file (GLOB_RECURSE _OGMI_SRC
    "src/interpreter/*.cpp"
)

include_directories(include)
include_directories("external/include")


if(DEFINED ENV{EMSCRIPTEN})

    #SET(linkFlags "-s WASM=1")

    set(CMAKE_EXECUTABLE_SUFFIX ".js")

    add_definitions(-DEMSCRIPTEN)

    add_executable(ogm
        "main/main.cpp"
        ${_AST_SRC}
        ${_COMMON_SRC}
        ${_OGM_BYTECODE_SRC}
        ${_OGM_BEAUTIFY_SRC}
        "external/pugixml/pugixml.cpp"
        ${_OGM_PROJECT_SRC}
        "external/stb/stb_image.cpp"
        ${_OGMI_SRC}
    )
    add_executable(ogm-test
        ${_TEST_SRC}
        ${_AST_SRC}
        ${_COMMON_SRC}
        ${_OGM_BYTECODE_SRC}
        ${_OGM_BEAUTIFY_SRC}
        "external/pugixml/pugixml.cpp"
        ${_OGM_PROJECT_SRC}
        "external/stb/stb_image.cpp"
        ${_OGMI_SRC}
    )
else()

    add_definitions(-DPARALLEL_COMPILE)

    add_library(ogmast
      ${_AST_SRC}
    )

    add_library(ogm-common
      ${_COMMON_SRC}
    )

    add_library(ogm-bytecode
      ${_OGM_BYTECODE_SRC}
    )

    add_library(ogm-beautify
      ${_OGM_BEAUTIFY_SRC}
    )

    add_library(ogm-project
      "external/pugixml/pugixml.cpp"
      ${_OGM_PROJECT_SRC}
    )

    add_library(ogmi
      "external/stb/stb_image.cpp"
      ${_OGMI_SRC}
    )

    add_executable(ogm "main/main.cpp")
    add_executable(ogm-test
        ${_TEST_SRC}
    )

    target_link_libraries(ogmast ogm-common)
    target_link_libraries(ogm-bytecode ogmast  ogm-common)
    target_link_libraries(ogm-project ogm-bytecode ogmast ogm-common pthread)
    target_link_libraries(ogmi ogmast ogm-common)
    target_link_libraries(ogm      ogmast ogm-bytecode ogm-beautify ogmi ogm-project ogm-common pthread)
    target_link_libraries(ogm-test ogmast ogm-bytecode ogm-beautify ogmi ogm-project ogm-common pthread)

endif()


### dependencies ###

function(apt_install_advice package)
    if(UNIX AND NOT APPLE)
        message("")
        message("    -- SUGGESTION: apt install " ${package})
        message("")
    endif()
endfunction(apt_install_advice)

# GNU Readline
find_library(READLINE_LIB readline)
if(READLINE_LIB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DREADLINE_AVAILABLE")
    target_link_libraries(ogm readline)
    target_link_libraries(ogm-test readline)
endif()
if(NOT READLINE_LIB)
    message("WARNING: readline library not found. Debugger crippled.")
endif()

# glfw3, opengl, glew
find_library(GLFW3_LIBRARY NAMES glfw3 glfw libglfw libglfw3)
find_package(OpenGL)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_library(GLEW_LIBRARY NAMES GLEW glew32 glew glew32s PATH_SUFFIXES lib64)
if(NOT GLFW3_LIBRARY)
    message("WARNING: glfw3 not found. Graphics will be disabled.")
    apt_install_advice(libglfw3-dev)
else()
    if(NOT OPENGL_FOUND)
        message("WARNING: OpenGL not found. Graphics will be disabled.")
    else()
        if(NOT GLEW_LIBRARY)
            message("WARNING: GLEW library files not found. Graphics will be disabled.")
            apt_install_advice(libglew-dev)
        else()
            if(NOT GLEW_INCLUDE_DIR)
                message("WARNING: GLEW include directory not found. Graphics will be disabled.")
                apt_install_advice(libglew-dev)
            else()
                unset(GLM_FOUND CACHE)
                CHECK_INCLUDE_FILE_CXX("glm/glm.hpp" GLM_FOUND)
                if(NOT GLM_FOUND)
                    message("WARNING: OpenGLM Mathematics library (GLM) not found. Graphics will be disabled.")
                    apt_install_advice(libglm-dev)
                else()
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGFX_AVAILABLE")
                    target_link_libraries(ogm ${GLFW3_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY})
                    target_link_libraries(ogm-test ${GLFW3_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY})
                endif()
            endif()
        endif()
    endif()
endif()

# dynamic external linking
if (WIN32)
    # doesn't need any particular linkage to use LoadLibrary.
else ()
    target_link_libraries(ogm dl)
    target_link_libraries(ogm-test dl)
endif ()

# windows settings
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -m32")
endif()


if (WIN32)
    # due to position-dependent code, gig cannot be compiled as a .a file.
    add_library(gig SHARED
      ${_GIG_SRC}
    )

    SET_TARGET_PROPERTIES(gig PROPERTIES PREFIX "")
    target_link_libraries(gig ogmast ogm-bytecode)
endif (WIN32)
