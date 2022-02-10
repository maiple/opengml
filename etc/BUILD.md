# Build Instructions

You may build OpenGML as either a 32- or 64-bit application, but 32-bit mode is recommended (and default) to allow support for existing 32-bit extensions. Please install the 32- or 64- bit versions of the dependencies depending on what works for you. (Use `cmake -DX64=ON` to build in 64- bit mode.)

## Linux (with Docker)

The most straightforward way to build OpenGML is with Docker. Run this script:

```
apt install docker
bash ./docker/build.sh ubuntu x86
```

For a convenient development workflow, you can use the [VSCode Remote Container](https://code.visualstudio.com/docs/remote/containers) extension to run VSCode _within_ a docker container. VSCode will automatically mount a volume in the docker container, allowing changes you make within the image to persist in the codebase outside of the docker container. Since there is already a `.devcontainer` folder provided, this should work right out of the box with no fiddling needed.

## Dependencies

### Required

- [Boost Program Options](https://www.boost.org/doc/libs/1_65_1/doc/html/program_options.html)

### Graphics

All the following libraries are required for to have graphics, but optional for CLI projects:

- [OpenGL](https://www.opengl.org/)
- [SDL2](https://www.libsdl.org/)
- [GLEW](http://glew.sourceforge.net/)
- [GLM](https://glm.g-truc.net/0.9.9/index.html)

### Graphics Extensions

- [SDL2 ttf](https://www.libsdl.org/projects/SDL_ttf/) (**required for drawing the default font** and other ttf fonts).
- [SDL2 mixer](https://www.libsdl.org/projects/SDL_mixer/) (**required for audio**)

### 3D extensions

These are optional libraries to facilitate 3D model loading and collision.

- [Open Asset Importer Library](http://assimp.org/) for 3D model import.
- [Flexible Collision Library](https://github.com/flexible-collision-library/fcl) for 3D collision.

### Networking capability

These are also optional

- [libcurl](https://curl.haxx.se/libcurl/) for the HTTP async event.

### DLL support on Linux

- [Python3](https://www.python.org/) with the [Zugbruecke](https://pypi.org/project/zugbruecke/) module (for running Windows DLLs on UNIX systems). See the special instructions below.

## Linux

Install the optional dependencies. If some of these cannot be installed, know that not all of them are strictly necessary,
so don't panic. (The scons output will tell you what's missing.) On Ubuntu, the following commands ought to be sufficient:

```
apt install libglew-dev:i386 libglm-dev:i386 libsdl2-dev:i386 libsdl2-ttf-dev:i386 libsdl2-mixer-dev:i386 libassimp-dev:i386 libfcl-dev:i386 libcurl4-openssl-dev:i386
```

If the 32-bit `:i386` versions fail, you can try the 64-bit versions (leave out the `:i386` suffix on each of the above).

After cloning the repo, run the following commands (see 'Troubleshooting' below if an error occurs):

```
scons .
./build/ogm-test
```

Make sure the unit tests pass. To check that graphics are working, run the example project (and use the arrow keys to move around):

```
./build/ogm demo/projects/example/example.project.gmx
```

### Win32 DLL Support

There are two ways to run windows DLLs. You can use either PeLoader or Zugbruecke. Zugbruecke is recommended as it seems to be much more reliable.

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
scons . --zugbruecke=1
```

There is presently a bug in which DLLs which import other DLLs as dependencies
cannot load the dependency DLL if it is not in the working directory. Until this
is fixed, as a workaround, our suggestion is to **copy all datafile DLLs into the
working directory** (the directory `ogm` is invoked from.)

### Troubleshooting

If scons is having trouble finding a library or header file you require, try adjusting the `LD_LIBRARY_PATH` and `CPATH` environment variables. (On linux, you may wish to add these to your `~/.bashrc` file). You can also try adding `--debug=findlibs` to the scons invocation to view the logs to see where it is looking.

By default, the build is 32-bit instead of 64-bit. This is to allow compatability with
existing extensions. It may crop up as an error finding `bits/c++config.h`, in which case install `g++-multilib` and `gcc-multilib`, or
refer to [StackOverflow](https://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit). You'll also need the 64-bit version of the dependencies, so re-run the above install commands but remove the `:i386` suffixes.

## Windows (MSVC)

The following instructions are for a 32 bit build. (x86.) This is adapted from the file `build.yml` in this repo. We assume that Microsoft Visual Studio Compiler is installed. The 2019 Community edition is known to work.

1. Install the following vcpkg dependencies

```
# required
vcpkg install boost-program-options:x86-windows

# graphics
vcpkg install sdl2:x86-windows && vcpkg install sdl2-image:x86-windows && vcpkg install sdl2-mixer:x86-windows && vcpkg install sdl2-ttf:x86-windows && vcpkg install glew:x86-windows && vcpkg install glm:x86-windows && vcpkg install freeglut:x86-windows

# features
vcpkg install assimp:x86-windows && vcpkg install fcl:x86-windows && vcpkg install boost-filesystem:x86-windows && vcpkg install curl:x86-windows
```

2. Find `vcvars32.bat`. It is likely to be located in `C:\Program Files (x86)\Microsoft Visual Studio\<YEAR>\<VERSION>\VC\Auxilliary\Build\vcvars32.bat`. **In powershell** (not `cmd.exe` -- for some reason it doesn't work!), set the environment variable `MSVC_USE_SCRIPT` to be the path to `vcvars32.bat`, i.e.:

```
$env:MSVC_USE_SCRIPT="C:\Program Files (x86)\Microsoft Visual Studio\<YEAR>\<VERSION>\VC\Auxilliary\Build\vcvars32.bat"
```

3. (In the same terminal), run Scons:

```
scons architecture=x86 .
```

```
scons .
```

### Troubleshooting

If error `32-bit build configured but sizeof(void*) != 4` or `architecture bits detection (32/64) fail` appears, the MSVC is not configured for the correct architecture.
Try running `vcvars32.bat` again or using the [native developer prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160#developer_command_prompt_shortcuts). To debug this, try first setting the environment variable `$env:SCONS_MSCOMMON_DEBUG='-'` in powershell, or `set SCONS_MSCOMMON_DEBUG='-'` in cmd. Also, try running `cl.exe etc/meta/check-32-bits.cpp` and make sure that works.

if compilation succeeds, but running the executable from a mingw context yields this message:

```
ogm.exe: error while loading shared libraries: ?: cannot open shared object file: No such file or directory
```

Try running it from `cmd.exe`, it will tell you what `.dll` is missing. Either add the .dll to your PATH or copy it into the directory with `ogm.exe`.
