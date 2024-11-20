
// #include <lvgl.h>
// #include "display/lv_display.h"
// #include "misc/lv_color.h"
// #include "misc/lv_types.h"
// #include <misc/lv_anim.h>

// static void anim_x_cb(void * var, int32_t v)
// {
//     lv_obj_set_x((lv_obj_t*) var, v);
// }

// static void anim_size_cb(void * var, int32_t v)
// {
//     lv_obj_set_size((lv_obj_t*)var, v, v);
// }

// /**
//  * Create a playback animation
//  */
// void lv_example_anim_2(void)
// {

//     lv_obj_t * obj = lv_obj_create(lv_screen_active());
//     lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_RED), 0);
//     lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);

//     lv_obj_align(obj, LV_ALIGN_LEFT_MID, 10, 0);

//     lv_anim_t a;
//     lv_anim_init(&a);
//     lv_anim_set_var(&a, obj);
//     lv_anim_set_values(&a, 2, 50);
//     lv_anim_set_duration(&a, 5000);
//     lv_anim_set_playback_delay(&a, 100);
//     lv_anim_set_playback_duration(&a, 5000);
//     lv_anim_set_repeat_delay(&a, 100);
//     lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
//     lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

//     lv_anim_set_exec_cb(&a, anim_size_cb);
//     lv_anim_start(&a);
//     lv_anim_set_exec_cb(&a, anim_x_cb);
//     lv_anim_set_values(&a, 2, 75);
//     lv_anim_start(&a);
// }
