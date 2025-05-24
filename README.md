# Tools

This repository contains a couple of projects in a very early stage and a set of general purpose tools contained in `tools.h`.


## tools.h

In `tools.h` one can find utility macros, types, and functions to help with the following stuff:

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


## cast.h

Implementation of a basic C AST (abstract syntax tree) generator. For now it focuses on parsing type and data definitions.
It's not complete at all but it allows populating a good amount of information about user type and data definitions
(basic structs with trivial members, with nested structs inside, with fixed-length array members, pointer-type members, etc).
It scans and parses a C text file and fills a `Cast` object containing the parsed information in the form of an AST.
This structure can be then inspected by client programs to obtain information about the types and data declared in the code.


## cast.cpp

Sample application using `cast.h` that simply generates the AST of a C file and prints the whole structure to screen.


## reflex.h

Headers with container types needed for C reflection / RTTI (run time type information).
An application that is able to get this information populated for its most relevant structures is able to perform certain tasks an a more automatic way.
A couple of examples of tasks than can be automatized or improved by having a proper language reflection mechanism are data serialization or UI generation.


## reflex.cpp

C reflection data generator. It uses `cast.h` to parse a C file and generate its AST, and then it outputs the C code needed to populate the RTTI structs present in `reflex.h`.
The generated code can be then included by applications using the parsed C file types so they can benefit from the reflected information.


## tools_spirv.h

In `tools_spirv.h` there are utils to parse SPIRV shader modules and extract lists of descriptor sets used by them.
With this SPIRV reflection utility, client applications can automatize the creation of descriptors and binding of resources at different places in the code,
thus avoiding unnecessary boilerplate code.

## tools_gfx.h

Abstraction of a modern graphics API for now made on top of Vulkan.

## tools_ui.h

Implementation of a immediate mode UI library using `tools_gfx.h`.


## Other projects

Currently, there are the following *in-progress* projects:

* `main_interpreter`: Implementation of a scripted language interpreter. Following the contents of the *Crafting interpreters* book (by Robert Nystrom).
* `main_gfx`: Implementation of a graphics application template using the `tools_gfx.h` header.
* `main_d3d12`: Implementation of a graphics application template using the D3D12 graphics API.
* `main_atof`: Custom implementation of the atof (ASCII to float) function.
* `main_spirv`: Simple SPIRV parser to be used by a Vulkan engine potentially.
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
* [ANSI C Yacc grammar](https://www.lysator.liu.se/c/ANSI-C-grammar-y.html)
* [ANSI C Yacc grammar (based on 2011 ISO C standard)](https://www.quut.com/c/ANSI-C-grammar-y.html)
* [Grammars and Parsing](https://www.cs.cornell.edu/courses/cs211/2006sp/Sections/S3/grammars.html)
* [Reverse Z (and why it's so awesome)](https://tomhultonharrop.com/mathematics/graphics/2023/08/06/reverse-z.html)


## License

All code here present is available to anybody free of charge without any legal obligation under the [Unlicense](./LICENSE) terms (no terms basically). However, if these tools or part of them are included in third projects, attribution will be happily appreciated.

