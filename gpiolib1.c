#include <aio.h>
#include <stdint.h>
#include <stdlib.h>
#include <gpiod.h>

#include "gpiolib1.h"

struct gpiolib {
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	bool oe;
	bool oval;
};

void *gpio_alloc(const char *chip_path, unsigned int line, bool oe, bool oval)
{
	GPIOL1 *gpio = malloc(sizeof(struct gpiolib));

	if (gpio == NULL)
		goto out;

	gpio->chip = gpiod_chip_open(chip_path);
	if (gpio->chip == NULL)
		goto out_free;

	gpio->line = gpiod_chip_get_line(gpio->chip, line);
	if (gpio->line == NULL)
		goto out_chip;

	/* Set up IO based on parameters passed */
	if (oe) {
		if (gpiod_line_request_output(gpio->line, "gpiolib1", (int)oval) == -1)
			goto out_chip;

		gpio->oe = oe;
		gpio->oval = oval;
	} else {
		if (gpiod_line_request_input(gpio->line, "gpiolib1") == -1)
			goto out_chip;

		gpio->oe = oe;
		gpio->oval = false;
	}

	return (void*)gpio;

out_chip:
	gpiod_chip_close(gpio->chip);
out_free:
	free(gpio);
out:
	return NULL;
}

void gpio_free(GPIOL1 *gpio)
{
	gpiod_line_release(gpio->line);
	gpiod_chip_close(gpio->chip);
	free(gpio);
}

int gpio_direction_set(GPIOL1 *gpio, bool oe, bool oval)
{
	int ret;
	if (oe) {
		ret = gpiod_line_set_direction_output(gpio->line, oval);
		gpio->oe = oe;
		gpio->oval = oval;
	} else {
		ret = gpiod_line_set_direction_input(gpio->line);
		gpio->oe = oe;
		gpio->oval = false;
	}

	return ret;
}

bool gpio_direction_get(GPIOL1 *gpio)
{
	return gpio->oe;
}

/* I believe this will return -1 if the line is an input */
/* TODO: Consider making this always force a direction change to mitigate it
 * returning errors?
 */
int gpio_oval_set(GPIOL1 *gpio, bool oval)
{
	gpio->oval = oval;
	return gpiod_line_set_value(gpio->line, oval);
}

bool gpio_oval_get(GPIOL1 *gpio)
{
	return gpio->oval;
}

/* Returns -1 on error */
/* TODO: Consider making this always force a direction change to mitigate it
 * returning errors?
 */
int gpio_ival_get(GPIOL1 *gpio)
{
	return gpiod_line_get_value(gpio->line);
}

int gpio_oval_toggle(GPIOL1 *gpio)
{
	return gpio_oval_set(gpio, !(gpio_oval_get(gpio)));
}

/* Could also include select() style funcs like original gpiolib*/
