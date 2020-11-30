

/*================================================================
 *
 *
 *   文件名称：hw_pin.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月30日 星期一 10时56分18秒
 *   修改日期：2020年11月30日 星期一 11时45分14秒
 *   描    述：
 *
 *================================================================*/
#include "hw_pin.h"
#include "drivers/pin.h"

#define debug(fmt, ...) rt_kprintf("[%s:%s:%d] " fmt, __FILE__, __func__, __LINE__, ## __VA_ARGS__)

typedef struct {
	uint8_t hw_pin : 4;
	uint8_t hw_port : 4;
} hw_pin_port_t;

typedef union {
	hw_pin_port_t s;
	uint8_t v;
} u_hw_pin_port_t;

#define add_case_gpio_to_hw(gpio, hw) \
	case (uint32_t)gpio: { \
		v = hw; \
	} \
	break

static uint8_t gpio_port_to_hw_port(GPIO_TypeDef *GPIOx)
{
	uint8_t v = 0xf;

	switch((uint32_t)GPIOx) {
			add_case_gpio_to_hw(GPIOA, 0);
			add_case_gpio_to_hw(GPIOB, 1);
			add_case_gpio_to_hw(GPIOC, 2);
			add_case_gpio_to_hw(GPIOD, 3);
			add_case_gpio_to_hw(GPIOE, 4);
			add_case_gpio_to_hw(GPIOF, 5);
			add_case_gpio_to_hw(GPIOG, 6);
			add_case_gpio_to_hw(GPIOH, 7);
			add_case_gpio_to_hw(GPIOI, 8);

		default: {
		}
		break;
	}

	return v;
}

static uint8_t gpio_pin_to_hw_pin(uint16_t GPIO_Pin)
{
	uint8_t v = 0xf;

	switch(GPIO_Pin) {
			add_case_gpio_to_hw(GPIO_PIN_0, 0);
			add_case_gpio_to_hw(GPIO_PIN_1, 1);
			add_case_gpio_to_hw(GPIO_PIN_2, 2);
			add_case_gpio_to_hw(GPIO_PIN_3, 3);
			add_case_gpio_to_hw(GPIO_PIN_4, 4);
			add_case_gpio_to_hw(GPIO_PIN_5, 5);
			add_case_gpio_to_hw(GPIO_PIN_6, 6);
			add_case_gpio_to_hw(GPIO_PIN_7, 7);
			add_case_gpio_to_hw(GPIO_PIN_8, 8);
			add_case_gpio_to_hw(GPIO_PIN_9, 9);
			add_case_gpio_to_hw(GPIO_PIN_10, 10);
			add_case_gpio_to_hw(GPIO_PIN_11, 11);
			add_case_gpio_to_hw(GPIO_PIN_12, 12);
			add_case_gpio_to_hw(GPIO_PIN_13, 13);
			add_case_gpio_to_hw(GPIO_PIN_14, 14);
			add_case_gpio_to_hw(GPIO_PIN_15, 15);

		default: {
		}
		break;
	}

	return v;
}

#define add_case_hw_to_gpio(hw, gpio) \
	case hw: { \
		v = gpio; \
	} \
	break

static GPIO_TypeDef *hw_port_to_gpio_port(uint8_t hw_port)
{
	GPIO_TypeDef *v = NULL;

	switch(hw_port) {
			add_case_gpio_to_hw(0, GPIOA);
			add_case_gpio_to_hw(1, GPIOB);
			add_case_gpio_to_hw(2, GPIOC);
			add_case_gpio_to_hw(3, GPIOD);
			add_case_gpio_to_hw(4, GPIOE);
			add_case_gpio_to_hw(5, GPIOF);
			add_case_gpio_to_hw(6, GPIOG);
			add_case_gpio_to_hw(7, GPIOH);
			add_case_gpio_to_hw(8, GPIOI);

		default: {
		}
		break;
	}

	return v;
}

static uint16_t hw_pin_to_gpio_pin(uint8_t hw_pin)
{
	uint16_t v = 0xffff;

	switch(hw_pin) {
			add_case_gpio_to_hw(0, GPIO_PIN_0);
			add_case_gpio_to_hw(1, GPIO_PIN_1);
			add_case_gpio_to_hw(2, GPIO_PIN_2);
			add_case_gpio_to_hw(3, GPIO_PIN_3);
			add_case_gpio_to_hw(4, GPIO_PIN_4);
			add_case_gpio_to_hw(5, GPIO_PIN_5);
			add_case_gpio_to_hw(6, GPIO_PIN_6);
			add_case_gpio_to_hw(7, GPIO_PIN_7);
			add_case_gpio_to_hw(8, GPIO_PIN_8);
			add_case_gpio_to_hw(9, GPIO_PIN_9);
			add_case_gpio_to_hw(10, GPIO_PIN_10);
			add_case_gpio_to_hw(11, GPIO_PIN_11);
			add_case_gpio_to_hw(12, GPIO_PIN_12);
			add_case_gpio_to_hw(13, GPIO_PIN_13);
			add_case_gpio_to_hw(14, GPIO_PIN_14);
			add_case_gpio_to_hw(15, GPIO_PIN_15);

		default: {
		}
		break;
	}

	return v;
}


static void stm32_pin_mode(rt_device_t dev, rt_base_t pin, rt_base_t mode)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	debug("pin:%d, mode:%d\n", pin, mode);

	if (mode == PIN_MODE_OUTPUT) {
		/* output setting */
	} else if (mode == PIN_MODE_INPUT) {
		/* input setting: not pull. */
	} else if (mode == PIN_MODE_INPUT_PULLUP) {
		/* input setting: pull up. */
	} else if (mode == PIN_MODE_INPUT_PULLDOWN) {
		/* input setting: pull down. */
	} else if (mode == PIN_MODE_OUTPUT_OD) {
		/* output setting: od. */
	}
}

static void stm32_pin_write(rt_device_t dev, rt_base_t pin, rt_base_t value)
{
	u_hw_pin_port_t u_hw_pin_port;

	u_hw_pin_port.v = pin;

	debug("pin:%d, value:%d\n", pin, value);

	HAL_GPIO_WritePin(hw_port_to_gpio_port(u_hw_pin_port.s.hw_port), hw_pin_to_gpio_pin(u_hw_pin_port.s.hw_pin), (GPIO_PinState)value);
}

static int stm32_pin_read(rt_device_t dev, rt_base_t pin)
{
	int value = PIN_LOW;
	u_hw_pin_port_t u_hw_pin_port;

	u_hw_pin_port.v = pin;
	value = HAL_GPIO_ReadPin(hw_port_to_gpio_port(u_hw_pin_port.s.hw_port), hw_pin_to_gpio_pin(u_hw_pin_port.s.hw_pin));

	debug("pin:%d, value:%d\n", pin, value);

	return value;
}

static rt_err_t stm32_pin_attach_irq(struct rt_device *device, rt_int32_t pin, rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
	rt_base_t level;
	rt_int32_t irqindex = -1;

	debug("pin:%d\n", pin);

	level = rt_hw_interrupt_disable();

	rt_hw_interrupt_enable(level);

	return RT_EOK;
}

static rt_err_t stm32_pin_dettach_irq(struct rt_device *device, rt_int32_t pin)
{
	rt_base_t level;

	debug("pin:%d\n", pin);

	level = rt_hw_interrupt_disable();

	rt_hw_interrupt_enable(level);

	return RT_EOK;
}

static rt_err_t stm32_pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint32_t enabled)
{
	rt_base_t level;

	debug("pin:%d\n", pin);

	level = rt_hw_interrupt_disable();

	rt_hw_interrupt_enable(level);

	return RT_EOK;
}

static rt_base_t stm32_pin_get(const char *name)
{
	rt_base_t pin = 0;
	int hw_port_num, hw_pin_num = 0;
	int i, name_len;
	u_hw_pin_port_t u_hw_pin_port;

	name_len = rt_strlen(name);

	if ((name_len < 4) || (name_len >= 6)) {
		return -RT_EINVAL;
	}

	if ((name[0] != 'P') || (name[2] != '.')) {
		return -RT_EINVAL;
	}

	if ((name[1] >= 'A') && (name[1] <= 'Z')) {
		hw_port_num = (int)(name[1] - 'A');
	} else {
		return -RT_EINVAL;
	}

	for (i = 3; i < name_len; i++) {
		hw_pin_num *= 10;
		hw_pin_num += name[i] - '0';
	}

	debug("hw_pin_num:%d, hw_port_num:%d\n", hw_pin_num, hw_port_num);

	u_hw_pin_port.s.hw_pin = hw_pin_num;
	u_hw_pin_port.s.hw_port = hw_port_num;

	pin = u_hw_pin_port.v;

	return pin;
}

const static struct rt_pin_ops _stm32_pin_ops = {
	stm32_pin_mode,
	stm32_pin_write,
	stm32_pin_read,
	stm32_pin_attach_irq,
	stm32_pin_dettach_irq,
	stm32_pin_irq_enable,
	stm32_pin_get,
};


int rt_hw_pin_init(void)
{
	return rt_device_pin_register("pin", &_stm32_pin_ops, RT_NULL);
}
