# DwG Game Math
This is a starting project for DwG (Dziewczyny w Grze!) for learning game's math.
It's for Windows, using OpenGL, and modified Sony's math library.

### How to
You will need cmake (read on before you install):
https://cmake.org/download/

When installing, make sure to select option with setting PATH for either current user or all users (up to you, but we need PATH to be set).

Download the repository onto your disk (you can just download .zip version and unpack wherever you like).

Open terminal (hit Windows key, type cmd) and go to your repository (type "C:" or "Z:" or whatever your disk is and hit ENTER, then navigate to the repository by "cd folderName" until you're there, you go up the folder by "cd .."). Create "build" folder ("mkdir build") and go into it ("cd build"). The build folder should be a sibling to lib and src folders. Now type "cmake .." (yes with two dots) and hit ENTER.

This will generate Visual Studio solution for you.

You can now open build folder in Explorer and open game-math.sln file.

For debug drawing use dwgDebugLine and dwgDebugSphere functions. You can call them wherever you want as long the rendering is already initialised.
