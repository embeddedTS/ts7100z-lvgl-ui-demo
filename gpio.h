#ifndef __GPIO_H__
#define __GPIO_H__

void gpio_claim_all_and_set_cb(void);
void demo_relay_create(lv_obj_t *tab, lv_coord_t height, lv_coord_t width);
void demo_gpio_create(lv_obj_t *tab, lv_coord_t height, lv_coord_t width);

#endif // __GPIO_H__
