# Godot GDExtension VNC library

Port of https://github.com/BastiaanOlij/gdvnc to GDExtension.

Building
--------
Open up a terminal and CD into the folder into which you've cloned this project

We need to start with making sure our submodules are up to date:
```
git submodule init
git submodule update
```

Building libvnc:
```
cd libvncserver
mkdir build
cd build
ccmake ..
cmake --build . --config Release
cd ..\..
```

And finally build our library:
```
scons
```
