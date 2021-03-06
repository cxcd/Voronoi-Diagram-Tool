# Voronoi Diagram Tool

A simple performant tool to create 2D [Voronoi Diagrams](https://en.wikipedia.org/wiki/Voronoi_diagram). This program computes an approximation of the Euclidean distance using intersecting cones.

<img src="/images/animation.gif" alt="Animation" width="400">

## Getting Started
### Dependencies
Requires OpenGL, [FreeGLUT](http://freeglut.sourceforge.net/), [GLEW](http://glew.sourceforge.net/), and [GLM](https://glm.g-truc.net/0.9.9/index.html). The FreeGLUT and GLEW DLLs are already included.

### Running
Compile and run using Visual Studio 2017 or later.  You will need to use your own include and linker paths as they will likely differ from mine. Alternatively you can create a makefile and avoid VS.

## Usage
- **Left-click** anywhere to create a cell. 
- **Left-click** and drag a cell to move it, and **right-click** a cell to delete it. 
- Press the **c** key to clear the screen. 
- Press the **r** key to create a random distribution of 40 cells. You can change that number by changing ```randomAmount = 40``` to your desired amount. 
- Press the **m** key to set all the cells to their minimum radius, and the **n** key to set the cells to their maximum radius. These can be changed by changing the ```minConeRadius = 0.03``` and ```maxConeRadius = 2.2``` values. 
- Press the **g** key to grow the cells. The duration of this animation can be changed by changing the ```double originalGrowDuration = 8000```. This value must be in milliseconds.
- Press the **v** key to reverse the direction of growth, i.e to toggle between shrinking and growing.

All the input bindings can be changed by modifying the variable values in the ```Input``` class.
