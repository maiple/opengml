# Assemble release folder
# usage:
# python3 assemble-release.py
# python3 assemble-release.py from-dir [dst-dir] [build-from-subdir]

import os
import sys
import shutil
import glob
import pathlib

# folder in which to place release
target = "ogm-release"

# directory to import from
_from = "."

# folder relative to _from which contains built binaries
build_dir = os.path.join(_from, "build")

# replace above with clargs
if len(sys.argv) >= 2:
    _from = sys.argv[1]

if len(sys.argv) >= 3:
    target = sys.argv[2]

if len(sys.argv) >= 4:
    build_dir = os.path.join(_from, sys.argv[3])

if not os.path.isdir(_from) or not os.path.isdir(build_dir):
    print("'from' directory invalid (check that it is a directory and contains a build/ subfolder)")
    exit(1)

# folder relative to _from which contains libraries to copy in
lib_dir = os.path.join(_from, "libs")

# platform-dependent binary/library names
binext = ""
libext = ".so"
if sys.platform == "win32" or os.name == 'nt' or sys.platform == "cygwin":
    binext = ".exe"
    libext = ".dll"
elif sys.platform == "darwin":
    # mac os
    binext = ""
    libext = ".dylib"

# remove existing target directory and recreate it
if os.path.exists(target):
    print ("removing existing " + target)
    shutil.rmtree(target)
os.makedirs(target)

# verbose versions (to explain what is happening)
def copytree(src, dst):
    print("copytree " + src + " -> " + dst)
    return shutil.copytree(src, dst)
def copy(src, dst):
    print("copy " + src + " -> " + dst)
    return shutil.copy(src, dst)
    
# sparce copy etc/ folder
# etc/
os.mkdir(target + "/etc")
endings = ["*.png", "*.gif", "*.ico"]
for ending in endings:
    for file in glob.glob('etc/' + ending):
        copy(file, target + "/etc")

# demo/
copytree("demo", target + "/demo")

# license
copy(os.path.join(build_dir, "LICENSE"), os.path.join(target, "LICENSE"))

# binaries (including gig)
copy(os.path.join(build_dir, "ogm" + binext), target)
os.chmod(target + "/ogm" + binext, 777)
copy(os.path.join(build_dir, "ogm-test" + binext), target)
os.chmod(target + "/ogm-test" + binext, 777)
copy(os.path.join(build_dir, "gig" + libext), target)

# copy any desired shared libraries
if os.path.exists(lib_dir):
    print(f"searching for additional libraries in {lib_dir}...")
    any_found = False
    for file in pathlib.Path(lib_dir).rglob('*' + libext):
        file = str(file)
        # skip gig
        if os.path.basename(file).startswith("gig."):
            continue
        # don't copy what's already in the target/ dir
        if os.path.dirname(file) == target:
            continue
        print("found " + file)
        copy(file, target)
        any_found = True
    if not any_found:
        print("(none found.)")