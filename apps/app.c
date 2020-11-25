

/*================================================================
 *
 *
 *   文件名称：app.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月25日 星期三 13时11分59秒
 *   修改日期：2020年11月25日 星期三 13时25分22秒
 *   描    述：
 *
 *================================================================*/
#include "app.h"
#include <rtthread.h>

#include "usb_host.h"

static void usb_host_entry(void *parameter)
{
	while(1) {
		MX_USB_HOST_Process();
		rt_thread_mdelay(10);
	}
}


void start_app(void)
{
	rt_thread_t tid;

	tid = rt_thread_create("usb_host",
	                       usb_host_entry, RT_NULL,
	                       2048, RT_THREAD_PRIORITY_MAX / 3, 20);

	if (tid != RT_NULL) {
		rt_thread_startup(tid);
	}
}
