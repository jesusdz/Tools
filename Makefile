.PHONY: default main_interpreter main_vulkan main_spirv reflex main_reflect_serialize main_clon cast shaders clean

CXX=g++
CXXFLAGS= -g
DXC=./dxc/linux/bin/dxc

default: main_vulkan

main_interpreter:
	${CXX} ${CXXFLAGS} -o main_interpreter main_interpreter.cpp

main_vulkan:
	${CXX} ${CXXFLAGS} -o main_vulkan  main_vulkan.cpp -I"vulkan/include" -DVK_NO_PROTOTYPES -lxcb

main_spirv:
	${CXX} ${CXXFLAGS} -o main_spirv main_spirv.cpp

reflex:
	${CXX} ${CXXFLAGS} -o reflex reflex.cpp

main_reflect_serialize: reflex
	./reflex assets.h > assets.reflex.h
	${CXX} ${CXXFLAGS} -o main_reflect_serialize main_reflect_serialize.cpp

main_clon:
	${CXX} ${CXXFLAGS} -o main_clon main_clon.cpp

cast:
	${CXX} ${CXXFLAGS} -o cast cast.cpp

shaders:
	${DXC} -spirv -T vs_6_7 -Fo shaders/vertex.spv -Fc shaders/vertex.dis shaders/vertex.hlsl -E main
	${DXC} -spirv -T ps_6_7 -Fo shaders/fragment.spv -Fc shaders/fragment.dis shaders/fragment.hlsl -E main
	${DXC} -spirv -T vs_6_7 -Fo shaders/vs_shadowmap.spv -Fc shaders/vs_shadowmap.dis shaders/vs_shadowmap.hlsl -E main
	${DXC} -spirv -T ps_6_7 -Fo shaders/fs_shadowmap.spv -Fc shaders/fs_shadowmap.dis shaders/fs_shadowmap.hlsl -E main
	${DXC} -spirv -T cs_6_7 -Fo shaders/compute_clear.spv -Fc shaders/compute_clear.dis shaders/compute.hlsl -E main_clear
	${DXC} -spirv -T cs_6_7 -Fo shaders/compute_update.spv -Fc shaders/compute_update.dis shaders/compute.hlsl -E main_update

clean:
	rm -f main_interpreter main_vulkan main_atof main_spirv reflex main_reflect_serialize main_clon cast shaders/*.spv shaders/*.dis

