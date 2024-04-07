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


## tools_spirv.h

In `tools_spirv.h` there are utils to parse SPIRV shader modules and extract lists of descriptor sets used by them.
With this SPIRV reflection utility, client applications can automatize the creation of descriptors and binding of resources at different places in the code,
thus avoiding unnecessary boilerplate code.


## reflex.h

Headers with types needed for C reflection. The idea of this in-progress utility is that we will have a reflection tool that will parse C headers with type definitions,
and it will generate files with C code filling the Reflex containers available in `reflex.h`. These generated files with information about your own C data structures,
will in turn be possibly used in your code to automatize certain tasks such as serialization, UI generation, etc.


## Projects

Currently, there are the following *in-progress* projects:

* `main_interpreter`: Implementation of a scripted language interpreter. Following the contents of the *Crafting interpreters* book (by Robert Nystrom).
* `main_vulkan`: Implementation of a graphics application template using the Vulkan graphics API.
* `main_d3d12`: Implementation of a graphics application template using the D3D12 graphics API.
* `main_atof`: Custom implementation of the atof (ASCII to float) function.
* `main_spirv`: Simple SPIRV parser to be used by a Vulkan engine potentially.
* `reflex`: Generator of C reflection data.
* `main_reflect_serialize`: JSON serializer using C reflection utils.


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
* [HLSL for Vulkan: Matrices](https://www.lei.chat/posts/hlsl-for-vulkan-matrices)
* [HLSL for Vulkan: Resources](https://www.lei.chat/posts/hlsl-for-vulkan-resources)
* [HLSL for Vulkan: Semantic strings and locations](https://www.lei.chat/posts/hlsl-for-vulkan-semantic-strings-and-location-numbers)
* [D3D12 tutorial](https://www.3dgep.com/category/graphics-programming/directx/)
* [Handmade Math](https://github.com/HandmadeMath/HandmadeMath)
* [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator)
* [My personal gists](https://gist.github.com/jesusdz)


## License

All code here present is available to anybody free of charge without any legal obligation under the [Unlicense](./LICENSE) terms (no terms basically). However, if these tools or part of them are included in third projects, attribution will be happily appreciated.

