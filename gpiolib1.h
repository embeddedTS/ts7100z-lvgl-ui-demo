#ifndef __GPIOLIB1_H__
#define __GPIOLIB1_H__
#include <stdint.h>

typedef struct gpiolib GPIOL1;

void *gpio_alloc(const char *chip_path, unsigned int line, bool oe, bool oval);

void gpio_free(GPIOL1 *gpio);

int gpio_direction_set(GPIOL1 *gpio, bool oe, bool oval);

bool gpio_direction_get(GPIOL1 *gpio);

/* I believe this will return -1 if the line is an input */
/* TODO: Consider making this always force a direction change to mitigate it
 * returning errors?
 */
int gpio_oval_set(GPIOL1 *gpio, bool oval);

bool gpio_oval_get(GPIOL1 *gpio);

/* Returns -1 on error */
/* TODO: Consider making this always force a direction change to mitigate it
 * returning errors?
 */
int gpio_ival_get(GPIOL1 *gpio);
	
int gpio_oval_toggle(GPIOL1 *gpio);

/* Could also include select() style funcs like original gpiolib*/
#endif // __GPIOLIB1_H__
