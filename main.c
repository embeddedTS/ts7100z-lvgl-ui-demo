#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/libinput_drv.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "gpio.h"
#include "meter.h"

#define DISP_BUF_SIZE (128 * 1024)

/* Width of tab on right side of screen */
#define TAB_W 50

LV_IMG_DECLARE(ts7100z_label_20220324);

/* The following two callbacks work in tandem with the touch_timer variable.
 * At creation of the tabview, we set up touch_timer so that out of the gate
 * there is a touch timeout/idle counter. This way the tabview tabs show on
 * the screen before disappearing.
 *
 * The timer continually runs, rather often, during this time in order to catch
 * touchscreen input as soon as possible. This is so if the user immediately
 * scrolls, the tabs show up quickly and the tab windows render correctly during
 * scroll.
 *
 * If/when the pinout tab is no longer selected, the timer is deleted and the
 * tabview is forced to be always on. If the pinout tab is ever selected again,
 * the timer is restarted and the interaction continues.
 */

lv_timer_t *touch_timer;
lv_style_t style_splash_label;

static void timer_tab_fadeout_cb(lv_timer_t *timer)
{
    lv_obj_t *tv = timer->user_data;


    if (lv_disp_get_inactive_time(NULL) > 1500) {
        lv_obj_add_flag(lv_tabview_get_tab_btns(tv), LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(lv_tabview_get_tab_btns(tv), LV_OBJ_FLAG_HIDDEN);
    }
}

static void tab_change_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_current_target(e);

    if (lv_tabview_get_tab_act(tv) == 0) {
        if (touch_timer == NULL) {
            touch_timer = lv_timer_create(timer_tab_fadeout_cb, 10, tv);
        }
    } else {
        if (touch_timer != NULL) {
            lv_timer_del(touch_timer);
            touch_timer = NULL;
        }
        lv_obj_clear_flag(lv_tabview_get_tab_btns(tv), LV_OBJ_FLAG_HIDDEN);

        /* Lazy initialization of GPIO. Wait until the first time we're off
         * the first screen before grabbing all of the GPIO. This allows the
         * application to be run, but all of the GPIO still usable by the rest
         * of the system until the tab is transitioned.
         *
         * gpio_claim_all_and_set_cb() is safe to run multiple times, however,
         * it must be run after all of the button objects are set up otherwise
         * it will consider the GPIO initialized but will have not actually
         * claimed any of the GPIO.
         */
        gpio_claim_all_and_set_cb();
        gpio_adc_setup();
    }
}

lv_obj_t *flex_obj_create(lv_obj_t *cont, int h, int w)
{
    /* Place buttons on a background, spaced as evenly as possible */
    lv_obj_t * flex = lv_obj_create(cont);
    lv_obj_set_size(flex, w, h);
    lv_obj_center(flex);
    lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(flex, LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(flex, lv_color_hex(0x4a5fa5), LV_PART_MAIN);

    return flex;
}

static void lv_tab_test_setup(void)
{
    lv_coord_t width = LV_HOR_RES-TAB_W;
    lv_coord_t height = LV_VER_RES;
    lv_obj_t * tv;
    lv_obj_t * tab;

    lv_theme_default_init(NULL, lv_color_black(),
      lv_color_hex(0xf77f00), LV_THEME_DEFAULT_DARK,
      &lv_font_montserrat_14);

    tv = lv_tabview_create(lv_scr_act(), LV_DIR_RIGHT, TAB_W);
    lv_obj_add_event_cb(tv, tab_change_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Set tabview background color of the main views */
    lv_obj_set_style_bg_color(tv, lv_color_black(), LV_PART_MAIN);

    /* Set up Pinout tab, sets the splashscreen image as the tab contents,
     * and then aligns this image based on the screen. This is to center the
     * image actually on the screen as each object that acts as a canvas adds
     * some padding. If we align the image on the drawing area of the tabview
     * itself, then it has an offset relative to the screen itself and is not
     * fully rendered.
     */
    tab = lv_tabview_add_tab(tv, "Pinout");
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *image = lv_img_create(tab);
    lv_obj_align_to(image, lv_scr_act(), LV_ALIGN_TOP_LEFT, 0, 19);
    lv_img_set_src(image, &ts7100z_label_20220324);
    touch_timer = lv_timer_create(timer_tab_fadeout_cb, 10, tv);
    lv_obj_t *label_splash = lv_label_create(image);
    lv_style_init(&style_splash_label);
    lv_style_set_text_font(&style_splash_label, &lv_font_montserrat_20);
    lv_label_set_recolor(label_splash, true);
    lv_obj_add_style(label_splash, &style_splash_label, LV_PART_MAIN);
    lv_label_set_text_static(label_splash, "#ffffff Touch anywhere#");
    lv_obj_set_pos(label_splash, 6, 50);

    /* Create a tab and populate contents with the Relay buttons */
    tab = lv_tabview_add_tab(tv, "Relays");
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    demo_relay_create(tab, height, width);

    /* Create a tab and populate contents with the GPIO buttons and LEDs */
    tab = lv_tabview_add_tab(tv, "HV IO");
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    demo_gpio_create(tab, height, width);

    /* Create a tab and populate it with a meter meant to show ADC values */
    tab = lv_tabview_add_tab(tv, "ADC");
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_meter(tab, height, width);
}

int main(void)
{
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 240;
    disp_drv.ver_res    = 320;
    disp_drv.sw_rotate  = 1;
    disp_drv.rotated    = LV_DISP_ROT_270;
    lv_disp_drv_register(&disp_drv);

    libinput_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = libinput_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

    /*Create a Demo*/
    lv_tab_test_setup();

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
