# SCons build script. See https://scons.org/doc/production/HTML/scons-user/index.html
# 1. Install scons: https://scons.org/pages/download.html
# 2. To build opengml, simply type `scons` (unquoted) (in the terminal from the repo root directory).

import os
import platform
import sys

args = ARGUMENTS
env = Environment(ENV=os.environ)

# add CPATH to CPPPATH (include directories)
env.Append(CPPPATH=os.environ.get("CPATH", ""))

# add LD_LIBRARY_PATH to LIBPATH (library search path)
env.Append(LIBPATH=os.environ.get("LD_LIBRARY_PATH", ""))

os_windows = env["HOST_OS"] == "win32"

# TODO: is this the correct way to detect MSVC?
msvc = not not (env.get("_MSVC_OUTPUT_FLAG", None))

source_trees_common = ["src", "include", "external"]
source_trees_all = source_trees_common + ["main", "test"]

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

# union of multiple globs
def globs(paths, *args, **kwargs):
  g = []
  for path in paths:
    g += Glob(path, *args, **kwargs)

  # remove duplicates
  return list(set(g))

# -- read command-line args -------------------------------------------------------------------------------------------
def define(*args):
  env.Append(CPPDEFINES=args)

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
  v = dict.get(key, None)
  if v == None:
    return defvalue
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

define_if(opts.array_2d, "OGM_2D_ARRAY")
define_if(opts.structs, "OGM_STRUCT_SUPPORT")
define_if(opts.functions, "OGM_FUNCTION_SUPPORT")
define_if(opts.filesystem, "CPP_FILESYSTEM_ENABLED")

# TODO: zugbruecke
# TODO: peloader

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
  for tree in source_trees_all:
    env.VariantDir(os.path.join(build_dir, tree), tree)
# ---------------------------------------------------------------------------------------------------------------------

# -- static/fixed definitions; change only for debugging purposes. ----------------------------------------------------
define("QUEUE_COLLISION_UPDATES")
define("OPTIMIZE_CORE")
define("OPTIMIZE_COLLISION")
define("OPTIMIZE_PARSE")
define("OPTIMIZE_STRING_APPEND")
define("CACHE_AST")
define("DOGM_GARBAGE_COLLECTOR")
define("DLIBZIP_ENABLED")

# 'other' and 'self' remap to 'other.id' and 'self.id' respectively.
define("KEYWORD_ID")
# ---------------------------------------------------------------------------------------------------------------------

# -- common settings --------------------------------------------------------------------------------------------------
# set include directories
# note: 'src' is private to ogm; when using ogm as a library,
#       'src' includes will not be accessible.
env.Append(CPPPATH=["include", "external/include", "src"])

# source files are all files ending in .c, .cpp, etc.
source_files_common = globs([os.path.join(build_dir, tree, wc) for tree in source_trees_common for wc in ["*.c", "*.cpp"]])
source_files_main = [os.path.join(build_dir, path) for path in ["main.cpp", "unzip.cpp"]]
source_files_test = globs([os.path.join(build_dir, "test", "*.cpp")])

# TODO: install directory
# ---------------------------------------------------------------------------------------------------------------------

# -- compiler-specific settings ---------------------------------------------------------------------------------------
if msvc:
  # C++ standard
  env.Append(CCFLAGS=["/std:c++17"])

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
  source_files_main += ["ogm.rc"]

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

  # C++ standard
  env.Append(CCFLAGS=["-std=c++17"])

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

missing_required_dependency = False

# check if the given dependency is available;
# if not, print a message and possibly error out (if required)
# otherwise, add definition and link library.
def check_dependency(lib, header, language="c", required=False, message=None, defn=None, libname=None):
  global missing_required_dependency
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
      missing_required_dependency = True
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
      define(defn)
    return True
    
# Open Asset Importer Library
check_dependency("assimp", "assimp/Importer.hpp", "cpp", False, "Cannot import models.", "ASSIMP")

# Native FIle Dialogue
check_dependency("nfd", "nfd.h", "c", False, "Open/Save file dialogs will not be available", "NATIVE_FILE_DIALOG")

# Sockets / Networking
if opts.networking and os_windows and not msvc:
  check_dependency(None, "ws2def.h", "c", False, "Networking not available", "NETWORKING_ENABLED")
else:
  define_if(opts.networking, "NETWORKING_ENABLED")

# curl (for HTTP)
check_dependency("curl", "curl/curl.h", "c", False, "async HTTP will be disabled", "OGM_CURL")

# graphics dependencies
if not opts.headless:
  check_dependency("SDL2", "SDL2/SDL.h", "c", True)
  check_dependency("SDL2_ttf", "SDL2/SDL_ttf.h", "c", False, "Text will be disabled", "GFX_TEXT_AVAILABLE")
  check_dependency(["GLEW", "glew32", "glew", "glew32s"], "GL/glew.h", "c", True)
  check_dependency(None, "glm/glm.hpp", "cpp", True)

  # TODO: IMGUI
  pass

  # sound (requires SDL, so not available if headless)
  if opts.sound:
    if check_dependency("SDL2_mixer", "SDL2/SDL_mixer.h", "c", False, "SFX_AVAILABLE"):
      # tell soloud what sound engine to use:
      define("WITH_SDL2")

# TODO: fcl and boost

if missing_required_dependency:
  error("Missing required dependency (see logs above). Aborting.")
  Exit(1)
  assert(False)
conf.Finish()
# ---------------------------------------------------------------------------------------------------------------------

# TODO: cpack