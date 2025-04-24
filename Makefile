.PHONY: default main_interpreter main_vulkan main_spirv reflex main_reflect_serialize main_clon cast shaders clean

CXX=g++
CXXFLAGS= -g
DXC=./dxc/linux/bin/dxc
DEBUG_FLAGS=-fspv-debug=vulkan-with-source

default: main_vulkan

main_interpreter:
	${CXX} ${CXXFLAGS} -o main_interpreter main_interpreter.cpp

main_vulkan: reflex
	./reflex assets/assets.h > assets.reflex.h
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
	${DXC} -spirv ${DEBUG_FLAGS} -T vs_6_7 -E VSMain -Fo shaders/vs_shading.spv -Fc shaders/vs_shading.dis -Fi shaders/vs_shading.pp shaders/shading.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T ps_6_7 -E PSMain -Fo shaders/fs_shading.spv -Fc shaders/fs_shading.dis -Fi shaders/fs_shading.pp shaders/shading.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T vs_6_7 -E VSMain -Fo shaders/vs_sky.spv -Fc shaders/vs_sky.dis shaders/sky.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T ps_6_7 -E PSMain -Fo shaders/fs_sky.spv -Fc shaders/fs_sky.dis shaders/sky.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T vs_6_7 -E VSMain -Fo shaders/vs_shadowmap.spv -Fc shaders/vs_shadowmap.dis shaders/shadowmap.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T ps_6_7 -E PSMain -Fo shaders/fs_shadowmap.spv -Fc shaders/fs_shadowmap.dis shaders/shadowmap.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T vs_6_7 -E VSMain -Fo shaders/vs_ui.spv -Fc shaders/vs_ui.dis shaders/ui.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T ps_6_7 -E PSMain -Fo shaders/fs_ui.spv -Fc shaders/fs_ui.dis shaders/ui.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T cs_6_7 -E main_clear -Fo shaders/compute_clear.spv -Fc shaders/compute_clear.dis shaders/compute.hlsl
	${DXC} -spirv ${DEBUG_FLAGS} -T cs_6_7 -E main_update -Fo shaders/compute_update.spv -Fc shaders/compute_update.dis shaders/compute.hlsl

clean:
	rm -f main_interpreter main_vulkan main_atof main_spirv reflex main_reflect_serialize main_clon cast shaders/*.spv shaders/*.dis

