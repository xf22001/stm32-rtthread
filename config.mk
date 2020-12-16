BSP_ROOT ?= /home/xiaofei/media/rt-thread/bsp/stm32f20x
RTT_ROOT ?= /home/xiaofei/media/rt-thread

CROSS_COMPILE ?=/usr/bin/arm-none-eabi-

CPPPATHS :=-I$(BSP_ROOT) \
	-I$(BSP_ROOT)/Drivers \
	-I$(BSP_ROOT)/Libraries/CMSIS/CM3/CoreSupport \
	-I$(BSP_ROOT)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F2xx \
	-I$(BSP_ROOT)/Libraries/CMSIS/Include \
	-I$(BSP_ROOT)/Libraries/STM32F2x7_ETH_Driver/inc \
	-I$(BSP_ROOT)/Libraries/STM32F2xx_StdPeriph_Driver/inc \
	-I$(BSP_ROOT)/applications \
	-I$(BSP_ROOT)/packages/CmBacktrace-v1.4.0 \
	-I$(BSP_ROOT)/packages/at_device-latest/class/sim76xx \
	-I$(BSP_ROOT)/packages/at_device-latest/inc \
	-I$(BSP_ROOT)/packages/gui_engine-latest/include \
	-I$(BSP_ROOT)/packages/gui_engine-latest/libraries/lodepng \
	-I$(BSP_ROOT)/packages/netutils-v1.2.0/iperf \
	-I$(BSP_ROOT)/packages/netutils-v1.2.0/tcpdump \
	-I$(BSP_ROOT)/packages/netutils-v1.2.0/telnet \
	-I$(BSP_ROOT)/packages/optparse-v1.0.0 \
	-I$(BSP_ROOT)/packages/tencent-iot-sdk-latest/ports/ssl \
	-I$(BSP_ROOT)/packages/tencent-iot-sdk-latest/qcloud-iot-explorer-sdk-embedded-c/include \
	-I$(BSP_ROOT)/packages/tencent-iot-sdk-latest/qcloud-iot-explorer-sdk-embedded-c/include/exports \
	-I$(BSP_ROOT)/packages/tencent-iot-sdk-latest/qcloud-iot-explorer-sdk-embedded-c/sdk_src/internal_inc \
	-I$(BSP_ROOT)/packages/wiznet-latest/inc \
	-I$(BSP_ROOT)/packages/wiznet-latest/ioLibrary \
	-I$(BSP_ROOT)/packages/wiznet-latest/ioLibrary/Ethernet \
	-I$(BSP_ROOT)/packages/wiznet-latest/ioLibrary/Internet \
	-I$(RTT_ROOT)/components/dfs/filesystems/devfs \
	-I$(RTT_ROOT)/components/dfs/filesystems/elmfat \
	-I$(RTT_ROOT)/components/dfs/filesystems/romfs \
	-I$(RTT_ROOT)/components/dfs/include \
	-I$(RTT_ROOT)/components/drivers/include \
	-I$(RTT_ROOT)/components/drivers/spi \
	-I$(RTT_ROOT)/components/finsh \
	-I$(RTT_ROOT)/components/libc/compilers/common \
	-I$(RTT_ROOT)/components/libc/compilers/newlib \
	-I$(RTT_ROOT)/components/net/at/at_socket \
	-I$(RTT_ROOT)/components/net/at/include \
	-I$(RTT_ROOT)/components/net/lwip-2.0.2/src \
	-I$(RTT_ROOT)/components/net/lwip-2.0.2/src/arch/include \
	-I$(RTT_ROOT)/components/net/lwip-2.0.2/src/include \
	-I$(RTT_ROOT)/components/net/lwip-2.0.2/src/include/ipv4 \
	-I$(RTT_ROOT)/components/net/lwip-2.0.2/src/include/netif \
	-I$(RTT_ROOT)/components/net/netdev/include \
	-I$(RTT_ROOT)/components/net/sal_socket/impl \
	-I$(RTT_ROOT)/components/net/sal_socket/include \
	-I$(RTT_ROOT)/components/net/sal_socket/include/dfs_net \
	-I$(RTT_ROOT)/components/net/sal_socket/include/socket \
	-I$(RTT_ROOT)/components/net/sal_socket/include/socket/sys_socket \
	-I$(RTT_ROOT)/include \
	-I$(RTT_ROOT)/libcpu/arm/common \
	-I$(RTT_ROOT)/libcpu/arm/cortex-m3 

DEFINES := -DAUTH_MODE_KEY -DAUTH_WITH_NOTLS -DHAVE_CCONFIG_H -DMQTT_COMM_ENABLED -DRT_USING_NEWLIB -DUSE_STDPERIPH_DRIVER -D__RTTHREAD__
