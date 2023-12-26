# Tools

This repository contains a couple of projects in a very early stage and a set of general purpose tools contained in `tools.h`.


## tools.h

In `tools.h` one can find macros, types, and functions for the following stuff:

* Platform identification
* Assertions, debugging, errors, logging
* Aliases for sized types
* Strings
* Memory
  - Linear memory arena allocators
  - Virtual memory allocation abstraction
* File reading
* Mathematics
* Clock / timing
* Window creation
* Input handling (mouse and keyboard)


## Projects

Currently, there are the following *in-progress* projects:

* `main_interpreter`: Implementation of a scripted language interpreter. Following the contents of the *Crafting interpreters* book (by Robert Nystrom).
* `main_vulkan`: Implementation of a graphics application template using the Vulkan graphics API.
* `main_d3d12`: Implementation of a graphics application template using the D3D12 graphics API.
* `main_atof`: Custom implementation of the atof (ASCII to float) function.
* `main_spirv`: Simple SPIRV parser to be used by a Vulkan engine potentially.


## Dependencies

To the greatest extent possible, the projects and tools here present will not have heavy external dependencies. At most, dependencies will be included within this repository itself and will consist of lightweight source code files compiled along with the projects.

This is the list of the dependencies currently included in this repository:

* [Volk](https://github.com/zeux/volk) (a meta-loader for Vulkan by Zeux)
* [stb_image](https://github.com/nothings/stb) (public domain image loader by Seann Barrett)
* [Dear ImGui](https://github.com/ocornut/imgui) (bloat-free graphical user interface library for C++ by Omar Cornut)
* [DXC](https://github.com/microsoft/DirectXShaderCompiler) (the DirectX Shader Compiler binary and library)
* [Android SDK](https://developer.android.com/studio) (actually all the tools and packages required to build and deploy .APK files)


## Supported platforms

So far the current code is being tested on Windows, Linux, and Android platforms.


## Interesting links

Here are some interesting links that I have been looking while writing some code:

* [Crafting interpreters online book](https://craftinginterpreters.com/contents.html)
* [Creating windows on Windows](https://learn.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program)
* [Creating windows on Linux](https://www.codeproject.com/articles/1089819/an-introduction-to-xcb-programming)
* [XCB Documentation](https://xcb.freedesktop.org/manual/index.html)
* [XCB tricks](http://metan.ucw.cz/blog/things-i-wanted-to-know-about-libxcb.html)
* [Vulkan tutorial](https://vulkan-tutorial.com/)
* [Vulkan guide: GPU driven rendering](https://vkguide.dev/docs/gpudriven/gpu_driven_engines)
* [D3D12 tutorial](https://www.3dgep.com/category/graphics-programming/directx/)
* [Handmade Math](https://github.com/HandmadeMath/HandmadeMath)
* [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator)
* [My personal gists](https://gist.github.com/jesusdz)


## License

All code here present is available to anybody free of charge without any legal obligation under the [Unlicense](./LICENSE) terms (no terms basically). However, if these tools or part of them are included in third projects, attribution will be happily appreciated.

