

/*================================================================
 *   
 *   
 *   文件名称：hw_pin.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月30日 星期一 10时56分24秒
 *   修改日期：2020年11月30日 星期一 11时46分57秒
 *   描    述：
 *
 *================================================================*/
#ifndef _HW_PIN_H
#define _HW_PIN_H
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
int rt_hw_pin_init(void);
#endif //_HW_PIN_H
