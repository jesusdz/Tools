.PHONY: default main main_xwindow shaders clean

default: main_xwindow

main:
	g++ -g -o main main.cpp

main_xwindow:
	g++ -g -o main_xwindow  main_xwindow.cpp -DVK_NO_PROTOTYPES -lxcb

shaders:
	glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/vertex.spv
	glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/fragment.spv

clean:
	rm -f main main_xwindow shaders/*.spv
