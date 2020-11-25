

/*================================================================
 *   
 *   
 *   文件名称：eth_link_monitor.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月19日 星期四 14时21分45秒
 *   修改日期：2020年11月25日 星期三 16时27分45秒
 *   描    述：
 *
 *================================================================*/
#ifndef _ETH_LINK_MONITOR_H
#define _ETH_LINK_MONITOR_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <rtthread.h>
#include <netif/ethernetif.h>
#include "lwipopts.h"
#include "app_platform.h"

#ifdef __cplusplus
}
#endif
void start_eth_link_monitor(struct eth_device * dev);
#endif //_ETH_LINK_MONITOR_H
