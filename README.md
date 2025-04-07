# Project: MediaTek MT7620 and OpenWRT

This project involves using the MediaTek MT7620 SoC and OpenWRT to develop and test a simple version of a Modbus TCP server.

## Overview

### MediaTek MT7620 SoC
Provide a brief explanation of the MT7620 SoC architecture, highlighting its key components and features.

### OpenWRT
Summarize the elements of OpenWRT and its applications, particularly in embedded systems.

### Development Tools
Describe the set of development tools required for working with the MT7620 SoC and OpenWRT.

---

## Steps to Set Up and Test the Modbus TCP Server

### 1. Modify the Makefile for Your Environment
Before building the project, ensure that the paths in the `Makefile` match your environment. Update the following variables in the `Makefile`:

- **`SDK_DIR`**: Path to your OpenWRT SDK.
- **`TARGET`**: Target board's address for deployment.

For example:
```makefile
export SDK_DIR=/path/to/your/openwrt-sdk
TARGET=root@your-board-ip:/path/to/deploy
```

### 2. Build and Deploy the Server
Once the `Makefile` is updated, run the following commands:
```bash
make libmodbus_build
make server
make server_install
```

### 3. Start the Server on the Board
Start the Modbus TCP server on the board.

### 4. Verify the Server
To verify that the server is working properly, run the following commands on a Linux PC:

#### a. Export Path to `modpoll`
```bash
export PATH=<path_to_this_cloned_repo>/CLIENT/modpoll/x86_64-linux-gnu/:$PATH
```

#### b. Run `modpoll`
```bash
modpoll -m tcp -a 1 -r 1 -c 5 192.168.1.1
```

---

## Configuring the Board

To allow Modbus TCP/IP communication on port 502, add the following firewall rules on the board:
```bash
uci add firewall rule
uci set firewall.@rule[-1].name="Allow Modbus TCP"
uci set firewall.@rule[-1].src="wan"
uci set firewall.@rule[-1].dest_port="502"
uci set firewall.@rule[-1].proto="tcp"
uci set firewall.@rule[-1].target="ACCEPT"
uci commit firewall
/etc/init.d/firewall restart
```

---

## Makefile Targets

The `Makefile` in this project automates the build and deployment process. Below are the key targets:

- **`libmodbus_build`**: Builds the `libmodbus` library required for the server.
- **`server`**: Compiles the Modbus TCP server using the cross-compiler.
- **`server_install`**: Copies the compiled server binary to the target board.

### Example Usage
To build and deploy the server:
```bash
make libmodbus_build
make server
make server_install TARGET=root@192.168.1.1:/root
```

---

## Example Usage

### Read Coils
Reads 10 coils starting from address 0:
```bash
modpoll -m tcp -c 10 -t 0 192.168.1.1
```

### Read Discrete Inputs
Reads 10 discrete inputs starting from address 0:
```bash
modpoll -m tcp -c 10 -t 1 192.168.1.1
```

### Read Input Registers
Reads 10 input registers starting from address 0:
```bash
modpoll -m tcp -c 10 -t 3 192.168.1.1
```

### Read Holding Registers
Reads 10 holding registers starting from address 0:
```bash
modpoll -m tcp -c 10 -t 4 192.168.1.1
```

