

/*================================================================
 *   
 *   
 *   文件名称：usart.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月24日 星期二 10时36分40秒
 *   修改日期：2020年11月24日 星期二 14时14分33秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USART_H
#define _USART_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <rtthread.h>
#include "app_platform.h"

#ifdef __cplusplus
}
#endif
void rt_hw_usart_init();
#endif //_USART_H
