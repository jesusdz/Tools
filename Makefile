.PHONY: default main_interpreter main_vulkan main_spirv shaders clean

export PATH:=$(PATH):dxc/linux/bin

default: main_vulkan

main_interpreter:
	g++ -g -o main_interpreter main_interpreter.cpp

main_vulkan:
	g++ -g -o main_vulkan  main_vulkan.cpp -I"vulkan/include" -DVK_NO_PROTOTYPES -lxcb

main_spirv:
	g++ -g -o main_spirv main_spirv.cpp

shaders:
	dxc -spirv -T vs_6_7 -Fo shaders/vertex.spv -Fc shaders/vertex.dis shaders/vertex.hlsl
	dxc -spirv -T ps_6_7 -Fo shaders/fragment.spv -Fc shaders/fragment.dis shaders/fragment.hlsl

clean:
	rm -f main_interpreter main_vulkan main_atof main_spirv shaders/*.spv

