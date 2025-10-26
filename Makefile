.PHONY: default build_and_run build_and_debug main_interpreter engine game main_spirv reflex main_reflect_serialize main_clon cast data clean main_alsa main_gamepad directories

CXX=g++
CXXFLAGS= -g
BUILD_DIR=build
DATA_DIR=${BUILD_DIR}
DATA_SHADERS_DIR=${DATA_DIR}/shaders
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

engine: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/engine  engine.cpp -I"vulkan/include" -lxcb -lpthread

game:
	${CXX} -fPIC -g -Wall -c game.cpp -o ${BUILD_DIR}/game.o
	${CXX} ${BUILD_DIR}/game.o -shared -o ${BUILD_DIR}/game.so

main_spirv: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_spirv main_spirv.cpp

reflex: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/reflex reflex.cpp

main_reflect_serialize: reflex
	./build/reflex assets/assets.h > assets.reflex.h
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_reflect_serialize main_reflect_serialize.cpp

main_clon: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_clon main_clon.cpp

cast: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/cast cast.cpp

main_alsa: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_alsa main_alsa.cpp

main_gamepad: directories
	${CXX} ${CXXFLAGS} -o ${BUILD_DIR}/main_gamepad main_gamepad.cpp

directories:
	mkdir -p build
	mkdir -p build/shaders

data: engine
	./build/engine --build-data

clean:
	rm -rf build

