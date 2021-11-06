# navi_maze  
navi_maze short for Navigational Maze (yes, I know), is a hobby project to understand and see if I can implement  
a very basic endless runner game. This uses c, built on top of raylib, using only the most basic library funcitonalities  
pixel drawing, and rendering to the screen.  
Everything else is implemented myself, to maximise the things I learn.  
Things I have done so far:  
* Implemented line drawing from 2 pixel points (using DDA)  
* Implemented how to generate random shapes  
* Implemented how to move the shapes across the screen to give the illusion of a 
moving player  
* Implemented collision detection by checking is a pixel is inside 3 points 
making up a triangle  

## Notes on building and running yourself  
As the project is still in progress, I am like 50% or slight more of the way through  
I have not setup any build methodologies for you (the user or potential experimenter).  
Sorry in advance, but the all the raylib scripts are there so if you need to build you  
would need to:  
* edit the build script (build-windows.bat)  
* set path to your compiler (i use msvc)  
I think this should be it, I maybe mistaken, do let me know if there are some errors.  

