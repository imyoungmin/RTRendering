# Real-Time High Quality Rendering Project 0
By Luis Ángel  (임 영민) - All rights reserved (c) 2019
www.youngmin.com.mx

## Functionality

This OpenGL 4.1 base code creates a GLFW window and renders on it a variety of predefined solid geometries, any 3D object model 
(provided in an `*.obj` format) you indicate, plus dots and lines.  You may set the color of your objects and rotate the scene with both
mouse dragging and the arrow keys.  To exit the window, just close it in its appropriate UI button or hit the `ESC` key.

We currently support the **Blinn-Phong Reflectance Model** using GLSL 4.1 and render text using FreeType and textures.

All of the fonts, shaders, and 3D object models must be located in a `Resources` directory, and you should provide its path in the 
`Configuration.h` header file.

## Requirements

The code has been tested for macOS 10.13 (High Sierra), and requires the following libraries to be installed 
under `/usr/local/lib` (with their respective `include` directories in `/usr/local/include`):
- GLFW `https://www.glfw.org/download.html`
- LibPNG `https://sourceforge.net/projects/libpng/files/`
- FreeType `https://sourceforge.net/projects/freetype/files/`
- Armadillo `http://arma.sourceforge.net/download.html`

The above libraries may be built using `./configure` - `make` - `sudo make install`, except *GLFW* which requires 
CMake to be installed (`https://cmake.org/download/`).

You may create a project using the *CLion* (`https://www.jetbrains.com/cpp/`), which requires XCode to be installed 
in your macOS system.

If you create the project on *XCode*, make sure to add the `OpenGL`  framework and the libraries `GLFW`, `FreeType`, and `Armadillo` to
your target in the project configuration settings.
