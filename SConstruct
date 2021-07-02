# SCons build script. See https://scons.org/doc/production/HTML/scons-user/index.html
# 1. Install scons: https://scons.org/pages/download.html
# 2. To build opengml, simply type `scons` (unquoted) (in the terminal from the repo root directory).

import os
import platform
import sys
from typing import DefaultDict

args = ARGUMENTS
env = Environment(ENV=os.environ)

def plural(count, s1, sp=None):
  return s1 if count == 1 else (sp if sp is not None else (s1 + "s"))

os_windows = env["HOST_OS"] == "win32"
os_linux = not os_windows # TODO: correct way to identify linux?

# TODO: is this the correct way to detect MSVC?
msvc = not not (env.get("_MSVC_OUTPUT_FLAG", None))

if os_linux:
  # add CPATH to CPPPATH (include directories)
  env.Append(CPPPATH=os.environ.get("CPATH", "").split(":"))

  # add LD_LIBRARY_PATH to LIBPATH (library search path)
  env.Append(LIBPATH=os.environ.get("LD_LIBRARY_PATH", "").split(":"))

source_trees = [
  "src",
  "test",
  "include",
  "external",
]

# use colored output if it is available.
if not args.get("porcelain", False) and sys.stdout.isatty():
  def warn(s):
    print("\033[0;33m" + str(s) + "\033[0m")
  def error(s):
    print("\033[0;31m" + str(s) + "\033[0m")
else:
  def warn(s):
    print(s)
  def error(s):
    print(s)

# union of multiple globs; hierarchical
def globs(paths, extensions, *args, **kwargs):
  g = DefaultDict(lambda: [])
  for path in paths:
    for ext in extensions:
      g[path] += env.Glob(os.path.join(build_dir, path, ext), *args, **kwargs)
      for root, dirs, files in os.walk(path):
        for dir in dirs:
            files = env.Glob(os.path.join(build_dir, root, dir, ext), *args, **kwargs)
            # add recursively to each of the directories
            key = os.path.join(root, dir)
            while len(key) > 0:
              lkey = len(key)
              g[key] += files
              key = os.path.dirname(key)
              # stop if basename is not able to reduce length.
              if len(key) >= lkey:
                break
  
  return g

# -- read command-line args -------------------------------------------------------------------------------------------
def define(*args):
  env.Append(CPPDEFINES={arg: 1 for arg in args})

def define_if(a, *args):
  if a:
    define(*args)

# read default build directory
build_dir = args.get("build-dir", "build")
assert type(build_dir) == type(""), "invalid path for build directory: " + str(build_dir)

# store some command line options
class _:
  pass
opts = _()
def d(dict, key, defvalue=None):
  return dict[key] if key in dict and dict[key] != None else defvalue
opts.architecture = d(args, "architecture", d(args, "arch", d(env, "TARGET_ARCH", d(env, "HOST_ARCH", platform.processor()))))
opts.release = d(args, "release", False)
opts.array_2d = d(args, "array2d", False)
# disable parallel build support in ogm by default if using mingw32 on windows, as for some reason it doesn't work.
opts.parallel_build = d(args, "parallel-compile", msvc or not os_windows)
opts.headless = d(args, "headless", False)
opts.sound = d(args, "sound", True)
opts.structs = d(args, "structs", True)
opts.functions = d(args, "functions", True)
opts.networking = d(args, "sockets", True) # networking enabled
opts.filesystem = d(args, "filesystem", True) # std::filesystem enabled
opts.linktest = d(args, "linktest", False) # if true, build only the linker test executable.

define_if(opts.array_2d, "OGM_2D_ARRAY")
define_if(opts.structs, "OGM_STRUCT_SUPPORT")
define_if(opts.functions, "OGM_FUNCTION_SUPPORT")
define_if(opts.filesystem, "CPP_FILESYSTEM_ENABLED")

# build summary
print("Release" if opts.release else "Debug", "build,", opts.architecture)

# release or debug
if opts.release:
  env.Append(
    CPPDEFINES=['RELEASE', 'NDEBUG'],
    CCFLAGS=['-O2']
  )
else:
  env.Append(
    CPPDEFINES=['DEBUG'],
    CCFLAGS=['-g']
  )

# architecture
if opts.architecture in ["x86", "i386"]:
  env.Replace(TARGET_ARCH="x86")
  define("OGM_X32")
elif opts.architecture in ["x86_64", "x64", "i686"]:
  env.Replace(TARGET_ARCH="x86_64")
  define("OGM_X64")
else:
  assert False, "architecture not supported: " + str(opts.architecture)

# set build directory
if build_dir != "." and (not os.path.exists(build_dir) or not os.path.samefile(build_dir, ".")):
  os.makedirs(build_dir, exist_ok=True)

  # perform build within build/ instead of within tree,
  # and copy files as needed to there.
  for tree in source_trees:
    env.VariantDir(os.path.join(build_dir, tree), tree)
# ---------------------------------------------------------------------------------------------------------------------

# -- static/fixed definitions; change only for debugging purposes. ----------------------------------------------------
define("QUEUE_COLLISION_UPDATES")
define("OPTIMIZE_CORE")
define("OPTIMIZE_COLLISION")
define("OPTIMIZE_PARSE")
define("OPTIMIZE_STRING_APPEND")
define("CACHE_AST")
define("OGM_GARBAGE_COLLECTOR")
define("LIBZIP_ENABLED")

# 'other' and 'self' remap to 'other.id' and 'self.id' respectively.
define("KEYWORD_ID")
# ---------------------------------------------------------------------------------------------------------------------

# -- common settings --------------------------------------------------------------------------------------------------
# set include directories
# note: 'src' is private to ogm; when using ogm as a library,
#       'src' includes will not be accessible.
env.Append(CPPPATH=["src", "include", "external/include", "external/soloud/include"])

# source files are all files ending in .c, .cpp, etc.
source_files = globs(source_trees, ["*.c", "*.cpp", "*.cc"])

# TODO: install directory
# ---------------------------------------------------------------------------------------------------------------------

# -- compiler-specific settings ---------------------------------------------------------------------------------------
if msvc:
  # C/C++ standard
  env.Append(CXXFLAGS=["/std:c++17"])
  env.Append(CFLAGS=["/std:c17"])

  # 4244: conversion from integer to smaller integer type. This happens a lot in return statements, gcc doesn't care.
  # 4267: conversion of size_t to smaller type. This happens a lot in return statements, gcc doesn't care.
  # 4099: pugixml headers give this warning.
  # 4661: "no suitable definition provided for explicit template initialization request" -- msvc seems to
  #       interpret some template logic in Variable.hpp differently from gcc; hopefully it doesn't matter.
  # 4996: C++17 deprecations  warnings. ThreadPool.h has these warnings.
  env.Append(CCFLAGS=[
    "/we4033", "/Zc:externConstexpr", "/wd4244", 
    "/wd4267", "/wd4099", "/wd4661", "/wd4996", "/wd4018",
    
  ])

  # stack size
  env.Append(CCFLAGS=["/F60777216", "/STACK:60777216"])

  # permits use of strcpy
  define("_CRT_SECURE_NO_WARNINGS")

  # icon
  source_files["src/main"] += ["ogm.rc"]

  # TODO: mingw set icon (windres ogm.rc)

  if msvc:
    # sockets support
    env.Append(LIBS=["Ws2_32"])
  else:
    # mingw
    env.Append(
      CCFLAGS=["-static-libgcc", "-static-libstdc++"],
      LIBS=["shlwapi"]
    )
else:
  # gcc and clang

  # C/C++ standard
  env.Append(CXXFLAGS=["-std=c++17"])
  env.Append(CFLAGS=["-std=c17"])

  # warn if non-void function is missing a return
  env.Append(CCFLAGS=["-Werror=return-type"])

  # <!> Unknown why this is required by g++.
  env.Append(CCFLAGS=["-fpic"])

  if not os_windows:
    # pthread support
    env.Append(LIBS=["pthread"])

    # enable opening shared libraries dynamically
    env.Append(LIBS=["dl"])

    # <!> Unknown why this is required
    env.Append(LIBS=["stdc++fs"])
# ---------------------------------------------------------------------------------------------------------------------

# -- check for required and optional library dependencies -------------------------------------------------------------
conf = Configure(env)

# this will be set to true later if a required dependency is not found.
missing_required_dependencies = []

# check if the given dependency is available;
# if not, print a message and possibly error out (if required)
# otherwise, add definition and link library.
def check_dependency(lib, header, language="c", required=False, message=None, defn=None, libname=None):
  assert (lib or header)
  found_lib = False
  if type(lib) == type([]):
    assert (len(lib) > 0)
    for l in lib:
      if conf.CheckLib(l):
        lib = l
        found_lib = True
        break
    else:
      lib = "\" or \"".join(lib)
  else:
    found_lib = conf.CheckLib(lib) if lib else True
  found_header = (conf.CheckCHeader(header) if language == "c" else conf.CheckCXXHeader(header)) if header else True
  if not found_lib or not found_header:
    s = ""
    missing = ""
    if not found_lib and not found_header:
      missing = "both library and header(s)"
    elif not found_lib:
      missing = "library"
    elif not found_header:
      missing = "header(s)"
    else:
      assert False
    libname = libname if libname else (lib if lib else os.path.basename(header))
    if required:
      missing_required_dependencies.append(libname)
      s = f"ERROR: missing {missing} for required dependency \"{libname}\""
    else:
      s = f"WARNING: missing {missing} for optional dependency \"{libname}\""
    if message:
      s += " -- " + message
    if required:
      error(s)
    else:
      warn(s)

    return False
  else:
    if lib:
      # link library
      env.Append(LIBS=[lib])
    if defn:
      # cpp definition that library is enabled
      define(*(defn if type(defn) == type([]) else [defn]))
    return True
    
# Open Asset Importer Library (assimp)
check_dependency("assimp", "assimp/Importer.hpp", "cpp", False, "Cannot import models.", "ASSIMP")

# Flexible Collision Library (fcl)
if check_dependency(["fcl", "fcl2", "fcl3"], "fcl/config.h", "cpp", False, "3D collision extension will be disabled"):
  if conf.CheckCXXHeader("fcl/BV/AABB.h"):
    define("OGM_FCL")
  elif conf.CheckCXXHeader("fcl/math/bv/AABB.h"):
    define("OGM_ALT_FCL_AABB_DIR")
    define("OGM_FCL")
  else:
    warn("Neither fcl/BV/AABB.h nor fcl/math/bv/AABB.h could be located. This version of fcl is unrecognized. 3D collision extension will be disabled.")

# Native File Dialogue
if check_dependency("nfd", "nfd.h", "c", False, "Open/Save file dialogs will not be available", "NATIVE_FILE_DIALOG"):
  # nfd also requires gtk on linux
  if os_linux:
    if conf.CheckProg("pkg-config"):
        linkflags = os.popen("pkg-config --cflags --libs gtk+-3.0").read()
        for flag in linkflags.split():
          if flag.startswith("-l"):
            env.Append(
              LIBS=flag[2:]
            )
    else:
      warn("WARNING: pkg-config not found; gtk-3 dependency for Native File Dialogue may not be linked correctly.")

# Sockets / Networking
if opts.networking and os_windows and not msvc:
  check_dependency(None, "ws2def.h", "c", False, "Networking not available", "NETWORKING_ENABLED")
else:
  define_if(opts.networking, "NETWORKING_ENABLED")

# curl (for HTTP)
check_dependency("curl", "curl/curl.h", "c", False, "async HTTP will be disabled", "OGM_CURL")

# graphics dependencies
if not opts.headless:
  # all these dependencies are required and will halt the build
  # if not found:
  check_dependency("SDL2", "SDL2/SDL.h", "c", True)
  check_dependency("SDL2_ttf", "SDL2/SDL_ttf.h", "c", False, "Text will be disabled", "GFX_TEXT_AVAILABLE")
  check_dependency(["GLEW", "glew32", "glew", "glew32s"], "GL/glew.h", "c", True)
  check_dependency(None, "glm/glm.hpp", "cpp", True)

  define("GFX_AVAILABLE")

  # TODO: IMGUI (for gui/ support)

  # sound (requires SDL, so not available if headless)
  if opts.sound:
    check_dependency("SDL2_mixer", "SDL2/SDL_mixer.h", "c", False, "Sound will be disabled", ["SFX_AVAILABLE", "OGM_SOLOUD"])

if len(missing_required_dependencies) > 0:
  error(
    f"Missing required {plural(len(missing_required_dependencies), 'dependency', 'dependencies')}: \""
    + "\" , \"".join(missing_required_dependencies) + "\""
  )
  Exit(1)
  assert(False)

conf.Finish()
# ---------------------------------------------------------------------------------------------------------------------

# TODO: cpack

# -- compile ----------------------------------------------------------------------------------------------------------

# returns source files in directories given by args (and recursively all subdirectories thereof)
# e.g. sources("src", "ast") -> all source files
def sources(*args):
  # remove duplicates and return 
  return list(set(source_files[os.path.join(*args)]))

def outname(name):
  return os.path.join(build_dir, name)

if opts.linktest:
  # link test (just for checking that all the libraries and includes are found)
  define("OGM_LINK_TEST")
  env.Program(
    outname("ogm-linktest"),
    os.path.join(build_dir, "test", "link_test.cpp")
  )
else:
  # ogm-common
  ogm_common = env.StaticLibrary(
    outname("ogm-common"),
    sources("src", "common") +
    sources("external", "fmt")
  )

  # ogm-ast
  ogm_ast = env.StaticLibrary(
    outname("ogm-ast"),
    sources("src", "ast"),
  )

  # ogm-bytecode
  ogm_bytecode = env.StaticLibrary(
    outname("ogm-bytecode"),
    sources("src", "bytecode"),
  )

  # ogm-beautify
  ogm_beautify = env.StaticLibrary(
    outname("ogm-beautify"),
    sources("src", "beautify")
  )

  # ogm-asset
  ogm_asset = env.StaticLibrary(
    outname("ogm-asset"),
    sources("src", "asset") +
    sources("src", "resource") +
    sources("external", "stb") +
    sources("external", "xbr")
  )

  # ogm-project
  ogm_project = env.StaticLibrary(
    outname("ogm-project"),
    sources("src", "project") +
    sources("simpleini", "ConvertUTF.c") +
    sources("external", "pugixml"),
  )

  # ogm-interpreter
  ogm_interpreter = env.StaticLibrary(
    outname("ogm-interpreter"),
    sources("src", "interpreter") +
    sources("external", "md5") +
    sources("external", "base64"),
  )

  # all ogm libraries required to execute ogm code.
  ogm_execution_libs = [
    ogm_interpreter,
    ogm_project,
    ogm_asset,
    ogm_bytecode,
    ogm_beautify,
    ogm_ast,
    ogm_common
  ]

  # soloud
  if opts.sound and not opts.headless:
    # we only require a very specific set of functionality from soloud, so
    # we are very precise here about what source files to use.
    soloud = env.StaticLibrary(
      outname("soloud"),
      sources("external", "soloud", "src", "audiosource") +
      sources("external", "soloud", "src", "backend", "sdl") +
      sources("external", "soloud", "src", "backend", "miniaudio") +
      sources("external", "soloud", "src", "backend", "null") +
      sources("external", "soloud", "src", "core") +
      sources("external", "soloud", "src", "filter"),
      CPPDEFINES=["WITH_MINIAUDIO", "WITH_NULL", "WITH_SDL2", "DISABLE_SIMD"]
    )
    ogm_execution_libs += soloud

  env.Program(
    outname("ogm"),
    sources("src", "main"),
    LIBS=ogm_execution_libs + env["LIBS"]
  )

  env.Program(
    outname("ogm-test"),
    sources("test"),
    LIBS=ogm_execution_libs + env["LIBS"]
  )

  # gig (shared library for use within ogm projects)
  env.SharedLibrary(
    outname("gig"),
    sources("src", "gig"),
    LIBS=[ogm_common, ogm_ast, ogm_bytecode],
    SHLIBPREFIX="" # remove 'lib' prefix
  )