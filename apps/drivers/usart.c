

/*================================================================
 *
 *
 *   文件名称：usart.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月24日 星期二 10时26分25秒
 *   修改日期：2020年11月25日 星期三 11时20分15秒
 *   描    述：
 *
 *================================================================*/
#include "usart.h"
#include "serial.h"

#ifdef RT_USING_UART1
extern UART_HandleTypeDef huart1;
static struct stm32_serial_int_rx uart1_int_rx = {0};
static struct stm32_serial_dma_tx uart1_dma_tx = {0};
static struct rt_device uart1_device;
static struct stm32_serial_device uart1 = {
	&huart1,
	&uart1_int_rx,
	&uart1_dma_tx
};
#endif

#ifdef RT_USING_UART3
extern UART_HandleTypeDef huart3;
static struct stm32_serial_int_rx uart3_int_rx = {0};
static struct stm32_serial_dma_tx uart3_dma_tx = {0};
static struct rt_device uart3_device;
static struct stm32_serial_device uart3 = {
	&huart3,
	&uart3_int_rx,
	&uart3_dma_tx
};
#endif

/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init()
{
#ifdef RT_USING_UART1
	/* register uart1 */
	rt_hw_serial_register(&uart1_device, "uart1",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
	                      &uart1);
#endif

	/* uart init */
#ifdef RT_USING_UART3
	/* register uart3 */
	rt_hw_serial_register(&uart3_device, "uart3",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
	                      &uart3);
#endif
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* enter interrupt */
	rt_interrupt_enter();

#ifdef RT_USING_UART1

	if(huart == uart1.huart) {
		rt_hw_serial_dma_tx_isr(&uart1_device);
	}

#endif

#ifdef RT_USING_UART3

	if(huart == uart3.huart) {
		rt_hw_serial_dma_tx_isr(&uart3_device);
	}

#endif

	/* leave interrupt */
	rt_interrupt_leave();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* enter interrupt */
	rt_interrupt_enter();

#ifdef RT_USING_UART1

	if(huart == uart1.huart) {
		rt_hw_serial_rx_isr(&uart1_device);

	}

#endif

#ifdef RT_USING_UART3

	if(huart == uart3.huart) {
		rt_hw_serial_rx_isr(&uart3_device);
	}

#endif

	/* leave interrupt */
	rt_interrupt_leave();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
}
void HAL_UART_AbortCpltCallback (UART_HandleTypeDef *huart)
{
}
void HAL_UART_AbortTransmitCpltCallback (UART_HandleTypeDef *huart)
{
}
void HAL_UART_AbortReceiveCpltCallback (UART_HandleTypeDef *huart)
{
}

