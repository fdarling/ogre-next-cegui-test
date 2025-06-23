# Overview

This is a demo project that is attempting to integrate the 3D graphics library Ogre Next v4.0 with a GUI overlay provided by CEGUI v0.8.7. This is a crucial element for making video games that have menu systems, debug screens, etc.

The project is written in C++ using CMake as the build system, and has been tested on Debian 12 (bookworm) Linux. Currently, some X11 specific code is being used to pass the window "handle" from SDL2 to Ogre, but besides that it should be relatively cross-platform because of using SDL2 and OpenGL.

# Dependencies

* CMake (tested with v3.25.1)
* GCC (tested with v12.2.0)
* Ogre Next v4.0 (v3.0 might also work?)
* CEGUI (tested with v0.8.7)
* GLEW (tested with v2.2)
* SDL2 (tested with v2.26.5)
* Open Asset Import Library aka "assimp" (tested with v5.2.5)

## Ogre Next

Here is how to install Ogre Next v4.0 (the master branch on git) locally, such that it is installed inside the user folder rather than system-wide.

NOTE: Ogre Next v3.0 might also work, but I was having trouble setting it up and decided to go with the master branch as a result.

Installing the Ogre Next dependencies required for compiling it:

```
sudo apt install libfreeimage-dev libxaw7-dev libxrandr-dev libgl1-mesa-dev
```

NOTE: the above dependencies will implicitly pull in these other dependencies that Ogre Next requires:

* `libx11-dev`
* `libxt-dev`

Then we must clone the repository:

```
git clone --recursive https://github.com/OGRECave/ogre-next.git
```

And finally within the cloned directory, we can build and install using `cmake`:

```
mkdir build
cmake .. -DCMAKE_INSTALL_PREFIX=~/apps/ogre-next
cmake --build . --config Release --target install
```

## CEGUI

CEGUI v0.8.7 is what is included with Debian 12 (bookworm):

```
sudo apt install libcegui-mk2-dev
```

## Open Asset Import Library

Installing under Debian 12 (bookworm):

```
sudo apt install libassimp-dev
```

## GLEW

NOTE: GLEW is required for CEGUI's OpenGL3 backend to be used in a project

Installing under Debian 12 (bookworm):

```
sudo apt install libglew-dev
```

## SDL2

Installing under Debian 12 (bookworm):

```
sudo apt install libsdl2-dev
```

# Compiling

```
cmake -DCMAKE_PREFIX_PATH=/home/MYUSERNAME/apps/ogre-next -DCMAKE_MODULE_PATH=/home/MYUSERNAME/apps/ogre-next/lib/OGRE-Next/cmake/ ..
```

NOTE: you must "hard-code" the paths and not use the tilde (~) symbol, as it doesn't evaluate properly.

# Running

```
./OgreNextCEGUIDemo
```

NOTE: it is assumed that you are inside of a `build` type directory within the main project folder, therefore you can access assets using a relative path `../data`:

NOTE: it is assumed that Ogre Next was installed to `~/apps/ogre-next`, and this project builds that path using the Linux APIs to get the home directory location.

# The Problem

If you disable the following `#define` symbol by commenting out the following line in `main.cpp`:

```cpp
#define ENABLE_CEGUI_CONTEXT
```

You can run the program, albeit with some glitchy rendering artifacts, in particular when mousing over the window decorations on Linux.

However, if you leave that line enabled (not commented), then you'll get a segmentation fault!
