.PHONY: default build_and_run build_and_debug main_interpreter engine game main_spirv reflex main_reflect_serialize main_clon cast data clean main_alsa directories

CXX=g++
CXXFLAGS= -g
BUILD_DIR=build
DATA_DIR=${BUILD_DIR}
DATA_SHADERS_DIR=${DATA_DIR}/shaders
DATA_ASSETS_DIR=${DATA_DIR}/assets
DXC=./dxc/linux/bin/dxc

# Requires Vulkan 1.3
#DXC_FLAGS=-Od -fspv-extension=SPV_KHR_non_semantic_info -fspv-debug=vulkan-with-source
DXC_FLAGS=-O3

default: build_and_run

build_and_run: engine
	i3 workspace run
	./build/engine; i3 workspace 2

build_and_debug: engine
	i3 workspace debug
	gf2 ./build/engine; i3 workspace 2

main_interpreter: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_interpreter main_interpreter.cpp

engine: reflex
	./build/reflex assets/assets.h > assets.reflex.h
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/engine  engine.cpp -I"vulkan/include" -DVK_NO_PROTOTYPES -lxcb

game:
	${CXX} -fPIC -g -Wall -c game.cpp -o ${BUILD_DIR}/game.o
	${CXX} ${BUILD_DIR}/game.o -shared -o ${BUILD_DIR}/game.so

main_spirv: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_spirv main_spirv.cpp

reflex: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex reflex.cpp

main_reflect_serialize: reflex
	./build/reflex assets.h > assets.reflex.h
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_reflect_serialize main_reflect_serialize.cpp

main_clon: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_clon main_clon.cpp

cast: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/cast cast.cpp

main_alsa: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_alsa main_alsa.cpp

directories:
	mkdir -p build
	mkdir -p build/shaders
	mkdir -p build/assets

data: directories
	${DXC} -spirv ${DXC_FLAGS} -T vs_6_7 -E VSMain -Fo ${DATA_SHADERS_DIR}/vs_shading.spv -Fc ${DATA_SHADERS_DIR}/vs_shading.dis -Fi shaders/vs_shading.pp shaders/shading.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T ps_6_7 -E PSMain -Fo ${DATA_SHADERS_DIR}/fs_shading.spv -Fc ${DATA_SHADERS_DIR}/fs_shading.dis -Fi shaders/fs_shading.pp shaders/shading.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T vs_6_7 -E VSMain -Fo ${DATA_SHADERS_DIR}/vs_sky.spv -Fc ${DATA_SHADERS_DIR}/vs_sky.dis shaders/sky.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T ps_6_7 -E PSMain -Fo ${DATA_SHADERS_DIR}/fs_sky.spv -Fc ${DATA_SHADERS_DIR}/fs_sky.dis shaders/sky.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T vs_6_7 -E VSMain -Fo ${DATA_SHADERS_DIR}/vs_shadowmap.spv -Fc ${DATA_SHADERS_DIR}/vs_shadowmap.dis shaders/shadowmap.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T ps_6_7 -E PSMain -Fo ${DATA_SHADERS_DIR}/fs_shadowmap.spv -Fc ${DATA_SHADERS_DIR}/fs_shadowmap.dis shaders/shadowmap.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T vs_6_7 -E VSMain -Fo ${DATA_SHADERS_DIR}/vs_ui.spv -Fc ${DATA_SHADERS_DIR}/vs_ui.dis shaders/ui.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T ps_6_7 -E PSMain -Fo ${DATA_SHADERS_DIR}/fs_ui.spv -Fc ${DATA_SHADERS_DIR}/fs_ui.dis shaders/ui.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T vs_6_7 -E VSMain -Fo ${DATA_SHADERS_DIR}/vs_id.spv -Fc ${DATA_SHADERS_DIR}/vs_id.dis shaders/id.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T ps_6_7 -E PSMain -Fo ${DATA_SHADERS_DIR}/fs_id.spv -Fc ${DATA_SHADERS_DIR}/fs_id.dis shaders/id.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T cs_6_7 -E CSMain -Fo ${DATA_SHADERS_DIR}/compute_select.spv -Fc ${DATA_SHADERS_DIR}/compute_select.dis shaders/compute_select.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T cs_6_7 -E main_clear -Fo ${DATA_SHADERS_DIR}/compute_clear.spv -Fc ${DATA_SHADERS_DIR}/compute_clear.dis shaders/compute.hlsl
	${DXC} -spirv ${DXC_FLAGS} -T cs_6_7 -E main_update -Fo ${DATA_SHADERS_DIR}/compute_update.spv -Fc ${DATA_SHADERS_DIR}/compute_update.dis shaders/compute.hlsl
	cp -R assets/* ${DATA_ASSETS_DIR}

clean:
	rm -rf build

