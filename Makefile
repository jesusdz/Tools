.PHONY: default main_interpreter main_vulkan shaders clean

default: main_vulkan

main_interpreter:
	g++ -g -o main_interpreter main_interpreter.cpp

main_vulkan:
	g++ -g -o main_vulkan  main_vulkan.cpp -DVK_NO_PROTOTYPES -lxcb

shaders:
	glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/vertex.spv
	glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/fragment.spv

clean:
	rm -f main_interpreter main_vulkan main_atof shaders/*.spv

