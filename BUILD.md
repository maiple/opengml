# Build Instructions

## Dependencies

All the following libraries are required for to have graphics, but optional for CLI projects:

- [OpenGL](https://www.opengl.org/)
- [GLFW3](https://www.glfw.org)
- [GLEW](http://glew.sourceforge.net/)
- [GLM](https://glm.g-truc.net/0.9.9/index.html)

The following libraries are optional:

- [GNU Readline](https://tiswww.case.edu/php/chet/readline/rltop.html) (for the debugger CLI)

## Linux

Install the optional dependencies. On Ubuntu, the following commands ought to be sufficient:

```
apt install libreadline-dev libglfw3-dev libglew-dev libglm-dev
```

After cloning the repo, run the following commands:

```
cmake .
make
./ogm-test
```

Make sure the unit tests pass. To check that graphics are working, run the example project (and use the arrow keys to move around):

```
./ogm demo/projects/example/example.project.gmx
```

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
