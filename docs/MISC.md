# Experimental / misc tools

`code/misc/` holds small side projects and tech tests that are independent from the engine itself — one-off tools, learning exercises, and pieces used to prototype ideas before (or instead of) folding them into the engine proper.

* `cast.h` / `cast.cpp`: A basic C AST (abstract syntax tree) generator. Parses a C source file and populates a `Cast` object describing the type and data definitions found in it (structs, nested structs, fixed-length arrays, pointer members, etc).
* `reflex.h` / `reflex.cpp`: C reflection / RTTI data generator, built on `cast.h`. Parses a C file and emits the C code needed to populate reflection structs for its types, which the engine's data/reflection pipeline relies on to serialize assets.
* `main_interpreter`: A scripted language interpreter, following the *Crafting Interpreters* book by Robert Nystrom.
* `main_d3d12`: A graphics application template using the D3D12 API.
* `main_atof`: A custom implementation of `atof` (ASCII to float).
* `main_spirv`: A standalone SPIRV parser, precursor to `ilu_spirv.h`.
* `main_reflect_serialize`: A JSON serializer built on the C reflection utilities.
