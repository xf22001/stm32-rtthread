

/*================================================================
 *   
 *   
 *   文件名称：serial.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月24日 星期二 10时42分10秒
 *   修改日期：2020年11月27日 星期五 09时13分50秒
 *   描    述：
 *
 *================================================================*/
#ifndef _SERIAL_H
#define _SERIAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <rthw.h>
#include <rtthread.h>
#include "app_platform.h"

#ifdef __cplusplus
}
#endif


#define UART_RX_BUFFER_SIZE 256
#define UART_RX_DMA_BUFFER_SIZE (UART_RX_BUFFER_SIZE / 4)
#define UART_TX_DMA_NODE_SIZE 4

/* data node for Tx Mode */
struct stm32_serial_data_node
{
	rt_uint8_t *data_ptr;
	rt_size_t  data_size;
	struct stm32_serial_data_node *next, *prev;
};
struct stm32_serial_dma_tx
{
	/* DMA Channel */
	DMA_Stream_TypeDef* dma_channel;

	/* data list head and tail */
	struct stm32_serial_data_node *list_head, *list_tail;

	/* data node memory pool */
	struct rt_mempool data_node_mp;
	rt_uint8_t data_node_mem_pool[UART_TX_DMA_NODE_SIZE *
		(sizeof(struct stm32_serial_data_node) + sizeof(void*))];
};

struct stm32_serial_int_rx
{
	rt_uint8_t  rx_buffer[UART_RX_BUFFER_SIZE];
	rt_uint32_t read_index, save_index;
	rt_sem_t sem;
};

struct stm32_serial_device
{
	UART_HandleTypeDef *huart;

	/* rx structure */
	struct stm32_serial_int_rx* int_rx;

	/* tx structure */
	struct stm32_serial_dma_tx* dma_tx;
};

rt_err_t rt_hw_serial_register(rt_device_t device, const char *name, rt_uint32_t flag, struct stm32_serial_device *serial);
void rt_hw_serial_rx_isr(rt_device_t device);
void rt_hw_serial_dma_tx_isr(rt_device_t device);
#endif //_SERIAL_H
