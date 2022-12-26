.PHONY: default main_interpreter main_xwindow shaders clean

default: main_xwindow

main_interpreter:
	g++ -g -o main_interpreter main_interpreter.cpp

main_xwindow:
	g++ -g -o main_xwindow  main_xwindow.cpp -DVK_NO_PROTOTYPES -lxcb

shaders:
	glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/vertex.spv
	glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/fragment.spv

clean:
	rm -f main_interpreter main_xwindow shaders/*.spv

