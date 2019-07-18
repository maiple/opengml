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

Install the optional dependencies. On Ubuntu, the following commands will work:

```
apt install libreadline-dev
```

After cloning the repo, run the following commands:

```
cmake .
make
./ogm-test
```

Make sure the unit tests pass.

## Windows

1. Install [CMake](https://cmake.org/download/). **Make sure to add it to the PATH**.
2. Install [MinGW](https://sourceforge.net/projects/mingw/files/latest/download). Use the default install options.
3. Once MinGW is installed, press "Continue" (or open the MinGW Installation Manager). Select `mingw32-base`, `mingw32-gcc-g++`, and `msys-base`; mark them for installation. Click on `Installation -> Apply Changes` to begin the install. This shouldn't take more than 10 minutes.
4. Wait until everything above is installed. Add `C:\MinGW\bin` to the PATH (assuming you installed it there.)
5. Open the CMake GUI. Select this repository as the source code and binary build location, then click Generate. It should output "Configuring Done" and "Generating Done" at the end. If not, an error occurred and you must fix it before proceeding.
6. Open a new command window (`cmd.exe`), navigate to this repository, and run `mingw32-make`. This should succeed without error.

Run `ogm-test.exe` to confirm the build is working. Use `ogm.exe` instead of `ogm` hereafter.
