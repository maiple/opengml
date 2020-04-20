# Quickstart Guide

This guide will get you up and running with OpenGML.

## Step 1: Get OpenGML

If you haven't yet, [download OpenGML](https://github.com/maiple/opengml/releases), or build it from source with [these instructions](./BUILD.md).

You will need a terminal program to run OpenGML. On Windows, `cmd.exe` or [Git SCM](https://git-scm.com/) should work. Make sure that OpenGML is on your PATH. (Instructions: [Windows](https://docs.microsoft.com/en-us/previous-versions/office/developer/sharepoint-2010/ee537574(v=office.14)), [Linux](https://askubuntu.com/questions/109381/how-to-add-path-of-a-program-to-path-environment-variable)).

If you are using windows, replace `ogm` with `ogm.exe` in all the following commands.

## Step 2: Verify OpenGML

Ensure that the following command displays the OpenGML version information:

```
ogm --version
```

If this does not work, return to step 1. Don't forget to type `ogm.exe` instead of `ogm` if you are using Windows.

To check that graphics are working, run the example game:

```
ogm ./demo/projects/example/example.project.gmx
```

Proceed to either of the following headers depending on what your goal is.

## Run an existing GML project

If you already have a GML project you would like to run in OpenGML, the following command should launch it:

```
ogm /path/to/MyProject.project.gmx
```

Assuming this works and there are no errors, you're done.

## Starting a new GML project

If you are new to GML or want to start a new project, this remainder of this guide will walk you through setting up a minimal example project.

OpenGML can interpret whole GML projects or just individual scripts. Let's start with just a single script.

### Writing a script

Create a file called `myscript.gml` and write the following line in it:

```
show_debug_message("Hello, World!");
```

If you then run `ogm myscript.gml`, you should see the message `Hello, World!`.

### A more complicated script

TBD

### Setting up a project

Look at how the project located at `demo/projects/example` is structured. (TODO / TBD)
