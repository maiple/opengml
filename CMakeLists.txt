cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFileCXX)

project (ogm-project-gles)

option(RELEASE Release OFF)
option(NOGRAPHICS NoGraphics OFF)
option(OGM_2DARRAY "Use 2D arrays" OFF)
option(NO_FCL "Disable optional FCL support" OFF)
option(PARALLEL_COMPILE "Compile with multiple cores" ON)
option(LINK_ORIGIN "add $ORIGIN to RPATH" OFF)

if (OGM_2DARRAY)
    add_definitions(-DOGM_2DARRAY)
endif()

option(OGM_STRUCT_SUPPORT "Allow the use of structs (lightweight objects)" ON)
option(OGM_FUNCTION_SUPPORT "Allow the use of function literals (function declarations)" ON)

if (OGM_STRUCT_SUPPORT)
    add_definitions(-DOGM_STRUCT_SUPPORT)
endif()

if (OGM_FUNCTION_SUPPORT)
    add_definitions(-DOGM_FUNCTION_SUPPORT)
endif()

# 64-bit build (will not support existing project external library linkage)
option(X64 x86_64 OFF)

if (X64)
    add_definitions(-DOGM_X64)
else()
    add_definitions(-DOGM_X32)
endif()

if (RELEASE)
    set(CMAKE_BUILD_TYPE Release)
else()
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (MSVC)
    # 4244: conversion from integer to smaller integer type. This happens a lot in return statements, gcc doesn't care.
    # 4267: conversion of size_t to smaller type. This happens a lot in return statements, gcc doesn't care.
    # 4099: pugixml headers give this warning.
    # 4661: "no suitable definition provided for explicit template initialization request" -- msvc seems to
    #       interpret some template logic in Variable.hpp differently from gcc; hopefully it doesn't matter.
        # 4996: C++17 deprecations  warnings. ThreadPool.h has these warnings.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /we4033 /Zc:externConstexpr /wd4244 /wd4267 /wd4099 /wd4661 /wd4996 /wd4018")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /F60777216 /STACK:60777216")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror=return-type")

    # 32-bit is important for extension operability.
    if(NOT X64)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    else()
        #required by g++ for some reason.
        set(CMAKE_POSITION_INDEPENDENT_CODE True)
    endif()

    if (!WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs")
        add_link_options(-lstdc++fs)
        link_libraries(stdc++fs)
    endif()

    # FIXME: many -W ignores. These should be removed (and any errors resolved.)
    if (!WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror -Wno-sign-compare -Wno-reorder -Wno-switch -Wno-unused-function -Wno-unused-label")
    endif()
    if (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--stack,60777216")
    endif()
endif ()

add_definitions(-DQUEUE_COLLISION_UPDATES)
#add_definitions(-DONLY_RECTANGULAR_COLLISION)
add_definitions(-DOPTIMIZE_CORE)
add_definitions(-DOPTIMIZE_COLLISION)
add_definitions(-DOPTIMIZE_PARSE)
add_definitions(-DOPTIMIZE_STRING_APPEND)
add_definitions(-DCACHE_AST)

# gml2 compile options (since backported into gml1)
add_definitions(-DKEYWORD_ID) # other and self become other.id, self.id

# dear imgui support -- set this to the imgui directory without the trailing /
set(IMGUI "IMGUI_NOTFOUND" CACHE FILEPATH "Imgui directory (optional)")

# networking enabled
option(NETWORKING "sockets" ON)

# std:;filesystem enabled
option(FILESYSTEM "std::filesystem enabled" ON)

if(FILESYSTEM)
    add_definitions(-DCPP_FILESYSTEM_ENABLED)
endif()

# use zugbruecke for running windows DLLs on linux (https://github.com/pleiszenburg/zugbruecke)
option(ZUGBRUECKE "Zugbruecke" OFF)
set(EMBED_ZUGBRUECKE OFF)
set(EMBED_PYTHON3 OFF)

if (ZUGBRUECKE AND UNIX)
    set(EMBED_ZUGBRUECKE ON)
    set(EMBED_PYTHON3 ON)
endif ()

# directory to find PeLoader source (https://github.com/taviso/loadlibrary)
option(PELOADERDIR "PeLoaderDir" OFF)
set(USE_PELOADER 0)

if (PELOADERDIR AND UNIX)
    set(USE_PELOADER 1)
    if (NOT EXISTS "${PELOADERDIR}include")
        message("PeLoader directory ${PELOADERDIR}peloader/ is missing.")
        message("(Did you forget to use the absolute path? Make sure the path ends with a separator / )")
        set(USE_PELOADER 0)
    endif ()

    if (USE_PELOADER)
        message("PeLoader directory is ${PELOADERDIR}")
        link_directories("${PELOADERDIR}peloader")
        include_directories(${PELOADERDIR}peloader)
        add_definitions(-DPELOADER)

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fshort-wchar -Wno-multichar -mstackrealign -Wl,--whole-archive,${PELOADERDIR}peloader/libpeloader.a,--no-whole-archive")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--whole-archive,${PELOADERDIR}peloader/libpeloader.a,--no-whole-archive")
    endif ()
endif ()

add_definitions(-DOGM_GARBAGE_COLLECTOR)

file (GLOB_RECURSE _COMMON_SRC
    "external/fmt/format.cc"
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

file(GLOB_RECURSE _TEST_SRC
    "test/*.cpp"
)

file (GLOB_RECURSE _OGMI_SRC
    "src/interpreter/*.cpp"
    "src/interpreter/*.c"
)

file (GLOB_RECURSE _OGM_ASSET_SRC
    "src/asset/*.cpp"
)

file (GLOB_RECURSE _OGM_RESOURCE_SRC
    "src/resources/*.cpp"
)

set (_IMGUI_SRC "IMGUI_NOTFOUND")

include_directories(include)
include_directories("external/include")

# private includes
include_directories(src)

if(DEFINED ENV{EMSCRIPTEN})

    if (NOT CMAKE_EXECUTABLE_SUFFIX STREQUAL ".js")
        message(FATAL_ERROR "The default suffix for building executables should be .js!\nTry running ./etc/meta/cmake-clean.sh and then build with ./etc/meta/embuild.sh")
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
        ${_OGM_ASSET_SRC}
        ${_OGM_RESOURCE_SRC}
        "external/fmt/format.cpp"
        "external/pugixml/pugixml.cpp"
        "external/base64/base64.cpp"
        "external/md5/md5.cpp"
        "external/simpleini/ConvertUTF.c"
        "external/xbr/xbr.cpp"
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
        ${_OGM_ASSET_SRC}
        ${_OGM_RESOURCE_SRC}
        "external/fmt/format.cpp"
        "external/pugixml/pugixml.cpp"
        "external/md5/md5.cpp"
        "external/xbr/xbr.cpp"
        "external/simpleini/ConvertUTF.c"
        ${_OGM_PROJECT_SRC}
        "external/stb/stb_image.cpp"
        ${_OGMI_SRC}
    )

    set_target_properties(ogm PROPERTIES
            CXX_STANDARD 17
    )
    set_target_properties(ogm-test PROPERTIES
            CXX_STANDARD 17
    )
else()
    if (CMAKE_EXECUTABLE_SUFFIX STREQUAL ".js")
        message(FATAL_ERROR "The default suffix for building executables is set to .js!\nTry running ./etc/meta/cmake-clean.sh")
    endif()

    if (NOT WIN32 OR MSVC)
        if (PARALLEL_COMPILE)
            # couldn't get this to work with mingw32-make for some reason.
            add_definitions(-DPARALLEL_COMPILE)
        endif()
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

    add_library(ogm-asset
      "external/stb/stb_image.cpp"
      "external/xbr/xbr.cpp"
      ${_OGM_ASSET_SRC}
      ${_OGM_RESOURCE_SRC}
    )

    add_library(ogm-project
      "external/pugixml/pugixml.cpp"
      "external/simpleini/ConvertUTF.c"
      ${_OGM_PROJECT_SRC}
    )

    add_library(ogmi
      ${_OGMI_SRC}
      "external/md5/md5.cpp"
      "external/base64/base64.cpp"
    )

    if (NOT "${IMGUI}" MATCHES "IMGUI_NOTFOUND")
        if (NOT NOGRAPHICS)
            file(GLOB _IMGUI_SRC
                "${IMGUI}/*.cpp"
                "${IMGUI}/examples/imgui_impl_opengl3.cpp"
                "${IMGUI}/examples/imgui_impl_sdl.cpp"
            )

            file(GLOB_RECURSE _OGMGUI_SRC
                "src/gui/*.cpp"
            )

            add_definitions(-DIMGUI)

            include_directories("${IMGUI}")
            include_directories("${IMGUI}/examples")

            add_library(ogm-gui
                "${_IMGUI_SRC}"
                "${_OGMGUI_SRC}"
            )
        endif()
    endif()

    # filesystem library
    if (!WIN32)
        target_link_libraries(ogm-common stdc++fs)
    endif()

    # icon (MSVC)
    if (MSVC)
        file (GLOB _OGM_RC
            "*.rc"
        )
    else()
        # empty
        file (GLOB _OGM_RC
            ""
        )
    endif()

    add_executable(ogm "main/main.cpp" "main/unzip.cpp" ${_OGM_RC})
    add_executable(ogm-test
        ${_TEST_SRC}
    )

    # icon (GCC)
    if (NOT MSVC)
        if (WIN32)
            # mingw
            execute_process(
                COMMAND windres ogm.rc -O coff -o ogm.res
                RESULT_VARIABLE maderes
            )
            if (maderes)
                message("WARNING: could not create ogm.res; no icon will be used.")
                message("Return value was ${maderes}")
            else()
                SET_TARGET_PROPERTIES( ogm PROPERTIES LINK_FLAGS "ogm.res" )
            endif()
        endif()
    endif()

    if (MSVC OR WIN32)
        set(PTHREAD "")
        set(SOCKETS "Ws2_32")
    else ()
        set(PTHREAD "pthread")
        set(SOCKETS "")
    endif ()

    set_target_properties(ogmast PROPERTIES CXX_STANDARD 17)
    set_target_properties(ogm-bytecode PROPERTIES CXX_STANDARD 17)
    set_target_properties(ogm-project PROPERTIES CXX_STANDARD 17)
    set_target_properties(ogmi PROPERTIES CXX_STANDARD 17)
    set_target_properties(ogm PROPERTIES CXX_STANDARD 17)
    set_target_properties(ogm-test PROPERTIES CXX_STANDARD 17)

    target_link_libraries(ogmast ogm-common)
    target_link_libraries(ogm-bytecode ogmast  ogm-common)
    target_link_libraries(ogm-project ogm-bytecode ogmast ogm-common ogm-asset ${PTHREAD})
    target_link_libraries(ogmi ogmast ogm-common ogm-asset)
    target_link_libraries(ogm      ogmast ogm-bytecode ogm-beautify ogm-asset ogmi ogm-project ogm-common ${PTHREAD} ${SOCKETS})
    target_link_libraries(ogm-test ogmast ogm-bytecode ogm-beautify ogm-asset ogmi ogm-project ogm-common ${PTHREAD} ${SOCKETS})

    if (NOT "${IMGUI}" MATCHES "IMGUI_NOTFOUND")
        if (NOT NOGRAPHICS)
            target_link_libraries(ogm      ogm-gui)
            target_link_libraries(ogm-test ogm-gui)
        endif()
    endif()

    if (USE_PELOADER)
        target_link_libraries(ogmi peloader)
        target_link_libraries(ogm peloader)
        target_link_libraries(ogm-test peloader)
    endif ()
endif()

### dependencies ###

function(apt_install_advice package)
    if(UNIX AND NOT APPLE)
        message("")
        message("    -- SUGGESTION: apt install " ${package} ":i386")
        message("")
    endif()
endfunction(apt_install_advice)

function(apt_install_advice_header package)
    if(UNIX AND NOT APPLE)
        message("")
        message("    -- SUGGESTION: apt install " ${package})
        message("")
    endif()
endfunction(apt_install_advice_header)

# Python3 and Zugbruecke
if (EMBED_PYTHON3)
    find_library(PYTHON3 NAMES python3.7 python3.6 python3.5 python3.4 python3.3 python3.8 python3.9 python3.10 python3)
    if (PYTHON3)
        target_link_libraries(ogm ${PYTHON3} util)
        target_link_libraries(ogm-test ${PYTHON3} util)
        find_path(PYTHON3_DIR NAMES pyconfig.h)
        find_path(PYTHON3_INCLUDE_DIR NAMES Python.h)
        if(PYTHON3_INCLUDE_DIR)
            if(PYTHON3_DIR)
                include_directories(${PYTHON3_DIR})
                include_directories(${PYTHON3_INCLUDE_DIR})
                add_definitions(-DEMBED_PYTHON3)
                if (EMBED_ZUGBRUECKE)
                    add_definitions(-DEMBED_ZUGBRUECKE)
                endif ()
            else()
                message("Could not find pyconfig.h.")
            endif()
        else()
            message("Could not find Python.h.")
        endif()
    else()
        message("Python3 development library not found. (libpython3.x)")
        if (EMBED_ZUGBRUECKE)
            message("  Windows DLLs will not be available. (Zugbruecke library.)")
        endif ()
        apt_install_advice(python3-dev)
    endif()
endif()

# Open Asset Importer Library
find_library(ASSIMP_LIB assimp)
if(ASSIMP_LIB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASSIMP")
    target_link_libraries(ogm ${ASSIMP_LIB})
    target_link_libraries(ogm-test ${ASSIMP_LIB})
    find_path(ASSIMP_INCLUDE_DIR "assimp/Importer.hpp")
    if (ASSIMP_INCLUDE_DIR)
        include_directories(${ASSIMP_INCLUDE_DIR} SYSTEM)
    endif ()
endif()
if(NOT ASSIMP_LIB OR NOT ASSIMP_INCLUDE_DIR)
    message("WARNING: assimp library not found. Cannot import models.")
endif()

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

# this actually just calls 7z from the command line.
add_definitions(-DLIBZIP_ENABLED)

# nativefiledialog
if (NOT WIN32)
    find_library(NFD_LIBRARY nfd)
    find_path(NFD_INCLUDE_DIR "nfd.h")

    if (NFD_LIBRARY)
        if (NFD_INCLUDE_DIR)
            include_directories("${NFD_INCLUDE_DIR}")
            target_link_libraries(ogm-project ${NFD_LIBRARY})
            target_link_libraries(ogm ${NFD_LIBRARY})
            target_link_libraries(ogm-test ${NFD_LIBRARY})
            add_definitions(-DNATIVE_FILE_DIALOG)
        else()
            message("nfd.h (nativefiledialog) not found.")
            message("Open file dialogs will not be available.")
        endif()
    else()
        message("library nfd (nativefiledialog) not found.")
        message("Open file dialogs will not be available.")
    endif()
endif()

# sockets
if (NETWORKING)
    if (WIN32 AND NOT MSVC)
        find_path(WINSOCK2_INCLUDE_DIR "ws2def.h")
        if (WINSOCK2_INCLUDE_DIR)
            include_directories("${WINSOCK2_INCLUDE_DIR}")
            add_definitions(-DNETWORKING_ENABLED)
        else()
            message("ws2def.h not found; networking disabled.")
            message("(You may need to install the windows SDK.)")
        endif()
    else()
        add_definitions(-DNETWORKING_ENABLED)
    endif()
endif()

# sdl2, opengl, glew, glm
find_library(SDL2_LIBRARY SDL2)
find_library(SDL2_TTF_LIBRARY SDL2_ttf)
find_path(SDL2_INCLUDES "SDL2/SDL.h")
find_path(SDL2_TTF_INCLUDES "SDL2/SDL_ttf.h")
find_library(SDL2_MIXER_LIBRARY SDL2_mixer)
find_path(SDL2_MIXER_INCLUDES "SDL2/SDL_mixer.h")

find_package(OpenGL)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_library(GLEW_LIBRARY NAMES GLEW glew32 glew glew32s PATH_SUFFIXES lib64)

set(SEARCH_FOR_SOUND OFF)

if (DEFINED ENV{EMSCRIPTEN})
  # emscripten has support for these; override above.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGFX_AVAILABLE")
  include_directories("external/emscripten/include")
else()
    # enable testing
    enable_testing()
    add_test(tests ogm-test)

    # find graphics libraries
    if(NOT NOGRAPHICS)
        if(NOT SDL2_LIBRARY OR NOT SDL2_INCLUDES)
            if (NOT SDL2_LIBRARY)
                message("WARNING: SDL2 library not found. Graphics will be disabled.")
            else()
                message("WARNING: SDL2/SDL.h not found. Graphics will be disabled.")
            endif()
            apt_install_advice(libsdl2-dev)
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
                        if(NOT GLM_INCLUDE_DIR)
                            message("WARNING: OpenGLM Mathematics library (GLM) not found. Graphics will be disabled.")
                            apt_install_advice_header(libglm-dev)
                        else()
                            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGFX_AVAILABLE")
                            # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSHDRFX_SUPPORT")
                            set(SEARCH_FOR_SOUND ON)
                            include_directories("${GLM_INCLUDE_DIR}")
                            include_directories("${SDL2_INCLUDES}")
                            include_directories("${GLEW_INCLUDE_DIR}")
                            include_directories("${SDL2_INCLUDES}/SDL2")
                            target_link_libraries(ogm ${SDL2_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY})
                            target_link_libraries(ogm-test ${SDL2_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_gl_LIBRARY})
                            if(NOT SDL2_TTF_LIBRARY OR NOT SDL2_TTF_INCLUDES)
                                if (NOT SDL2_TTF_LIBRARY)
                                    message("WARNING: SDL2_ttf not found. Text will be disabled.")
                                else()
                                    message("WARNING: SDL2/SDL_ttf.h not found. Text will be disabled.")
                                endif()
                                apt_install_advice(libsdl2-ttf-dev)
                            else()
                                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGFX_TEXT_AVAILABLE")
                                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGFX_TEXT_AVAILABLE")
                                target_link_libraries(ogm ${SDL2_TTF_LIBRARY})
                                target_link_libraries(ogm-test ${SDL2_TTF_LIBRARY})
                            endif()
                        endif()
                    endif()
                endif()
            endif()
        endif()
    endif()
endif()

# soloud
add_definitions(-DOGM_SOLOUD)
add_definitions(-DWITH_MINIAUDIO)
add_definitions(-DWITH_NULL)
add_definitions(-DDISABLE_SIMD)
include_directories("external/soloud/include")
file (GLOB_RECURSE _SOLOUD_SRC
    "external/soloud/src/audiosource/*.cpp"
    "external/soloud/src/audiosource/*.c"
    "external/soloud/src/backend/sdl/*.cpp"
    "external/soloud/src/backend/sdl/*.c"
    "external/soloud/src/backend/miniaudio/*.cpp"
    "external/soloud/src/backend/miniaudio/*.c"
    "external/soloud/src/backend/null/*.cpp"
    "external/soloud/src/backend/null/*.c"
    "external/soloud/src/core/*.cpp"
    "external/soloud/src/filter/*.cpp"
)

add_library(soloud
  ${_SOLOUD_SRC}
)

target_link_libraries(ogm soloud)
target_link_libraries(ogm-test soloud)

# sound
if (SEARCH_FOR_SOUND)
    if(NOT SDL2_MIXER_LIBRARY OR NOT SDL2_MIXER_INCLUDES)
        message("WARNING: SDL2_mixer not found. SDL2 will not be used for audio.")
        apt_install_advice("sdl2-mixer-dev")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSFX_AVAILABLE")
        target_link_libraries(ogm ${SDL2_MIXER_LIBRARY})
        target_link_libraries(ogm-test ${SDL2_MIXER_LIBRARY})

        # soloud

        add_definitions(-DWITH_SDL2)
    endif()
else()
    message("Warning: because graphics are disabled, SDL2 will not be used for audio.")
endif()

# Boost
find_package(Boost 1.48.0 COMPONENTS
    filesystem
)
if(Boost_FOUND)
    include_directories("${Boost_INCLUDE_DIRS}")
else()
    message("Failed to find boost v1.48+. Boost is used by fcl, which is an optional add-on.")
endif()

# fcl
find_path(FCL_INCLUDE_DIR "fcl/config.h")
find_path(FCL_AABB_DIR "fcl/BV/AABB.h")
find_path(FCL_ALT_AABB_DIR "fcl/math/bv/AABB.h")
if (FCL_ALT_AABB_DIR AND NOT FCL_AABB_DIR)
    add_definitions(-DOGM_ALT_FCL_AABB_DIR)
endif()
find_path(EIGEN_INCLUDE_DIR Eigen/Dense)
find_library(FCL_LIBRARY NAMES fcl libfcl fcl2 fcl3)
if (FCL_INCLUDE_DIR AND FCL_LIBRARY AND EIGEN_INCLUDE_DIR AND Boost_FOUND AND NOT NO_FCL)
    add_definitions(-DOGM_FCL)
    include_directories("${FCL_INCLUDE_DIR}" "${FCL_INCLUDE_DIR}/.." "${EIGEN_INCLUDE_DIR}")
    target_link_libraries(ogm ${FCL_LIBRARY})
    target_link_libraries(ogm-test ${FCL_LIBRARY})
else()
    message("Warning: FCL/EIGEN not found; 3D collision support disabled.")
endif()

# dynamic external linking
if (WIN32)
    # doesn't need any particular linkage to use LoadLibrary.
else ()
    target_link_libraries(ogm dl)
    target_link_libraries(ogm-test dl)
endif ()

# curl (for HTTP)
find_path(CURL_INCLUDE_DIR "curl/curl.h")
find_library(CURL_LIBRARY NAMES curl)
if (CURL_LIBRARY AND CURL_INCLUDE_DIR)
    add_definitions(-DOGM_CURL)
    include_directories("${CURL_INCLUDE_DIR}")
    target_link_libraries(ogm ${CURL_LIBRARY})
    target_link_libraries(ogm-test ${CURL_LIBRARY})
else()
    message("Warning: libcurl not found; async HTTP requests will be disabled.")
endif()


# windows settings
if (WIN32)
    if (MSVC)
    else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    endif ()

    target_link_libraries(ogm shlwapi)
    target_link_libraries(ogm-test shlwapi)

    # sysinfoapi
    find_path(SYSINFOAPI_INCLUDE "sysinfoapi.h")
    if (SYSINFOAPI_INCLUDE)
        add_definitions(-DSYSINFOAPI_ENABLED)
        include_directories("${SYSINFOAPI_INCLUDE}")
    endif()
endif()

# due to position-dependent code, gig cannot be compiled as a .a file.
add_library(gig SHARED
  ${_GIG_SRC}
)

SET_TARGET_PROPERTIES(gig PROPERTIES PREFIX "")
target_link_libraries(gig ogmast ogm-bytecode)

# this is for appveyor, to build everything from one .vcxproj.
if (MSVC)
    add_dependencies(
        ogm-test
        ogm
    )
    add_dependencies(
        ogm-test
        gig
    )
endif()

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

if(LINK_ORIGIN)
    #very hacky -- find minizip b/c it's the only library not covered by $ORIGIN (?)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so.1" ".so")
    find_library(MINIZIP_LIB minizip)
    add_definitions(-DOGM_MINIZIP_NOOPT)

    target_link_libraries(ogm "-Wl,-rpath '-Wl,\$ORIGIN'" "-Wl,-rpath-link,." ${MINIZIP_LIB})
    target_link_libraries(ogm-test "-Wl,-rpath '-Wl,\$ORIGIN'" ${MINIZIP_LIB})
    target_link_libraries(gig "-Wl,-rpath '-Wl,\$ORIGIN'" ${MINIZIP_LIB})
endif()