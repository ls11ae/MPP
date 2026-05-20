# MPP

## Building

### Linux
```
<mkdir build>
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src
cmake --build .
```

To run (from `build` folder):
```
./mpp [path_to_input_file] [path_to_output_folder] <-v to visualize> <-a 0|1|2 to choose an algorithm>
```

### Windows

This method requires an installation of [vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows) with [CGAL](https://doc.cgal.org/latest/Manual/windows.html) installed.
```
mkdir build
cmake -S .\src\ -B .\build\ --toolchain [path to vcpkg]\scripts\buildsystems\vcpkg.cmake
cmake --build .\build\ --config Release
```

To run:
```
cd .\build\Release
.\mpp.exe [path_to_input_file] [path_to_output_folder] <-v to visualize> <-a 0|1|2 to choose an algorithm>
```
