# Voronoi Diagram Tool

A simple performant tool to create [Voronoi Diagrams](https://en.wikipedia.org/wiki/Voronoi_diagram). 

<img src="/images/animation.gif" alt="Animation" width="400">

## Getting Started
### Dependencies
Requires OpenGL, [FreeGLUT](http://freeglut.sourceforge.net/), [GLEW](http://glew.sourceforge.net/), and [GLM](https://glm.g-truc.net/0.9.9/index.html). The FreeGLUT and GLEW DLLs are already included.

### Running
Compile and run using Visual Studio 2017 or later.  You will need to use your own include and linker paths as they will likely differ from mine. Alternatively you can create a makefile and avoid VS.

## Features

Left-click anywhere to create a cell. Left-click and drag a cell to move it. Right-click a cell to delete it. Press the "c" key to clear the screen. Press the "r" key to create a random distribution of 40 cells.

## Roadmap
- Add growing and shrinking features (trivial)
- Anti-aliasing
- General improvements
