#include <stdint.h>
#include <stdio.h>
#include "lvgl/lvgl.h"

#include "gpiolib1.h"
#include "main.h"
#include "gpio.h"

/* Styles used for these buttons */
static lv_style_t style_relay_btn;
static lv_style_t style_gpio_btn;
static lv_style_t style_led;

struct gpio_desc {
    const char *chip_path;
    int line;
    const char *label;
    lv_style_t *style;
    void *gpio;
    lv_obj_t *obj;
};

static struct gpio_desc gpio_relay_desc[] = {
    { "/dev/gpiochip4", 4, "Relay 1", &style_relay_btn, NULL, NULL},
    { "/dev/gpiochip4", 5, "Relay 2", &style_relay_btn, NULL, NULL},
    {},
};

static struct gpio_desc gpio_btn_desc[] = {
    { "/dev/gpiochip5", 0, "GPIO\n5 0", &style_gpio_btn, NULL, NULL},
    { "/dev/gpiochip5", 1, "GPIO\n5 1", &style_gpio_btn, NULL, NULL},
    { "/dev/gpiochip6", 13, "GPIO\n6 13", &style_gpio_btn, NULL, NULL},
    {},
};

static struct gpio_desc gpio_hs_desc[] = {
    { "/dev/gpiochip5", 15, "GPIO\n6 15", &style_gpio_btn, NULL, NULL},
    {},
};

struct gpio_out_group {
    struct gpio_desc *desc;
};

static struct gpio_out_group gpio_out_group[] = {
    { gpio_relay_desc },
    { gpio_btn_desc },
    { gpio_hs_desc },
    {},
};

static struct gpio_desc gpio_led_desc[] = {
    { "/dev/gpiochip5", 2, "GPIO\n5 2", &style_led, NULL, NULL},
    { "/dev/gpiochip5", 3, "GPIO\n5 3", &style_led, NULL, NULL},
    { "/dev/gpiochip5", 4, "GPIO\n5 4", &style_led, NULL, NULL},
    {},
};

static void btn_gpio_set_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    void *gpio = lv_event_get_user_data(e);

    gpio_oval_set(gpio, !!(lv_obj_get_state(btn) & LV_STATE_CHECKED));
}

static void led_service(lv_timer_t * timer)
{
    lv_obj_t *led = timer->user_data;
    if (gpio_ival_get(led->user_data)) {
        lv_led_on(led);
    } else {
        lv_led_off(led);
    }
}

static bool gpio_is_init = false;
void gpio_claim_all_and_set_cb(void)
{
    int x, y;
    struct gpio_desc *desc;

    if (gpio_is_init) return;

    /* Grab the GPIO and set up callbacks for the out group */
    for (x = 0; ; x++) {
        if (gpio_out_group[x].desc == NULL)
            break;

        desc = gpio_out_group[x].desc;

        for (y = 0; ; y++) {
            /* No real error handling, just bail if the button object was not
             * actually allocated ahead of time.
             */
            if (desc[y].chip_path == NULL || desc[y].obj == NULL)
                break;

            /* Open and claim the button's associated GPIO */
            /* TODO: Error check */
            desc[y].gpio = gpio_alloc(desc[y].chip_path, desc[y].line, 1, 0);
            lv_obj_add_event_cb(desc[y].obj, btn_gpio_set_event_cb,
                LV_EVENT_VALUE_CHANGED, desc[y].gpio);
        }
    }

    desc = gpio_led_desc;
    for (y = 0; ; y++) {
        /* No real error handling, just bail if the LED object was not actually
         * allocated ahead of time.
         */
        if (desc[y].chip_path == NULL || desc[y].obj == NULL)
            break;

        /* Open and claim the LED's associated GPIO */
        /* TODO: Error check */
        desc[y].gpio = gpio_alloc(desc[y].chip_path, desc[y].line, 0, 0);

        /* Set up a timer to call the LED servicing routine every 100 ms */
        lv_obj_set_user_data(desc[y].obj, desc[y].gpio);
        lv_timer_create(led_service, 100, desc[y].obj);
    }

    gpio_is_init = true;
}

/* Does not do any atyle setup with the font and relies on defaults and/or
 * text recolor commands.
 */
static lv_obj_t *label_create_center(lv_obj_t *cont, const char *label)
{
    /* Create and set up label */
    lv_obj_t *label_obj = lv_label_create(cont);
    lv_label_set_recolor(label_obj, true);
    lv_label_set_text_static(label_obj, label);
    lv_obj_center(label_obj);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);

    return label_obj;
}

static lv_obj_t *btn_gpio_set_create(struct gpio_desc *desc, lv_obj_t *cont)
{
    /* Create button widget, set up style and flags. The associated GPIO and
     * its callback are set up later to allow for a lazy/JIT init so the GPIO
     * lines arn't consumed if the demo just stays on the main screen.
     */
    desc->obj = lv_btn_create(cont);
    lv_obj_add_style(desc->obj, desc->style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(desc->obj, LV_OBJ_FLAG_CHECKABLE);

    label_create_center(desc->obj, desc->label);

    return desc->obj;
}

static lv_obj_t *btn_gpio_get_create(struct gpio_desc *desc, lv_obj_t *cont)
{

    /* Create button widget, set up user data, flags, color, style */
    desc->obj = lv_led_create(cont);
    lv_led_set_brightness(desc->obj, 255);
    lv_led_set_color(desc->obj, lv_color_hex(0xff0000));
    lv_led_off(desc->obj);
    lv_obj_add_style(desc->obj, desc->style, LV_PART_MAIN | LV_STATE_DEFAULT);

    return desc->obj;
}

void demo_relay_create(lv_obj_t *tab, lv_coord_t height, lv_coord_t width)
{
    int i;

    lv_obj_t *cont = flex_obj_create(tab, height, width);

    /* Set up button style */
    lv_style_init(&style_relay_btn);
    /* Set up for 2 buttons with 10 px padding on sides, squared */
    width = (width/2) - 20;
    height = width;
    lv_style_set_width(&style_relay_btn, width);
    lv_style_set_height(&style_relay_btn, height);

    /* TODO: Can we add a second label that says asserted/deasserted and is
     * still centered? */

    for (i = 0; ; i++) {
        if (gpio_relay_desc[i].chip_path == NULL)
          break;

        btn_gpio_set_create(&gpio_relay_desc[i], cont);
    }
}

/* Set up objects in a row in the flex field. If it is the first object being
 * set up, then start it on a new row.
 *
 * NOTE: if there are more objects than a single track can handle, then
 * they will populate too the right, offscreen, and cause a scrollbar
 * to show up.
 *
 * This is meant for gpio_set buttons
 */
static void gpio_set_obj_create_loop_flex_break(struct gpio_desc *desc, lv_obj_t *cont)
{
    int i;
    lv_obj_t *newtrack;

    for (i = 0; ; i++) {
        if (desc[i].chip_path == NULL)
            break;

        newtrack = btn_gpio_set_create(&desc[i], cont);
        if (i == 0)
            lv_obj_add_flag(newtrack, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }
}

/* Similar to the above function, but, meant for creating gpio_get, LED icons. */
static void gpio_get_obj_create_loop_flex_break(struct gpio_desc *desc, lv_obj_t *cont)
{
    int i;
    lv_obj_t *newtrack;

    for (i = 0; ; i++) {
        if (desc[i].chip_path == NULL)
            break;

        newtrack = btn_gpio_get_create(&desc[i], cont);
        if (i == 0)
            lv_obj_add_flag(newtrack, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }
}

void demo_gpio_create(lv_obj_t *tab, lv_coord_t height, lv_coord_t width)
{
    lv_obj_t *cont = flex_obj_create(tab, height, width);

    label_create_center(cont, "#ffffff Low-side Switches#");

    /* Make labels for each col */
    lv_obj_t *label_dio1 = label_create_center(cont, "#ffffff DIO_1#");
    lv_obj_add_flag(label_dio1, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    label_create_center(cont, "#ffffff DIO_2#");
    label_create_center(cont, "#ffffff DIO_3#");

    /* Set up button style */
    lv_style_init(&style_gpio_btn);
    /* Set up for 3 buttons with 10 px padding on sides, squared */
    width = (width/3) - 40;
    height = width;
    lv_style_set_width(&style_gpio_btn, width);
    lv_style_set_height(&style_gpio_btn, height);

    gpio_set_obj_create_loop_flex_break(gpio_btn_desc, cont);

    /* Set up LED style */
    lv_style_init(&style_led);
    lv_style_set_width(&style_led, width/4);
    lv_style_set_height(&style_led, height/4);

    gpio_get_obj_create_loop_flex_break(gpio_led_desc, cont);

    /* High-side switch label creation */
    lv_obj_t *label_bot = label_create_center(cont, "#ffffff High-side Switch#");
    lv_obj_add_flag(label_bot, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    gpio_set_obj_create_loop_flex_break(gpio_hs_desc, cont);
}
