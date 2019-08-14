cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFileCXX)

project (ogm-project-gles)

set(CMAKE_BUILD_TYPE Debug)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4033 /Zc:externConstexpr")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror=return-type")

    # FIXME: many -W ignores. These should be removed (and any errors resolved.)
	if (!WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wno-sign-compare -Wno-reorder -Wno-switch -Wno-unused-function -Wno-unused-label")
	endif()
endif ()

add_definitions(-DQUEUE_COLLISION_UPDATES)
add_definitions(-DVERBOSE_COMPILE_LOG)

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

    if (NOT CMAKE_EXECUTABLE_SUFFIX STREQUAL ".js")
        message(FATAL_ERROR "The default suffix for building executables should be .js!\nTry running ./cmake-clean.sh and then build with ./embuild.sh")
    endif()
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    SET(linkFlags "-s TOTAL_MEMORY=64000kb -s DISABLE_EXCEPTION_CATCHING=0 -s")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s USE_SDL=2 -s DISABLE_EXCEPTION_CATCHING=0 -s EXIT_RUNTIME=1 -s TOTAL_MEMORY=64000kb")

    # TODO: figure out how to make --preload-file work, since it's less intensive.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --embed-file demo/")

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
    if (CMAKE_EXECUTABLE_SUFFIX STREQUAL ".js")
        message(FATAL_ERROR "The default suffix for building executables is set to .js!\nTry running ./cmake-clean.sh")
    endif()

	if (!WIN32 OR MSVC)
		# couldn't get this to work with mingw32-make for some reason ¯\_(ツ)_/¯
		add_definitions(-DPARALLEL_COMPILE)
	endif()

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

    if (MSVC OR WIN32)
    	set(PTHREAD "")
    else ()
    	set(PTHREAD "pthread")
    endif ()

    target_link_libraries(ogmast ogm-common)
    target_link_libraries(ogm-bytecode ogmast  ogm-common)
    target_link_libraries(ogm-project ogm-bytecode ogmast ogm-common ${PTHREAD})
    target_link_libraries(ogmi ogmast ogm-common)
    target_link_libraries(ogm      ogmast ogm-bytecode ogm-beautify ogmi ogm-project ogm-common ${PTHREAD})
    target_link_libraries(ogm-test ogmast ogm-bytecode ogm-beautify ogmi ogm-project ogm-common ${PTHREAD})

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
    target_link_libraries(ogm ${READLINE_LIB})
    target_link_libraries(ogm-test ${READLINE_LIB})
	add_definitions(-DREADLINE_STATIC)
	# for our windows friends
	find_path(READLINE_INCLUDE_DIR readline/readline.h)
	if (READLINE_INCLUDE_DIR)
		include_directories(${READLINE_INCLUDE_DIR} SYSTEM)
	endif ()
endif()
if(NOT READLINE_LIB OR NOT READLINE_INCLUDE_DIR)
    message("WARNING: readline library not found. Debugger crippled.")
endif()

# glfw3, opengl, glew, glm
find_library(GLFW3_LIBRARY NAMES glfw3 glfw libglfw libglfw3)
find_package(OpenGL)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_library(GLEW_LIBRARY NAMES GLEW glew32 glew glew32s PATH_SUFFIXES lib64)


if (DEFINED ENV{EMSCRIPTEN})
  # emscripten has support for these; override above.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGFX_AVAILABLE")
  include_directories("external/emscripten/include")
  target_link_libraries(ogm glfw)
  target_link_libraries(ogm-test glfw)
else()
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
	if (MSVC)
	else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -m32")
	endif ()
endif()


if (WIN32)
    # due to position-dependent code, gig cannot be compiled as a .a file.
    add_library(gig SHARED
      ${_GIG_SRC}
    )

    SET_TARGET_PROPERTIES(gig PROPERTIES PREFIX "")
    target_link_libraries(gig ogmast ogm-bytecode)
endif (WIN32)
