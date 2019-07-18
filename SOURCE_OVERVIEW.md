# Source Code Overview

Directory information

- `demo/`: contains test gml files.
- `etc/`: contains miscellanea not related to the code, including images for these readme files.
- `include/`: header files for `ogm` and the other libraries included in this repo (`pugixml`, `catch2`).
- `src/`: contains .cpp files and some private header files for the implementations of the header files in `include/`.
- `pugixml/`: contains implementation for [pugixml](https://pugixml.org/), an XML parser library used by OpenGML.
- `test/`: contains unit test code, which can be launched after compiling with `./ogm-test`.

The bulk of the code is shared between `include/` and `src/`, containing the `.hpp` and `.cpp` files respectively. These are broken up into a number of packages (containing the headers and implementation for these respectively), each of which compiles into its own library (or is header-only).

## Packages

The following packages exist in one or both of `include/` and `src/`.

These are the most important packages:

- `ast/`: Parses GML code strings into an [Abstract syntax tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree).
- `bytecode/`: Defines the [bytecode](https://en.wikipedia.org/wiki/Bytecode) format. Compiles an AST into bytecode and disassembles bytecode into string form.
- `interpreter/`: executes the bytecode, running an instance of a project.
  - `interpreter/library` (C): contains the implementations of all the built-in library functions (like `show_debug_message`)
  - `interpreter/ds` (H)
  - `interpreter/display`: Totally encapsulated graphics/display routines based on [glfw3](https://www.glfw.org/)
- `main/`: invokes the above packages according to user-provided command line arguments.

There are these additional packages as well:

- `common/` (H): some common base code shared between all the packages. Needs to be cleaned up a bit.
- `asset/`: Defines the data structures for compiled resource assets as understood by the compiler and interpreter.
- `collision/` (H): used by the interpreter for collision detection.
- `geometry/` (H): defines general-purpose geometry types (vector, bounding box, etc.) used by collision detection and the interpreter.
- `project/`: a wrapper for `ast/` and `bytecode/` that strings together compilation for a whole GML project containing multiple files and resources.
- `gig/` (C): GML bindings for invoking OpenGML from within GML (so you can GML while you GML). Stands for *GML in GML*.

(H): header-only; see `include/ogm/`.
(C): cpp-only; see `src/`.

More detail is gone into each of these packages below. The dependencies between the packages are made explicit in [this document](PACKAGE_DEPENDENCIES.md).

## ast

*dependencies: none*

Parses GML code in string form, producing an abstract syntax tree.


## asset

*dependencies: none*

Defines the static data for assets which is relevant to the
bytecode compiler and/or runtime environment.

For example, data such as the name or numerical index of the asset is included, as this is
necessary for both the compiler and runtime environment. However, the source code for an
object is *not* included -- this is relevant to the
`project/` package, but not to the compiler (`bytecode/`) nor the runtime environment
(`interpreter/`).

## bytecode

*dependencies: ast, asset*

- Defines the bytecode format 
- provides a disassembler, which converts bytecode to a (relatively) human-readable string.
- provides a compiler, which converts an AST (from `ast`) into bytecode.

Some additional context must also be provided in order to compile/disassemble bytecode.
- A Library object must be provided to the compiler and disassembler in order to compile/disassemble
native function calls (e.g. `show_debug_message`).
- Config is required, but there are currently no configuration options available.
- A ReflectionAccumulator object *may* be provided in order to direct and obtain
  instance and local variable names.
- An AssetTable *may* be provided in order to provide the definitions of assets.

## collision

*dependencies: asset*

Performs collision detection over a set of `Shape`s.

Depends on asset, because some of the `Shape`s may be sprite / image data.

## interpreter

*dependencies: asset, bytecode, collision*

Executes bytecode.

## project

*dependencies: ast, asset, bytecode*

Parses, stores, and compiles the source code and resource data for a whole project.

This is different from the `bytecode/` package, because the `bytecode/` package
only compiles snippets of code at a time.

The output is an AssetTable and BytecodeTable which can be executed by
the interpreter.

## gig

*dependencies: ast, asset, bytecode*

Provides recursive bindings to OGM, allowing projects compiled in OGM to run OGM.