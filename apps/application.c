/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_FINSH
#include <shell.h>
#include <finsh.h>
#endif

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs.h>
/* dfs filesystem:ELM filesystem init */
#ifdef RT_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#include "app_platform.h"
#include "app.h"
#include "iwdg.h"
#include "hw_pin.h"

extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim4;
extern RNG_HandleTypeDef hrng;

void rt_hw_stm32_eth_init(void);

typedef enum {
	PWM_COMPARE_COUNT_UP = 0,
	PWM_COMPARE_COUNT_DOWN,
	PWM_COMPARE_COUNT_KEEP,
} compare_count_type_t;

static void update_work_led(void)
{
	static compare_count_type_t type = PWM_COMPARE_COUNT_UP;
	static uint16_t duty_cycle = 0;
	static uint16_t keep_count = 0;
	//计数值小于duty_cycle,输出1;大于duty_cycle输出0

	switch(type) {
		case PWM_COMPARE_COUNT_UP: {

			if(duty_cycle < 1000) {
				duty_cycle += 5;
			} else {
				type = PWM_COMPARE_COUNT_KEEP;
			}
		}
		break;

		case PWM_COMPARE_COUNT_DOWN: {
			if(duty_cycle > 0) {
				duty_cycle -= 5;
			} else {
				type = PWM_COMPARE_COUNT_UP;
			}

		}
		break;

		case PWM_COMPARE_COUNT_KEEP: {
			if(keep_count < duty_cycle) {
				keep_count += 10;
			} else {
				keep_count = 0;
				type = PWM_COMPARE_COUNT_DOWN;
			}

		}
		break;

		default:
			break;
	}

	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty_cycle);
}

static int rt_tick_enable = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();

		if(rt_tick_enable == 1) {
			/* enter interrupt */
			rt_interrupt_enter();

			rt_tick_increase();

			/* leave interrupt */
			rt_interrupt_leave();
		}
	}

	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
	HAL_IncTick();

	if(rt_tick_enable == 1) {
		/* enter interrupt */
		rt_interrupt_enter();

		rt_tick_increase();

		/* leave interrupt */
		rt_interrupt_leave();
	}
}

void __wrap_srand(unsigned int seed)
{
	return HAL_RNG_GetRandomNumber(&hrng);
}

int __wrap_rand(void)
{
	return HAL_RNG_GetRandomNumber(&hrng);
}

static void idle()
{
	MX_IWDG_Init();
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

	while(1) {
		HAL_IWDG_Refresh(&hiwdg);
		update_work_led();
		rt_thread_mdelay(1);
	}
}

void rt_init_thread_entry(void *parameter)
{
	rt_tick_enable = 1;

	rt_hw_pin_init();

#ifdef RT_USING_COMPONENTS_INIT
	/* RT-Thread components initialization */
	rt_components_init();
#endif

	/* Filesystem Initialization */
#ifdef RT_USING_DFS
	{
		/* init sdcard driver */
//#if STM32_USE_SDIO
//		rt_hw_sdcard_init();
//#else
//		rt_hw_msd_init();
//#endif

		/* init the device filesystem */
		//dfs_init();

#ifdef RT_USING_DFS_ELMFAT
		/* init the elm chan FatFs filesystam*/
		//elm_init();

		rt_hw_usb_msc_init();

		///* mount sd card fat partition 1 as root directory */
		//if (dfs_mount("sd0", "/", "elm", 0, 0) == 0) {
		//	rt_kprintf("File System initialized!\n");
		//} else {
		//	rt_kprintf("File System initialzation failed!\n");
		//}
#endif
	}
#endif

	/* LwIP Initialization */
#ifdef RT_USING_LWIP
	{
		extern void lwip_sys_init(void);

		/* register ethernetif device */
		eth_system_device_init();

		/* initialize eth interface */
		rt_hw_stm32_eth_init();

		/* init lwip system */
		lwip_sys_init();
		rt_kprintf("TCP/IP initialized!\n");
	}
#endif

	//rt_hw_rtc_init();

#ifdef RT_USING_FINSH
	/* init finsh */
	finsh_system_init();
#endif
	start_app();
	idle();
}

int rt_application_init()
{
	rt_thread_t tid;

	tid = rt_thread_create("init",
	                       rt_init_thread_entry, RT_NULL,
	                       2048, RT_THREAD_PRIORITY_MAX / 3, 20);

	if (tid != RT_NULL) {
		rt_thread_startup(tid);
	}

	return 0;
}

/*@}*/
