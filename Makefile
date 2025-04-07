# Define the target board for deployment (update this to match your board's address)
TARGET ?= root@<your-board-ip>:/path/to/deploy

# Set up the OpenWRT SDK environment (update this to match your SDK path)
export SDK_DIR=<path-to-your-openwrt-sdk>
export STAGING_DIR=${SDK_DIR}/staging_dir
export PATH:=${STAGING_DIR}/toolchain-mipsel_24kc_gcc-12.3.0_musl/bin:${PATH}
export CROSS_COMPILE=mipsel-openwrt-linux-

# Compiler
CC = ${CROSS_COMPILE}gcc

# Include paths
INCLUDE += ${STAGING_DIR}/usr/include/

# Build and configure libmodbus
libmodbus_configure:
	cd SERVER/3pp/libmodbus; \
	./autogen.sh; \
	./configure --host=mipsel-openwrt-linux --prefix=${STAGING_DIR}/usr

libmodbus_build: libmodbus_configure
	cd SERVER/3pp/libmodbus; \
	make

libmodbus_install:
	cd SERVER/3pp/libmodbus; \
	make install

# Build third-party dependencies
3pp: libmodbus_build

3pp_install: libmodbus_install

# Compile the Modbus TCP server
server:
	mkdir -p SERVER/build
	${CC} -o SERVER/build/server SERVER/server.c -I${INCLUDE} -L${STAGING_DIR}/usr/lib -lmodbus

# Deploy the server to the target board
server_install:
	scp -O SERVER/build/server $(TARGET)

# Mark server_install as a phony target
.PHONY: server_install