BSP_ROOT ?= /home/xiaofei/media/rt-thread/bsp/stm32f20x
RTT_ROOT ?= /home/xiaofei/media/rt-thread

CPPPATHS :=-I$(BSP_ROOT) \
	-I$(BSP_ROOT)/packages/at_device-latest/class/sim76xx \
	-I$(BSP_ROOT)/packages/at_device-latest/inc \
	-I$(RTT_ROOT)/components/dfs/filesystems/devfs \
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

DEFINES := -DHAVE_CCONFIG_H -DRT_USING_NEWLIB -DUSE_STDPERIPH_DRIVER -D__RTTHREAD__