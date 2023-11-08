#include <stdio.h>
#include <iio.h>

#include "lvgl/lvgl.h"
#include "gpiolib1.h"
#include "main.h"
#include "meter.h"

struct gpio_desc {
    const char *chip_path;
    int line;
    void *gpio;
    int oval;
};

static struct gpio_desc gpio_adc_desc[] = {
    { "/dev/gpiochip5", 9, NULL, 0},
    { "/dev/gpiochip5", 10, NULL, 0},
    { "/dev/gpiochip5", 11, NULL, 0},
    { "/dev/gpiochip5", 14, NULL, 0},
    {},
};

static lv_style_t style_legend;
struct lv_adc_meter {
    const char *chan_name;
    struct iio_channel *iio_chan;
    lv_obj_t *meter;
    lv_meter_indicator_t * indic;
    lv_palette_t color;
    const char *legend;
    lv_anim_t anim;
};

static struct lv_adc_meter adc_desc[] = {
    { "voltage5", NULL, NULL, NULL, LV_PALETTE_RED, "ADC 5", {0} },
    { "voltage8", NULL, NULL, NULL, LV_PALETTE_GREEN, "ADC 8", {0} },
    { "voltage9", NULL, NULL, NULL, LV_PALETTE_BLUE, "ADC 9", {0} },
    { "voltage0", NULL, NULL, NULL, LV_PALETTE_ORANGE, "ADC 0", {0} },
    { },
};

static void meter_scale_cb(lv_event_t *e)
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->text != NULL) {
        snprintf(dsc->text, 3, "%d", dsc->value/1000);
    }
}

/* This sets up the GPIO pins that dictate if the ADC are in 0-12 V or 0-20 mA
 * modes.
 */
static bool gpio_is_init;
void gpio_adc_setup(void)
{
    int i;

    if (gpio_is_init) return;

    for (i = 0; ; i++) {
        if (gpio_adc_desc[i].chip_path == NULL) break;
        gpio_adc_desc[i].gpio = gpio_alloc(gpio_adc_desc[i].chip_path,
            gpio_adc_desc[i].line, 1, gpio_adc_desc[i].oval);
    }

    gpio_is_init = true;
}

static void adc_set_value(void * d, int32_t v)
{
    struct lv_adc_meter *desc = d;
    lv_meter_set_indicator_end_value(desc->meter, desc->indic, v);
}

/* Animation causes the arc to behave similar to an analog voltmeter needle.
 * Large changes are traversed fairly quickly, but as the needle reaches its
 * limit, or the changes are small, it is slow to settle.
 */
static void my_timer(lv_timer_t *timer)
{
    struct lv_adc_meter *desc = timer->user_data;
    long long sample;

    iio_channel_attr_read_longlong(desc->iio_chan, "raw", &sample);
    sample = sample * 13325 / 4095;

    lv_anim_del(&desc->anim, adc_set_value);
    lv_anim_init(&desc->anim);
    lv_anim_set_exec_cb(&desc->anim, adc_set_value);
    lv_anim_set_values(&desc->anim, desc->indic->end_value, sample);
    lv_anim_set_time(&desc->anim, 300);
    lv_anim_set_var(&desc->anim, desc);
    lv_anim_start(&desc->anim);
}

/**
 * A meter with multiple arcs
 */
void lv_meter(lv_obj_t *tab, int h, int w)
{
    /* XXX: These probably don't need to be static */
    static struct iio_context *ctx;
    static struct iio_device *dev;
    lv_obj_t * meter;
    int i;

    lv_obj_t *cont = flex_obj_create(tab, h, w);
    /* The meter is fairly large and default margins want to scroll withing
     * the flex container. Make it not scrollable to prevent that.
     */
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Create meter */
    meter = lv_meter_create(cont);
    lv_obj_center(meter);
    lv_obj_set_size(meter, 230, 230);
    /* The meter itself needs to not be scrollable, otherwise, the labels
     * added later can be independently scrolled for some reason.
     */
    lv_obj_clear_flag(meter, LV_OBJ_FLAG_SCROLLABLE);


    /* IIO setup */
    ctx = iio_create_default_context();
    dev = iio_context_find_device(ctx, "2198000.adc");
    for (i = 0; ; i++) {
        if (adc_desc[i].chan_name == NULL) break;
        adc_desc[i].iio_chan = iio_device_find_channel(dev,
            adc_desc[i].chan_name, false);
        adc_desc[i].meter = meter;
    }

    /*Remove the circle from the middle*/
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

    /*Add a scale first*/
    lv_meter_scale_t * scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 13, 2, 1, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 1, 2, 50, lv_color_hex3(0xeee), 10);
    lv_meter_set_scale_range(meter, scale, 0, 12000, 270, 90);

    /* Add text label in middle */
    lv_obj_t *label_obj = lv_label_create(meter);
    lv_label_set_text_static(label_obj, "Volts");
    lv_obj_center(label_obj);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);

    /* Set up a callback to modify the major tick labels to show V, while the
     * the arc indicators are actually using mV.
     */
    lv_obj_add_event_cb(meter, meter_scale_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);

    /* Set up legend text style */
    lv_style_init(&style_legend);
    lv_style_set_text_font(&style_legend, &lv_font_montserrat_10);

    /*Add arc indicators */
    for (i = 0; ; i++) {
        if (adc_desc[i].chan_name == NULL) break;
        adc_desc[i].indic = lv_meter_add_arc(adc_desc[i].meter, scale, 10,
            lv_palette_main(adc_desc[i].color), i * -10);
        lv_timer_create(my_timer, 100, &adc_desc[i]);

        lv_obj_t *label_arc = lv_label_create(meter);
        lv_label_set_text_static(label_arc, adc_desc[i].legend);
        /* The offset of the label location is fixed */
        lv_obj_align(label_arc, LV_ALIGN_CENTER, 20, (i * 10) + 65);
        lv_obj_add_style(label_arc, &style_legend, LV_PART_MAIN);
    }
}

