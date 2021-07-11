# Assemble release folder

import os
import sys
import shutil
import glob
import pathlib

target = "ogm_release"

binext = ""
libext = ".so"

_from = "."
if len(sys.argv) >= 2:
    _from = sys.argv[1]

if os.name == 'nt':
    binext = ".exe"
    libext = ".dll"
    
dlibext = "d" + libext

if os.path.exists(target):
    print ("removing existing " + target)
    shutil.rmtree(target)

os.mkdir(target)

def copytree(src, dst):
    print("copytree " + src + " -> " + dst)
    return shutil.copytree(src, dst)
def copy(src, dst):
    print("copy " + src + " -> " + dst)
    return shutil.copy(src, dst)
    
# etc/
os.mkdir(target + "/etc")
endings = ["*.png", "*.gif", "*.ico"]
for ending in endings:
    for file in glob.glob('etc/' + ending):
        copy(file, target + "/etc")

# demo/
copytree("demo", target + "/demo")

# binaries
copy(os.path.join(_from, "ogm" + binext), target)
os.chmod(target + "/ogm" + binext, 777)
copy(os.path.join(_from, "ogm-test" + binext), target)
os.chmod(target + "/ogm-test" + binext, 777)
for file in pathlib.Path(_from).rglob('*' + libext):
    file = str(file)
    if os.path.dirname(file) == target:
        continue
    print("found " + file)
    copy(file, target)
    
# licenses
copy("LICENSE", target + "/LICENSE_opengml")
copy("external/xbr/xbrjs.license", target + "/LICENSE_xbrjs")
copy("external/pugixml/LICENCE.md", target + "/LICENSE_pugixml")
copy("external/include/nlohmann/LICENCE.MIT", target + "/LICENSE_nlohmann")
copy("external/include/rectpack2D/LICENSE.md", target + "/LICENSE_rectpack2d")
copy("external/include/simpleini/LICENCE.txt", target + "/LICENSE_simpleini")
copy("external/include/ThreadPool_zlib_license.txt", target + "/LICENCE_ThreadPool")
copy("external/include/rapidcsv.license", target + "/LICENSE_rapidcsv")
copy("external/include/base64.license", target + "/LICENSE_base64")
copy("external/soloud/LICENSE", target + "/LICENSE_soloud")
copy("external/crossline/LICENSE", target + "/LICENSE_crossline")
