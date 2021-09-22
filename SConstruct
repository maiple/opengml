# SCons build script. See https://scons.org/doc/production/HTML/scons-user/index.html
# 1. Install scons: https://scons.org/pages/download.html
# 2. To build opengml, simply type `scons` (unquoted) (in the terminal from the repo root directory).

import os
import platform
import sys
import shutil
from collections import defaultdict
import re

def d(dict, key, defvalue=None):
  return dict[key] if key in dict and dict[key] != None else defvalue

# project info
project_name = "OpenGML"
project_abbreviation = "ogm"
version_major = "0"
version_minor = "8"
version_patch = "0"
version_name = "alpha"
project_description = "Interpreter for GML 1.4"

licenses = {
  "opengml": "LICENSE",
  "xbrjs": "external/xbr/xbrjs.license",
  "pugixml": "external/pugixml/LICENCE.md",
  "nlohmann": "external/include/nlohmann/LICENCE.MIT",
  "rectpack2d": "external/include/rectpack2D/LICENSE.md",
  "simpleini": "external/include/simpleini/LICENCE.txt",
  "ThreadPool": "external/include/ThreadPool_zlib_license.txt",
  "rapidcsv": "external/include/rapidcsv.license",
  "base64": "external/include/base64.license",
  "soloud": "external/soloud/LICENSE",
  "crossline": "external/crossline/LICENSE",
  "libretro": "src/libretro/libretro_h_LICENSE",
}

args = ARGUMENTS
default_target_arch = None

# create scons environment
msvc_use_script = d(os.environ, "MSVC_USE_SCRIPT", d(args, "MSVC_USE_SCRIPT", ""))
if msvc_use_script not in ["", False, None]:
  # MSVC_USE_SCRIPT (which is a SCons construct) apparently sets up environment variables in its
  # own way. If additional environment variables are supplied, MSVC_USE_SCRIPT may
  # conflict with them or fail in a way which is nearly impossible to debug.
  env = Environment(MSVC_USE_SCRIPT = msvc_use_script)
  print(f"Note: MSVC_USE_SCRIPT=\"{os.environ['MSVC_USE_SCRIPT']}\" supplied, so other environment variables will be ignored for the SCons build.")
  if "32.bat" in os.path.basename(msvc_use_script):
    default_target_arch = "x86"
  elif "64.bat" in os.path.basename(msvc_use_script):
    default_target_arch = "x64"
else:
  # if no MSVC use script, then read the environment variables.
  env = Environment(ENV=os.environ)
  msvc_use_script = False # to check falsiness more easily

def plural(count, s1, sp=None):
  return s1 if count == 1 else (sp if sp is not None else (s1 + "s"))

os_is_windows = env["HOST_OS"] == "win32" or platform.system() == "Windows"
os_is_osx = sys.platform == "darwin" or platform.system() == "Darwin"
os_is_linux = not os_is_windows and not os_is_osx # TODO: correct way to identify linux?
os_name = "(unknown os)"
if os_is_windows:
  os_name = "Windows"
if os_is_linux:
  os_name = "Linux"
if os_is_osx:
  os_name = "osx"

# TODO: is this the correct way to detect MSVC?
msvc = not not (
  msvc_use_script or env.get("_MSVC_OUTPUT_FLAG", None)
)
    
pathsplit = re.compile("[:;]" if not os_is_windows else "[;]")

# add CPATH and INCLUDE to CPPPATH (include directories)
env.Append(CPPPATH=pathsplit.split(d(os.environ, "CPATH", "")))
if msvc:
  env.Append(CPPPATH=pathsplit.split(d(os.environ, "INCLUDE", "")))

# add LD_LIBRARY_PATH and LIBPATH and LIB to LIBPATH (library search path)
env.Append(LIBPATH=pathsplit.split(d(os.environ, "LD_LIBRARY_PATH", "")))
if msvc:
  # Paranoia. windows / msvc might not get these by default, so we add them to be safe.
  env.Append(LIBPATH=pathsplit.split(d(os.environ, "LIB", "")))
  env.Append(LIBPATH=pathsplit.split(d(os.environ, "LIBPATH", "")))

# what to scan for source files
source_trees = [
  "src",
  "test",
  "include",
  "external",
]

# -- util functions ---------------------------------------------------------------------------------------------------
# use colored output if it is available.
if not args.get("porcelain", False) and sys.stdout.isatty():
  def warn(s):
    print("\033[0;33m" + str(s) + "\033[0m")
  def error(s):
    print("\033[0;31m" + str(s) + "\033[0m")
  def important(s):
    print("\033[1m" + str(s) + "\033[0m")
else:
  def warn(s):
    print(s)
  def error(s):
    print(s)
  def important(s):
    print(s)

# union of multiple globs; hierarchical
def globs(paths, extensions, *args, **kwargs):
  g = defaultdict(lambda: [])
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

# ---------------------------------------------------------------------------------------------------------------------

# -- read command-line args -------------------------------------------------------------------------------------------
def define(*args, **kwargs):
  env.Append(CPPDEFINES={arg: 1 for arg in args})
  env.Append(CPPDEFINES=kwargs)

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
opts.architecture = d(args, "architecture", d(args, "arch", default_target_arch or d(os.environ, "TARGET_ARCH", d(os.environ, "VSCMD_ARG_TGT_ARCH", platform.machine())))).lower()
opts.release = d(args, "release", False)
opts.appimage = d(args, "appimage", False)
opts.zugbruecke = d(args, "zugbruecke", False)
opts.install = d(args, "install", False) # set to either a bool/int ('1') or a directory
opts.install_directory = None
opts.build_libretro = d(args, "libretro", False)
if opts.install and type(opts.install) == type("") and len(opts.install) > 1 and not opts.install.isdigit():
  if opts.install.lower() in ["on", "true", "yes"]: # safety
    error("to install to default directory, use argument \"install=1\" instead")
    Exit(1)
  else:
    opts.install_directory = opts.install
if opts.install and not opts.install_directory:
  if os_is_linux:
    opts.install_directory = "/usr"
  else:
    error("please specify a directory to install to (e.g. /usr)")
    Exit(1)

opts.deb = d(args, "deb", False)
if opts.deb and opts.install:
  error("deb and install are mutually exclusive")

opts.array_2d = d(args, "array2d", False)
# disable parallel build support in ogm by default if using mingw32 on windows, as for some reason it doesn't work.
opts.parallel_build = d(args, "parallel-compile", msvc or not os_is_windows)
opts.headless = d(args, "headless", False)
opts.sound = d(args, "sound", True)
opts.structs = d(args, "structs", True)
opts.functions = d(args, "functions", True)
opts.networking = d(args, "sockets", True) # networking enabled
opts.filesystem = d(args, "filesystem", True) # std::filesystem enabled
opts.linktest = d(args, "linktest", d(args, "link-test", False)) # if true, build only the linker test executable.

define_if(opts.array_2d, "OGM_2D_ARRAY")
define_if(opts.structs, "OGM_STRUCT_SUPPORT")
define_if(opts.functions, "OGM_FUNCTION_SUPPORT")
define_if(opts.filesystem, "CPP_FILESYSTEM_ENABLED")
define_if(opts.zugbruecke, "EMBED_ZUGBRUECKE")

# build summary
important(
  ("Release" if opts.release else "Debug")
  + (" linktest" if opts.linktest
      else (" install" if opts.install
      else (" debian package" if opts.deb else " build")))
  + " for " + opts.architecture + " " + os_name
  + ("" if not opts.install else (" to " + opts.install_directory))
)

if opts.install and not opts.release:
  warn("Warning: installing debug build.")

if opts.deb and not opts.release:
  warn("Warning: packaging debug build.")

if opts.deb and not os_is_linux:
  error("debian package (.deb) can only be built for linux platform")
  Exit(1)

if opts.linktest and (opts.deb or opts.appimage or opts.install):
  error("linktest is mutually exclusive with other packaging options.")
  Exit(1)

# set full project name (OpenGML a.b.c (...))
project_full_name = None
if not opts.linktest:
  project_full_name = f"OpenGML {version_major}.{version_minor}.{version_patch}"
  project_appends = (
    ([version_name] if version_name else [])
    + (["debug"] if not opts.release else [])
  )
  if len(project_appends) > 0:
    project_full_name += f" ({', '.join(project_appends)})"
  important(project_full_name)
  env.Append(CPPDEFINES={"VERSION":project_full_name})

# release macros / debug macros
if opts.release:
  env.Append(
    CPPDEFINES=['RELEASE', 'NDEBUG'],
    CCFLAGS=['-O2' if not msvc else "/O2"]
  )
else:
  env.Append(
    CPPDEFINES=['DEBUG'],
    CCFLAGS=['-g' if not msvc else "/DEBUG"],
    LINKFLAGS=['-g' if not msvc else "/DEBUG"]
  )

# architecture
deb_depends = []
dep_architecture = None
vcpkg_arch = ""
if opts.architecture in ["x86", "i386"]:
  deb_architecture = "i386"
  architecture = "i386"
  vcpkg_arch = "x86"
  if not msvc:
    env.Append(CCFLAGS="-m32", LINKFLAGS="-m32")
  define("OGM_X32")
elif opts.architecture in ["x86_64", "x64", "i686", "amd64"]:
  deb_architecture = "amd64"
  architecture = "x86_64"
  vcpkg_arch = "x64"
  if not msvc:
    env.Append(CCFLAGS="-m64", LINKFLAGS="-m64")
  define("OGM_X64")
else:
  error("architecture not supported: " + str(opts.architecture))
  error("please use: scons arch=<ARCHITECTURE>")
  Exit(1)

# add some common default places to look for libraries
library_name_suffixes = [""] # suffixes to add before extensions, but after library name
if not d(os.environ, "NO_LD_LIBRARY_PATH_ADDITIONS"):
  if os_is_linux:
      env.Append(LIBPATH=[
        "/usr/lib",
        "/usr/local/lib",
        "/usr/lib/x86_64-linux-gnu",
        "/usr/local/lib/x86_64-linux-gnu",
        "/usr/lib/i386-linux-gnu",
        "/usr/local/lib/i386-linux-gnu",
      ])
  elif os_is_osx:
    env.Append(CPPPATH=["/usr/include", "/usr/local/include", "/usr/local/Cellar"])
    env.Append(LIBPATH=["/usr/lib", "/usr/local/lib", "/usr/local/Cellar"])
  elif os_is_windows:
    # find vcpkg (optional)
    vcpkg_dirs = []
    vcpkg_libsuffix_re = re.compile("(-vc[0-9]*)(-[mg][td])*") # (see usage)
    vcpkg_triplet = vcpkg_arch + "-windows"
    for path in pathsplit.split(d(os.environ, "PATH", "") + ";" + d(os.environ, "VCPKG_ROOT", "")):
      if os.path.exists(os.path.join(path, "vcpkg.exe")) and os.path.exists(os.path.join(path, "installed", vcpkg_triplet, "bin")):
        vcpkg_installed = os.path.join(path, "installed", vcpkg_triplet)
        vcpkg_dirs.append(vcpkg_installed)
        env.Append(LIBPATH=os.path.join(vcpkg_installed, "bin"))
        env.Append(LIBPATH=os.path.join(vcpkg_installed, "lib"))
        env.Append(CPPPATH=os.path.join(vcpkg_installed, "include"))
        
        # vcpkg names some libraries with things like -vcXXX-mt-gd.dll
        for name in list(os.listdir(os.path.join(vcpkg_installed, "bin"))) + list(os.listdir(os.path.join(vcpkg_installed, "lib"))):
          m = vcpkg_libsuffix_re.search(name)
          if m:
            s = m.group()
            if s not in library_name_suffixes:
              library_name_suffixes.append(s)
        

# set build directory
if build_dir != "." and (not os.path.exists(build_dir) or not os.path.samefile(build_dir, ".")):
  os.makedirs(build_dir, exist_ok=True)

  # perform build within build/ instead of within tree,
  # and copy files as needed to there.
  for tree in source_trees:
    env.VariantDir(os.path.join(build_dir, tree), tree)

# debian package info
if opts.deb:
  deb_revision = version_patch
  deb_package_name = f"{project_abbreviation}-{version_major}.{version_minor}_{deb_revision}_{deb_architecture}"
  important("debian package: " + deb_package_name)
  
  # hijack install to package folder
  opts.install = True
  opts.deb_directory = deb_package_name
  opts.install_directory = os.path.join(opts.deb_directory, "usr")

  # paranoid safety check to ensure we don't delete anything important
  assert len(opts.deb_directory) > 3
  if os.path.exists(opts.deb_directory):
    assert not os.path.samefile(opts.deb_directory, ".")

  # ensure package root dire is new and empty
  if os.path.isdir(opts.deb_directory):
    shutil.rmtree(opts.deb_directory)
  os.makedirs(opts.deb_directory, exist_ok=False)

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
define("OGM_CROSSLINE")

# 'other' and 'self' remap to 'other.id' and 'self.id' respectively.
define("KEYWORD_ID")
# ---------------------------------------------------------------------------------------------------------------------

# -- license information ----------------------------------------------------------------------------------------------
license_text = ""
license_dependencies = []
for license, license_path in licenses.items():
  license_text += "===== " + license + " =====\n\n"
  license_dependencies += [os.path.join(license_path)]
  with open(os.path.join(license_path), "r", encoding="utf8") as f:
    license_text += f.read() + "\n\n"

# create license include for C++
with open(os.path.join("src", "common", "license.inc"), "w", encoding="utf8") as f:
  f.write(
    f"// This file was created automatically by SCons\n\nstatic const char* _ogm_license_ = R\"OGMSTR({license_text})OGMSTR\";"
  )

# create combined license in 'out' folder
with open(os.path.join(build_dir, "LICENSE"), "w", encoding="utf8") as f:
  f.write(license_text)
  
# ---------------------------------------------------------------------------------------------------------------------

# -- common settings --------------------------------------------------------------------------------------------------
# set include directories
# note: 'src' is private to ogm; when using ogm as a library,
#       'src' includes will not be accessible.
env.Append(CPPPATH=["src", "include", "external/include", "external/soloud/include"])

# source files are all files ending in .c, .cpp, etc.
source_files = globs(source_trees, ["*.c", "*.cpp", "*.cc"])

# ---------------------------------------------------------------------------------------------------------------------

# -- compiler-specific settings ---------------------------------------------------------------------------------------
if msvc:
  # C/C++ standard
  env.Append(CXXFLAGS=["/std:c++17"])

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
  
  # use /MD (static release) rather than dynamic
  # (we don't want to have to ship vcredist separately)
  # NOTE TO USER: if this causes problems, remove it.
  env.Append(CCFLAGS=["/MT"])

  # stack size
  env.Append(
    CCFLAGS=["/F60777216"],
    LINKFLAGS=["/STACK:60777216"]
  )

  # unwind semantics
  env.Append(
    CCFLAGS=["/EHsc"]
  )

  # permits use of strcpy
  define("_CRT_SECURE_NO_WARNINGS")

  # icon rc file
  # create ogm.rc and add etc/icon/ogm.ico to build
  ogm_rc_path = "ogm.rc"
  env.VariantDir(os.path.join(build_dir, "etc", "icon"), os.path.join("etc", "icon"))
  if not os.path.exists(ogm_rc_path):
    with open("ogm_rc_path", "w") as f:
      f.write("id ICON \"etc/icon/ogm.ico\"\n")
  source_files["src/main"] += [ogm_rc_path]

  # sockets support
  env.Append(LIBS=["Ws2_32", "comdlg32", "User32", "Shlwapi"])
else:
  # gcc and clang

  # C++ standard
  env.Append(CXXFLAGS=["-std=c++17"])

  # warn if non-void function is missing a return
  env.Append(CCFLAGS=["-Werror=return-type"])

  # <!> Unknown why this is required by g++.
  env.Append(CCFLAGS=["-fpic"], LINKFLAGS=["-fpic"])

  if not os_is_windows:
    # pthread support
    env.Append(LIBS=["pthread"])

    # enable opening shared libraries dynamically
    env.Append(LIBS=["dl"])

  if os_is_linux:
    # <!> Unknown why this is required
    env.Append(LIBS=["stdc++fs"])
  
  if os_is_windows:
    # mingw
    env.Append(
      CCFLAGS=["-static-libgcc", "-static-libstdc++"],
      LINKFLAGS=["-static-libgcc", "-static-libstdc++"],
      LIBS=["shlwapi"]
    )
    # TODO: mingw set icon (windres ogm.rc)

# For some reason, some osx builds won't work with fmt unless header only.
fmt_header_only = os_is_osx
if fmt_header_only:
  env.Append(CCFLAGS=["-DFMT_HEADER_ONLY"])

# ---------------------------------------------------------------------------------------------------------------------

# -- check for required and optional library dependencies -------------------------------------------------------------
conf = Configure(env.Clone() if msvc else env)

# this will be set to true later if a required dependency is not found.
missing_required_dependencies = []
missing_debian_packages = [] # only used if deb mode

def elf_architecture_matching(elfpath):
  # TODO: verify not only bits (elf32/elf64) but architecture too (amd/arm, etc)
  elfh = os.popen(f"readelf -h {elfpath}").read()
  if deb_architecture == "i386" and "ELF32" in elfh:
    return True
  if deb_architecture == "amd64" and "ELF64" in elfh:
    return True
  return False

# get debian package matching library name
def find_deb_dependency(libname):
  assert os_is_linux and opts.deb
  found_static_lib = False
  dynamic_lib_path = None
  for path in [path for p in d(env, "LIBPATH", "") for path in pathsplit.split(p)]:
    # check for dynamic library
    libpath_so = os.path.join(path, f"lib{libname}.so")
    if os.path.exists(libpath_so) and elf_architecture_matching(libpath_so):
      dynamic_lib_path = libpath_so
      break

    # check for static library
    libpath_a = os.path.join(path, f"lib{libname}.a")
    if not found_static_lib and os.path.exists(libpath_a):
      found_static_lib = elf_architecture_matching(libpath_a)
  
  if not dynamic_lib_path:
    if found_static_lib:
      return False
    else:
      missing_debian_packages.append(libname)
      return False
  
  # follow symlinks
  org_dynamic_lib_path = dynamic_lib_path
  for i in range(100):
    if not os.path.islink(dynamic_lib_path):
      break
    prev_dynamic_lib_path = dynamic_lib_path
    readlink = os.readlink(prev_dynamic_lib_path)
    if readlink is None or readlink == "":
      break
    dynamic_lib_path = readlink
    if not os.path.isabs(dynamic_lib_path):
      dynamic_lib_path = os.path.join(os.path.dirname(prev_dynamic_lib_path), dynamic_lib_path)
    if prev_dynamic_lib_path == dynamic_lib_path:
      break
  else:
    warn("failed to resolve relative simlink: " + org_dynamic_lib_path)
    return False
  
  # get package contianing library
  package = os.popen(f"dpkg -S {dynamic_lib_path}").read().strip()
  if package and len(package) > 0 and ":" in package:
    deb_depends.append(package.split(":")[0])
    print(f"{libname} found in package {package}")
    return True
  return False
  
# check if the given dependency is available;
# if not, print a message and possibly error out (if required)
# otherwise, add definition and link library.
def find_dependency(lib, header, language="c", required=False, message=None, defn=None, libname=None, **kwargs):
  assert (lib or header)
  found_lib = False
  libtypesuffix = "LIBSUFFIX" if "force_shared" not in kwargs or not kwargs["force_shared"] else "SHLIBSUFFIX"
  if type(lib) == type(""):
    lib = [lib]
  if type(lib) == type([]):
    assert (len(lib) > 0)
    for l in lib:
      for suffix in library_name_suffixes:
        # try searching for static libraries by specific path first.
        # we prefer to link against static libraries to improve portability.
        for paths in env["LIBPATH"]:
          if msvc:
            for path in pathsplit.split(paths):
              if path.strip() != "":
                path = path.strip()
                libpath = os.path.join(path, l + suffix + env[libtypesuffix])
                if os.path.exists(libpath):
                  if conf.CheckLib(libpath):
                    lib = libpath
                    found_lib = True
                    break
          if found_lib:
            break
        else:
          # try searching without specific path
          if conf.CheckLib(l + suffix):
            lib = l + suffix
            found_lib = True
            break
      if found_lib:
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
      if lib not in env["LIBS"]:
        env.Append(LIBS=[lib])
      if opts.deb:
        find_deb_dependency(lib)
    if defn:
      # cpp definition that library is enabled
      define(*(defn if type(defn) == type([]) else [defn]))
    return True
    
# Open Asset Importer Library (assimp)
find_dependency("assimp", "assimp/Importer.hpp", "cpp", False, "Cannot import models.", "ASSIMP")

# Flexible Collision Library (fcl)
if find_dependency(["fcl", "fcl2", "fcl3"], "fcl/config.h", "cpp", False, "3D collision extension will be disabled"):
  # fcl's API is constantly in flux. We check for various versions here...
  if conf.CheckCXXHeader("fcl/BV/AABB.h"):
    define(OGM_FCL=500)
  elif conf.CheckCXXHeader("fcl/math/bv/AABB.h"):
    define(OGM_FCL=600)
  else:
    warn("Neither fcl/BV/AABB.h nor fcl/math/bv/AABB.h could be located. This version of fcl is unrecognized. 3D collision extension will be disabled.")
  
  


# Native File Dialogue
if find_dependency("nfd", "nfd.h", "c", False, "Open/Save file dialogs will not be available", "NATIVE_FILE_DIALOG"):
  # nfd also requires gtk on linux
  if os_is_linux:
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
if opts.networking and os_is_windows and not msvc:
  find_dependency(None, "ws2def.h", "c", False, "Networking not available", "NETWORKING_ENABLED")
else:
  define_if(opts.networking, "NETWORKING_ENABLED")

# curl (for HTTP)
find_dependency(["curl", "curld", "curl-d", "libcurl"], "curl/curl.h", "c", False, "async HTTP will be disabled", "OGM_CURL", force_shared=True)

# graphics dependencies
if not opts.headless:
  find_dependency("SDL2", "SDL2/SDL.h", "c", True, force_shared=True)
  find_dependency("SDL2_ttf", "SDL2/SDL_ttf.h", "c", False, "Text will be disabled", "GFX_TEXT_AVAILABLE", force_shared=True)
  if os_is_linux:
    find_dependency("GL", None, None, True)
  elif os_is_osx:
    env.Append(LINKFLAGS="-framework OpenGL")
  if os_is_windows:
    find_dependency(["glut32", "freeglut", "glut", "freeglut32"], "gl/glut.h", "c", True)
    find_dependency(["opengl32", "opengl"], None, None, True, force_shared=True)
    find_dependency(["glu32", "glu"], None, None, True)
  find_dependency(["GLEW", "glew32", "glew", "glew32s"], "GL/glew.h", "c", True, force_shared=True)
  find_dependency(None, "glm/glm.hpp", "cpp", True)

  # (nota bene: some of the above dependencies are required)
  define("GFX_AVAILABLE")

  # TODO: IMGUI (for gui/ support)

  # sound (requires SDL, so not available if headless)
  if opts.sound:
    find_dependency("SDL2_mixer", "SDL2/SDL_mixer.h", "c", False, "Sound will be disabled", ["SFX_AVAILABLE", "OGM_SOLOUD"])

if len(missing_required_dependencies) > 0:
  error(
    f"Missing required {plural(len(missing_required_dependencies), 'dependency', 'dependencies')}: \""
    + "\" , \"".join(missing_required_dependencies) + "\""
  )
  error("note: to debug this error, try using the flag --debug=findlibs and reading the log file produced.")
  if os_is_windows and not d(os.environ, "NO_LD_LIBRARY_PATH_ADDITIONS"):
    if len(vcpkg_dirs) == 0:
      error("Note: no vcpkg directory could be found. If you are using vcpkg, please ensure it is on the path.")
    else:
      error("Note: vcpkg directory: " + ", ".join(vcpkg_dirs))
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
  # sorting is important so that scons doesn't detect the dependency order has changed.
  return sorted(list(set(source_files[os.path.join(*args)])))

def outname(name):
  return os.path.join(build_dir, name)

if opts.linktest:
  # link test (just for checking that all the libraries and includes are found)
  linktest = env.Program(
    outname("ogm-linktest"),
    os.path.join(build_dir, "test", "link_test.cpp")
  )
  env.Default(linktest)

if architecture == "i386":
  bittest = env.Program(
    outname("ogm-bittest"),
    os.path.join("etc", "meta", "check-32-bits.cpp")
  )
  env.Default(bittest)

module_sources = _()

# ogm-common
module_sources.ogm_common =  \
  sources("src", "common") + \
  ([] if fmt_header_only else sources("external", "fmt"))
ogm_common = env.StaticLibrary(
  outname("ogm-common"),
  module_sources.ogm_common
)

# ogm-sys
module_sources.ogm_sys = sources("src", "sys")
ogm_sys = env.StaticLibrary(
  outname("ogm-sys"),
  module_sources.ogm_sys,
)

# ogm-ast
module_sources.ogm_ast = sources("src", "ast")
ogm_ast = env.StaticLibrary(
  outname("ogm-ast"),
  module_sources.ogm_ast,
)

# ogm-bytecode
module_sources.ogm_bytecode = sources("src", "bytecode")
ogm_bytecode = env.StaticLibrary(
  outname("ogm-bytecode"),
  module_sources.ogm_bytecode,
)

# ogm-beautify
module_sources.ogm_beautify = sources("src", "beautify")
ogm_beautify = env.StaticLibrary(
  outname("ogm-beautify"),
  module_sources.ogm_beautify,
)

# ogm-asset
module_sources.ogm_asset =     \
  sources("src", "asset")    + \
  sources("src", "resource") + \
  sources("external", "stb") + \
  sources("external", "xbr")
ogm_asset = env.StaticLibrary(
  outname("ogm-asset"),
  module_sources.ogm_asset,
)

# ogm-project
module_sources.ogm_project =             \
  sources("src", "project")            + \
  sources("simpleini", "ConvertUTF.c") + \
  sources("external", "pugixml")
ogm_project = env.StaticLibrary(
  outname("ogm-project"),
  module_sources.ogm_project,
)

# ogm-interpreter
module_sources.ogm_interpreter =  \
  sources("src", "interpreter") + \
  sources("external", "md5")    + \
  sources("external", "base64") + \
  sources("external", "crossline")
ogm_interpreter = env.StaticLibrary(
  outname("ogm-interpreter"),
  module_sources.ogm_interpreter,
)

# all ogm libraries required to execute ogm code.
ogm_execution_libs = [
  ogm_interpreter,
  ogm_project,
  ogm_asset,
  ogm_bytecode,
  ogm_beautify,
  ogm_ast,
  ogm_sys,
  ogm_common
]

# soloud
if opts.sound and not opts.headless:
  # we only require a very specific set of functionality from soloud, so
  # we are very precise here about what source files to use.
  module_sources.soloud =                                          \
    sources("external", "soloud", "src", "audiosource")          + \
    sources("external", "soloud", "src", "backend", "sdl")       + \
    sources("external", "soloud", "src", "backend", "miniaudio") + \
    sources("external", "soloud", "src", "backend", "null")      + \
    sources("external", "soloud", "src", "core")                 + \
    sources("external", "soloud", "src", "filter")
  soloud = env.StaticLibrary(
    outname("soloud"),
    module_sources.soloud,
    CPPDEFINES=["WITH_MINIAUDIO", "WITH_NULL", "WITH_SDL2", "DISABLE_SIMD"]
  )
  ogm_execution_libs += soloud
else:
  module_sources.soloud = []
  
ogm = env.Program(
  outname("ogm"),
  sources("src", "main"),
  LIBS=ogm_execution_libs + env["LIBS"]
)

ogm_test = env.Program(
  outname("ogm-test"),
  # don't put link_test in test build, as it is its own thing. (TODO: should it have its own build directory..?), 
  list(filter(lambda source : "link_test.cpp" not in str(source), sources("test"))),
  LIBS=ogm_execution_libs + env["LIBS"]
)

# gig (shared library for use within ogm projects)
if os_is_windows:
  # we can reuse .obj files on windows for .dlls
  gig = env.SharedLibrary(
    outname("gig"),
    sources("src", "gig") ,
    LIBS=[ogm_common, ogm_ast, ogm_bytecode],
    SHLIBPREFIX="" # remove 'lib' prefix
  )
else:
  # we can't reuse .o files for shared libraries on unix systems.
  gig = env.SharedLibrary(
    outname("gig"),
    sources("src", "gig") 
    + module_sources.ogm_common
    + module_sources.ogm_ast
    + module_sources.ogm_bytecode,
    SHLIBPREFIX="" # remove 'lib' prefix
  )
  
# libretro core
if opts.build_libretro:
  if os_is_windows:
    # we can reuse .obj files on windows for .dlls
    libretro_opengml = env.SharedLibrary(
      outname("retro_opengml"),
      sources("src", "libretro"),
      LIBS=ogm_execution_libs + env["LIBS"]
    )
  else:
    libretro_opengml = env.SharedLibrary(
      outname("retro_opengml"),
      sources("src", "libretro")
      + module_sources.ogm_common
      + module_sources.ogm_ast
      + module_sources.ogm_sys
      + module_sources.ogm_bytecode
      + module_sources.ogm_asset
      + module_sources.ogm_project
      + module_sources.ogm_interpreter
      + module_sources.soloud,
    )

# build all targets that are in build_dir
# (this is actually only important to specify due
# to the additional call to Default when installing)
if not opts.linktest:
  env.Default(build_dir)

# ---------------------------------------------------------------------------------------------------------------------

# -- install (optional) -----------------------------------------------------------------------------------------------
ogm_install = None
if opts.install: 
  def get_prefix_build_path(targets):
    paths = [env.GetBuildPath(target) for target in targets]
    if len(paths) == 1:
      return paths[0]
    else:
      return os.path.commonpath(*paths)

  def print_install(targets, installtargets):
    important(f"install {os.path.basename(get_prefix_build_path(targets))} to {get_prefix_build_path(installtargets)}")

  ogm_install = env.Install(os.path.join(opts.install_directory, "bin"), ogm)
  print_install(ogm, ogm_install)

  # TODO: install headers and library

  env.Default(ogm_install)

# ---------------------------------------------------------------------------------------------------------------------

# required for deb and appimage:
desktop_entry = """[Desktop Entry]
Name=OpenGML
Comment=Interpreter for GML1.4
Exec=%%exec%%
Icon=%%icon%%
Categories=Development
Type=Application
Keywords=ogm;maker;gml;game
"""

# -- deb package (optional) -------------------------------------------------------------------------------------------

deb_package = None
if opts.deb:
  assert ogm_install
  # package  info
  if len(missing_required_dependencies) > 0:
    error("Could not identify debian packages for these libraries: " + ", ".join(missing_required_dependencies))
    Exit(1)
  if len(deb_depends) == 0:
    error("no debian dependencies detected -- this is almost certainly an error.")
    Exit(1)
  # add debian packages specified on the command line
  deb_depends += d(os.environ, "OGM_DEB_REQUIREMENTS", "").split(":")
  deb_depends = list(filter(lambda x : len(x.strip()) > 0, deb_depends))
  print("package dependencies are: ", *deb_depends)
  if os.path.isdir(opts.deb_directory):
    shutil.rmtree(opts.deb_directory)
  os.makedirs(os.path.join(opts.deb_directory, "DEBIAN"), exist_ok=True)
  os.makedirs(os.path.join(opts.deb_directory, "usr", "share", "applications"), exist_ok=True)
  os.makedirs(os.path.join(opts.deb_directory, "usr", "share", "icons"), exist_ok=True)

  # dep package control file
  # this is required by dpkg-deb to create the deb package
  # note in particular the 'Depends' field, which is a list of 
  # (debian package) dependencies
  with open(os.path.join(opts.deb_directory, "DEBIAN", "control"), "w") as f:
    control_str = "\n".join([
      f"Package: {project_abbreviation}",
      f"Version: {version_major}.{version_minor}",
      f"Architecture: {deb_architecture}",
      f"Maintainer: Maiple <mairple@gmail.com>",
      f"Description: {project_description}",
      f"Depends: {', '.join(deb_depends)}" if len(deb_depends) > 0 else ""
    ]) + "\n"
    print("control file contents:\n" + control_str)
    f.write(control_str)
  
  # .desktop file enables ogm to appear in the start menu and similar menus
  # specification: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#recognized-keys
  with open(os.path.join(opts.deb_directory, "usr", "share", "applications", "ogm.desktop"), "w") as f:
    f.write(
      desktop_entry.replace("%%exec%%", "/usr/bin/ogm --popup").replace("%%icon%%", "/usr/share/icons/ogm.png")
    )
  shutil.copyfile(os.path.join("etc", "icon", "ogm.png"), os.path.join(opts.deb_directory, "usr", "share", "icons", "ogm.png"))
  
  # deb package builder
  assert ogm_install
  out_deb = opts.deb_directory + ".deb"
  deb_package = env.Command(
    out_deb, [opts.deb_directory, ogm_install],
    f"dpkg-deb --build {opts.deb_directory} {out_deb}"
  )
  env.Default(deb_package)

# ---------------------------------------------------------------------------------------------------------------------

# -- appimage (optional) ----------------------------------------------------------------------------------------------
# create an AppDir folder, then run linuxdeploy
if opts.appimage:
  # recreate AppDir
  appdir = os.path.join(build_dir, "AppDir")
  if os.path.exists(appdir):
    shutil.rmtree(appdir)
  os.makedirs(appdir, exist_ok=True)

  # create desktop file
  with open(os.path.join(appdir, "ogm.desktop"), "w") as f:
    f.write(desktop_entry.replace("%%exec%%", "ogm").replace("%%icon%%", "ogm"))

  # download linuxdeploy (a tool which creates appimages)
  wget_linuxdeploy = env.Command(
    f"linuxdeploy-{architecture}.AppImage", [],
    f"wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-{architecture}.AppImage && chmod u+x linuxdeploy-{architecture}.AppImage"
  )
  appimage_name = f"ogm-{version_major}.{version_minor}.{version_patch}-{architecture}.AppImage"
  env["OUTPUT"] = appimage_name
  ogm_appimage = env.Command(
    "ogm.AppImage", [ogm],
    f"./linuxdeploy-{architecture}.AppImage --appdir {appdir} -e {env.GetBuildPath(ogm)[0]} -i etc/icon/ogm.png -d {os.path.join(appdir, 'ogm.desktop')} --output appimage"
  )
  ogm_appimage_rename = env.Command(
    appimage_name, [ogm_appimage, ogm],
    f"mv OpenGML*.AppImage {appimage_name}"
  )
  env.Default(ogm_appimage_rename)
