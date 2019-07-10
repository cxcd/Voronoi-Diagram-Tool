# Voronoi Diagram Tool

A simple performant tool to create [Voronoi Diagrams](https://en.wikipedia.org/wiki/Voronoi_diagram). 

<img src="/images/Screenshot1.png" alt="Screenshot" width="400">

## Usage
### Dependencies
Requires OpenGL, FreeGLUT, GLEW, and GLM. The FreeGLUT and GLEW DLLs are already included.

### Running
Compile and run using Visual Studio 2017 or later.  You will need to use your own include and linker paths as they will likely differ from mine. Alternatively you can create a makefile and avoid VS.

### Features

Left-click anywhere to create a cell. Left-click and drag a cell to move it. Right-click a cell to delete it. Press the "c" key to clear the screen. Press the "r" key to create a random distribution of 40 cells.

## Roadmap
- Add growing and shrinking features (trivial)
- Anti-aliasing
- General improvements
