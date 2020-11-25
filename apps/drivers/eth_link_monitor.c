

/*================================================================
 *
 *
 *   文件名称：eth_link_monitor.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月19日 星期四 13时58分57秒
 *   修改日期：2020年11月25日 星期三 16时33分45秒
 *   描    述：
 *
 *================================================================*/
#include "eth_link_monitor.h"

extern ETH_HandleTypeDef heth;

static void eth_link_thread_entry(void *parameter)
{
	int phy_status = RT_FALSE;
	int link_status = RT_FALSE;
	struct eth_device *dev = (struct eth_device *)parameter;

	while(1) {
		uint32_t regvalue = 0;

		HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &regvalue);
		regvalue &= PHY_LINKED_STATUS;

		if(regvalue != 0) {
			phy_status = RT_TRUE;
		} else {
			phy_status = RT_FALSE;
		}

		if(link_status != phy_status) {
			link_status = phy_status;
			eth_device_linkchange(dev, link_status);
		}

		rt_thread_mdelay(50);
	}
}

void start_eth_link_monitor(struct eth_device *dev)
{
	rt_thread_t tid;

	tid = rt_thread_create("link_mon", eth_link_thread_entry, dev, 2048, RT_THREAD_PRIORITY_MAX / 3, 20);
	RT_ASSERT(tid != RT_NULL);

	rt_thread_startup(tid);
}
