

/*================================================================
 *
 *
 *   文件名称：eth.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月25日 星期三 14时01分47秒
 *   修改日期：2020年11月25日 星期三 17时39分54秒
 *   描    述：
 *
 *================================================================*/
#include "eth.h"
/*
 * STM32 Eth Driver for RT-Thread
 * Change Logs:
 * Date           Author       Notes
 * 2009-10-05     Bernard      eth interface driver for STM32F107 CL
 */
#include <rtthread.h>
#include <netif/ethernetif.h>
#include <lwip/prot/ethernet.h>
#include "lwipopts.h"
#include "app_platform.h"
#include "eth_link_monitor.h"
#include "os_utils.h"

extern ETH_HandleTypeDef heth;

/* Ethernet Rx & Tx DMA Descriptors */
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB], DMATxDscrTab[ETH_TXBUFNB];

/* Ethernet Receive buffers  */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE];

/* Ethernet Transmit buffers */
uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE];

#define MAX_ADDR_LEN 6
struct rt_stm32_eth {
	/* inherit from ethernet device */
	struct eth_device parent;

	/* interface address info. */
	rt_uint8_t  dev_addr[MAX_ADDR_LEN];			/* hw address	*/
};
static struct rt_stm32_eth stm32_eth_device;
static struct rt_semaphore rx_wait;

/* RT-Thread Device Interface */
/* initialize the interface */
static rt_err_t rt_stm32_eth_init(rt_device_t dev)
{
#if 0
#if LWIP_ARP || LWIP_ETHERNET
	uint32_t regvalue = 0;
	struct rt_stm32_eth *eth = (struct rt_stm32_eth *)dev->user_data;
	struct netif *netif = eth->parent.netif;

	/* set MAC hardware address length */
	netif->hwaddr_len = ETH_HWADDR_LEN;

	/* set MAC hardware address */
	netif->hwaddr[0] =  heth.Init.MACAddr[0];
	netif->hwaddr[1] =  heth.Init.MACAddr[1];
	netif->hwaddr[2] =  heth.Init.MACAddr[2];
	netif->hwaddr[3] =  heth.Init.MACAddr[3];
	netif->hwaddr[4] =  heth.Init.MACAddr[4];
	netif->hwaddr[5] =  heth.Init.MACAddr[5];

	/* maximum transfer unit */
	netif->mtu = 1500;

	/* Accept broadcast address and ARP traffic */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
#if LWIP_ARP
	netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
	netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */


	/* Enable MAC and DMA transmission and reception */
	HAL_ETH_Start(&heth);

	/* USER CODE BEGIN PHY_PRE_CONFIG */

	/* USER CODE END PHY_PRE_CONFIG */


	/* Read Register Configuration */
	HAL_ETH_ReadPHYRegister(&heth, PHY_ISFR, &regvalue);
	regvalue |= (PHY_ISFR_INT4);

	/* Enable Interrupt on change of link status */
	HAL_ETH_WritePHYRegister(&heth, PHY_ISFR, regvalue );

	/* Read Register Configuration */
	HAL_ETH_ReadPHYRegister(&heth, PHY_ISFR, &regvalue);

	/* USER CODE BEGIN PHY_POST_CONFIG */

	/* USER CODE END PHY_POST_CONFIG */

#endif /* LWIP_ARP || LWIP_ETHERNET */

	/* USER CODE BEGIN LOW_LEVEL_INIT */

	/* USER CODE END LOW_LEVEL_INIT */
#endif

	return RT_EOK;
}

static rt_err_t rt_stm32_eth_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}

static rt_err_t rt_stm32_eth_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t rt_stm32_eth_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

static rt_size_t rt_stm32_eth_write (rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

static rt_err_t rt_stm32_eth_control(rt_device_t dev, int cmd, void *args)
{
	switch(cmd) {
		case NIOCTL_GADDR:

			/* get mac address */
			if(args) {
				rt_memcpy(args, stm32_eth_device.dev_addr, 6);
			} else {
				return -RT_ERROR;
			}

			break;

		default :
			break;
	}

	return RT_EOK;
}

void show_frame(struct pbuf *q)
{
	int i = 0;
	char *ptr = q->payload;

	for( i = 0; i < q->len; i++ ) {
		rt_kprintf("0x%02X ", *(ptr++));
	}

	rt_kprintf("\n");
}


void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
	rt_sem_release(&rx_wait);
}

/* reception packet. */
struct pbuf *rt_stm32_eth_rx(rt_device_t dev)
{
#if 0
	struct pbuf *p = NULL;
	struct pbuf *q = NULL;
	uint16_t len = 0;
	uint8_t *buffer;
	__IO ETH_DMADescTypeDef *dmarxdesc;
	uint32_t bufferoffset = 0;
	uint32_t payloadoffset = 0;
	uint32_t byteslefttocopy = 0;
	uint32_t i = 0;

	if(rt_sem_take(&rx_wait, 250) != RT_EOK) {
		return NULL;
	}

	/* get received frame */
	if (HAL_ETH_GetReceivedFrame_IT(&heth) != HAL_OK) {
		return NULL;
	}

	/* Obtain the size of the packet and put it into the "len" variable. */
	len = heth.RxFrameInfos.length;
	buffer = (uint8_t *)heth.RxFrameInfos.buffer;

	if (len > 0) {
		/* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
		p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
	}

	if (p != NULL) {
		dmarxdesc = heth.RxFrameInfos.FSRxDesc;
		bufferoffset = 0;

		for(q = p; q != NULL; q = q->next) {
			byteslefttocopy = q->len;
			payloadoffset = 0;

			/* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
			while( (byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE ) {
				/* Copy data to pbuf */
				memcpy( (uint8_t *)((uint8_t *)q->payload + payloadoffset), (uint8_t *)((uint8_t *)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));

				/* Point to next descriptor */
				dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
				buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);

				byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
				payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
				bufferoffset = 0;
			}

			/* Copy remaining data in pbuf */
			memcpy( (uint8_t *)((uint8_t *)q->payload + payloadoffset), (uint8_t *)((uint8_t *)buffer + bufferoffset), byteslefttocopy);
			bufferoffset = bufferoffset + byteslefttocopy;
		}
	}

	/* Release descriptors to DMA */
	/* Point to first descriptor */
	dmarxdesc = heth.RxFrameInfos.FSRxDesc;

	/* Set Own bit in Rx descriptors: gives the buffers back to DMA */
	for (i = 0; i < heth.RxFrameInfos.SegCount; i++) {
		dmarxdesc->Status |= ETH_DMARXDESC_OWN;
		dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth.RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it and resume reception */
	if ((heth.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		heth.Instance->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		heth.Instance->DMARPDR = 0;
	}

	return p;
#else
	return NULL;
#endif
}

/* ethernet device interface */
/* transmit packet. */
rt_err_t rt_stm32_eth_tx( rt_device_t dev, struct pbuf *p)
{
#if 0
	err_t errval;
	struct pbuf *q;
	uint8_t *buffer = (uint8_t *)(heth.TxDesc->Buffer1Addr);
	__IO ETH_DMADescTypeDef *DmaTxDesc;
	uint32_t framelength = 0;
	uint32_t bufferoffset = 0;
	uint32_t byteslefttocopy = 0;
	uint32_t payloadoffset = 0;
	DmaTxDesc = heth.TxDesc;
	bufferoffset = 0;

	return ERR_OK;

	/* copy frame from pbufs to driver buffers */
	for(q = p; q != NULL; q = q->next) {
		/* Is this buffer available? If not, goto error */
		if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET) {
			errval = ERR_USE;
			goto error;
		}

		/* Get bytes in current lwIP buffer */
		byteslefttocopy = q->len;
		payloadoffset = 0;

		/* Check if the length of data to copy is bigger than Tx buffer size*/
		while( (byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE ) {
			/* Copy data to Tx buffer*/
			memcpy( (uint8_t *)((uint8_t *)buffer + bufferoffset), (uint8_t *)((uint8_t *)q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset) );

			/* Point to next descriptor */
			DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

			/* Check if the buffer is available */
			if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET) {
				errval = ERR_USE;
				goto error;
			}

			buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

			byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
			payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
			framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
			bufferoffset = 0;
		}

		/* Copy the remaining bytes */
		memcpy( (uint8_t *)((uint8_t *)buffer + bufferoffset), (uint8_t *)((uint8_t *)q->payload + payloadoffset), byteslefttocopy );
		bufferoffset = bufferoffset + byteslefttocopy;
		framelength = framelength + byteslefttocopy;
	}

	/* Prepare transmit descriptors to give to DMA */
	HAL_ETH_TransmitFrame(&heth, framelength);

	errval = ERR_OK;

error:

	/* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
	if ((heth.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth.Instance->DMASR = ETH_DMASR_TUS;

		/* Resume DMA transmission*/
		heth.Instance->DMATPDR = 0;
	}

	return errval;
#else
	return ERR_OK;
#endif
}

void rt_hw_stm32_eth_init(void)
{
	stm32_eth_device.dev_addr[0] = heth.Init.MACAddr[0];
	stm32_eth_device.dev_addr[1] = heth.Init.MACAddr[1];
	stm32_eth_device.dev_addr[2] = heth.Init.MACAddr[2];
	stm32_eth_device.dev_addr[3] = heth.Init.MACAddr[3];
	stm32_eth_device.dev_addr[4] = heth.Init.MACAddr[4];
	stm32_eth_device.dev_addr[5] = heth.Init.MACAddr[5];

	/* Initialize Tx Descriptors list: Chain Mode */
	HAL_ETH_DMATxDescListInit(&heth, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);

	/* Initialize Rx Descriptors list: Chain Mode  */
	HAL_ETH_DMARxDescListInit(&heth, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

	stm32_eth_device.parent.parent.init = rt_stm32_eth_init;
	stm32_eth_device.parent.parent.open = rt_stm32_eth_open;
	stm32_eth_device.parent.parent.close = rt_stm32_eth_close;
	stm32_eth_device.parent.parent.read = rt_stm32_eth_read;
	stm32_eth_device.parent.parent.write = rt_stm32_eth_write;
	stm32_eth_device.parent.parent.control = rt_stm32_eth_control;
	stm32_eth_device.parent.parent.user_data = &stm32_eth_device;

	stm32_eth_device.parent.eth_rx = rt_stm32_eth_rx;
	stm32_eth_device.parent.eth_tx = rt_stm32_eth_tx;

	/* init tx semaphore */
	rt_sem_init(&rx_wait, "rx_wait", 1, RT_IPC_FLAG_FIFO);

	/* register eth device */
	eth_device_init(&(stm32_eth_device.parent), "e0");
	//start_eth_link_monitor(&(stm32_eth_device.parent));
}
