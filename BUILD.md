# Build Instructions

## Dependencies

You may build OpenGML as either a 32- or 64-bit application, but 32-bit mode is recommended to allow support for existing 32-bit extensions.

All the following libraries are required for to have graphics, but optional for CLI projects:

- [OpenGL](https://www.opengl.org/)
- [GLFW3](https://www.glfw.org)
- [GLEW](http://glew.sourceforge.net/)
- [GLM](https://glm.g-truc.net/0.9.9/index.html)

The following libraries are optional:

- [GNU Readline](https://tiswww.case.edu/php/chet/readline/rltop.html) (for the debugger CLI)
- [Python3](https://www.python.org/) with the [Zugbruecke](https://pypi.org/project/zugbruecke/) module (for running Windows DLLs on UNIX systems). See the special isntructions below.

## Linux

Install the optional dependencies. If some of these cannot be installed, know that not all of them are strictly necessary,
so don't panic. (The cmake output will tell you what's missing.) On Ubuntu, the following commands ought to be sufficient:

```
apt install libglfw3-dev:i386 libglew-dev:i386 libglm-dev:i386 libsdl2-dev:i386 libsdl2-ttf-dev:i386 libreadline-dev:i386
```

If the 32-bit `:i386` versions fail, you can try the 64-bit versions (leave out the `:i386` suffix on each of the above).

After cloning the repo, run the following commands (see 'Troubleshooting' below if an error occurs):

```
cmake .
make -j 8
./ogm-test
```

Make sure the unit tests pass. To check that graphics are working, run the example project (and use the arrow keys to move around):

```
./ogm demo/projects/example/example.project.gmx
```

### Win32 DLL Support

There are two ways to run windows DLLs. You can use either PeLoader or Zugbruecke. Zugbruecke is recommended as it seems
to be much more reliable.

#### Zugbruecke

First, install (the 32-bit version of) Python3, then install zugbruecke with pip. Make sure that
you are installing zugbruecke into the 32-bit installation, and not the 64-bit installation if you have one.
On ubuntu, these commands should suffice:

```
apt install libpython3-dev:i386
python3 -m pip install zugbruecke
```

Then build with:

```
cmake . -DZUGBRUECKE=ON
make -j 8
```

#### PeLoader

To use 32-bit windows DLLs (to support some pre-existing extensions), the [PeLoader](https://github.com/taviso/loadlibrary)
library is required, and special build instructions must be followed.

First, download [PeLoader](https://github.com/taviso/loadlibrary). Then run `make CXXFLAGS="-m32"` (assuming a 32-bit installation is desired).
Then for OpenGML, run

```
cmake . -DPELOADERDIR="/path/to/ploader/root/"
make -j 8
```

Replace the path above with the actual path to PeLoader.

### Troubleshooting

If cmake is having trouble finding a library or header file you require, try directing it
with `cmake -DCMAKE_PREFIX_PATH=/path/to/directory/containing/file .`. If this succeeds once,
you should be able to change the prefix path or leave it out on subsequent calls to `cmake`.

By default, the build is 32-bit instead of 64-bit. This is to  allow compatability with
existing extensions. It may crop up as an error finding `bits/c++config.h`, in which case install `g++-multilib` and `gcc-multilib`, or
refer to [StackOverflow](https://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit), or else edit the cmake file to remove the `-m32` flag (at the cost of extension compatability). You'll also need the 64-bit version of the dependencies, so re-run the above install command and remove the `:i386` suffixes.

## Windows

1. Install [CMake](https://cmake.org/download/). Make sure to add it to the PATH. Double check that the CMake GUI is installed.
2. Install [MinGW](https://sourceforge.net/projects/mingw/files/latest/download). Use the default install options.
3. Once MinGW is installed, press "Continue" (or open the MinGW Installation Manager). Select `mingw32-base`, `mingw32-gcc-g++`, and `msys-base`; mark them for installation. Click on `Installation -> Apply Changes` to begin the install. This shouldn't take more than 10 minutes.
4. Wait until everything above is installed. Add `C:\MinGW\bin` to the PATH (assuming you installed it there.)
5. Unless you don't want them, install the prerequisite libraries (OpenGL, GLFW3, GLEW, GLM, GNU Readline). OpenGL may not need to be installed separately. GNU Readline may be troublesome to install on Windows, but it can be ignored if you don't mind a slightly worse debugging experience.
6. Open the CMake GUI. Select the OpenGML repository as the source code and binary build location, then click Generate. If it prompts you to select a toolchain, select the MinGW one. It should output "Configuring Done" and "Generating Done" at the end. If not, an error occurred and you must fix it before proceeding.
7. Open a new command window (`cmd.exe`), navigate to the OpenGML repository, and run `mingw32-make.exe`. This should succeed without error.

Run `ogm-test.exe` to confirm the build is working. If you installed the graphics libraries, check that they work with `ogm.exe demo/projects/example/example.project.gmx` Use `ogm.exe` instead of `ogm` when reading any of the other documentation.

### Troubleshooting

if compilation succeeds, but running the executable from a mingw context yields the message

```
ogm.exe: error while loading shared libraries: ?: cannot open shared object file: No such file or directory
```

Try running it from `cmd.exe`, it will tell you what `.dll` is missing. Either add the .dll to your PATH or copy it into the directory with `ogm.exe`.
