.PHONY: default main_interpreter main_vulkan main_spirv main_cparser main_reflect_serialize shaders clean

export PATH:=$(PATH):dxc/linux/bin

default: main_vulkan

main_interpreter:
	g++ -g -o main_interpreter main_interpreter.cpp

main_vulkan:
	g++ -g -o main_vulkan  main_vulkan.cpp -I"vulkan/include" -DVK_NO_PROTOTYPES -lxcb

main_spirv:
	g++ -g -o main_spirv main_spirv.cpp

main_cparser:
	g++ -g -o main_cparser main_cparser.cpp

main_reflect_serialize: main_cparser
	./main_cparser assets.h > assets.reflex.h
	g++ -g -o main_reflect_serialize main_reflect_serialize.cpp

shaders:
	dxc -spirv -T vs_6_7 -Fo shaders/vertex.spv -Fc shaders/vertex.dis shaders/vertex.hlsl
	dxc -spirv -T ps_6_7 -Fo shaders/fragment.spv -Fc shaders/fragment.dis shaders/fragment.hlsl

clean:
	rm -f main_interpreter main_vulkan main_atof main_spirv main_cparser main_reflect_serialize shaders/*.spv shaders/*.dis

