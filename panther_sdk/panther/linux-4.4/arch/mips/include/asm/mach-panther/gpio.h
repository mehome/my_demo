/* 
 *  Copyright (c) 2008, 2009, 2017    Montage Technology Group Limited     All rights reserved.
 */
#ifndef __ASM_PANTHER_GPIO_H
#define __ASM_PANTHER_GPIO_H

#include <linux/kernel.h>

#define PANTHER_GPIO_LINES      32      /* GPIO[0~31] */
#define PLATFORM_GPIO_NAME      "panther-gpio"

// GPIO registers in PMU
#define GPIO_FUNC_EN      (0x2C)
#define GPIO_ODS          (0x30)
#define GPIO_ODC          (0x34)
#define GPIO_OEN          (0x38)
#define GPIO_OUTPUT_DATA  (0x3C)
#define GPIO_INT_RAISING  (0x40)
#define GPIO_INT_FALLING  (0x44)
#define GPIO_INT_HIGH     (0x48)
#define GPIO_INT_LOW      (0x4C)
#define GPIO_INT_MASK     (0x50)
#define GPIO_INT_STATUS   (0x54)
#define GPIO_ID           (0x58)

void panther_raw_gpio_irq_mask(int gpio);
void panther_raw_gpio_irq_unmask(int gpio);
void panther_raw_gpio_irq_clear_unmask(int gpio);
void camelot_gpio_set_value(unsigned gpio, int value);
int panther_raw_gpio_get_value(unsigned gpio);
int camelot_gpio_direction_input(unsigned gpio);
int camelot_gpio_direction_output(unsigned gpio, int value);
int panther_raw_gpio_request(unsigned int gpio);

#ifndef CONFIG_GPIOLIB
extern int camelot_gpio_to_irq(unsigned gpio);
extern int camelot_gpio_get_value(unsigned gpio);
extern void camelot_gpio_set_value(unsigned gpio, int value);
extern int camelot_gpio_direction_input(unsigned gpio);
extern int camelot_gpio_direction_output(unsigned gpio, int value);

static inline int gpio_request(unsigned gpio, const char *label)
{
    return 0;
}
static inline void gpio_free(unsigned gpio)
{
}
static inline int gpio_to_irq(unsigned gpio)
{
	return camelot_gpio_to_irq(gpio);
}
static inline int gpio_get_value(unsigned gpio)
{
	return camelot_gpio_get_value(gpio);
}
static inline void gpio_set_value(unsigned gpio, int value)
{
	camelot_gpio_set_value(gpio, value);
}
static inline int gpio_direction_input(unsigned gpio)
{
	return camelot_gpio_direction_input(gpio);
}

static inline int gpio_direction_output(unsigned gpio, int value)
{
	return camelot_gpio_direction_output(gpio, value);
}
#endif

#endif /* __ASM_PANTHER_GPIO_H */
