

/*================================================================
 *
 *
 *   文件名称：usbh_msc_diskio.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月30日 星期一 14时50分32秒
 *   修改日期：2020年11月30日 星期一 16时02分12秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USBH_MSC_DISKIO_H
#define _USBH_MSC_DISKIO_H
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
void rt_hw_usb_msc_init();
void usbh_msc_mount(void);
void usbh_msc_umount(void);
#endif //_USBH_MSC_DISKIO_H
