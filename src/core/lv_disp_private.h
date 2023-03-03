/**
 * @file lv_disp_private.h
 *
 */

#ifndef LV_DISP_PRIVATE_H
#define LV_DISP_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_obj.h"
#include "../draw/lv_draw.h"

/*********************
 *      DEFINES
 *********************/
#ifndef LV_INV_BUF_SIZE
#define LV_INV_BUF_SIZE 32 /*Buffer size for invalid areas*/
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_disp_t {

    /*---------------------
     * Resolution
     *--------------------*/

    /** Horizontal resolution.*/
    lv_coord_t hor_res;

    /** Vertical resolution.*/
    lv_coord_t ver_res;

    /** Horizontal resolution of the full / physical display. Set to -1 for fullscreen mode.*/
    lv_coord_t physical_hor_res;

    /** Vertical resolution of the full / physical display. Set to -1 for fullscreen mode.*/
    lv_coord_t physical_ver_res;

    /** Horizontal offset from the full / physical display. Set to 0 for fullscreen mode.*/
    lv_coord_t offset_x;

    /** Vertical offset from the full / physical display. Set to 0 for fullscreen mode.*/
    lv_coord_t offset_y;

    uint32_t dpi;              /** DPI (dot per inch) of the display. Default value is `LV_DPI_DEF`.*/

    /*---------------------
     * Buffering
     *--------------------*/

    /** First display buffer.*/
    void * draw_buf_1;

    /** Second display buffer.*/
    void * draw_buf_2;

    /** Internal, used by the library*/
    void * draw_buf_act;

    /** In pixel count*/
    uint32_t draw_buf_size;

    /** MANDATORY: Write the internal buffer (draw_buf) to the display. 'lv_disp_flush_ready()' has to be
     * called when finished*/
    void (*flush_cb)(struct _lv_disp_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

    /*1: flushing is in progress. (It can't be a bit field because when it's cleared from IRQ Read-Modify-Write issue might occur)*/
    volatile int flushing;

    /*1: It was the last chunk to flush. (It can't be a bit field because when it's cleared from IRQ Read-Modify-Write issue might occur)*/
    volatile int flushing_last;
    volatile uint32_t last_area         : 1; /*1: the last area is being rendered*/
    volatile uint32_t last_part         : 1; /*1: the last part of the current area is being rendered*/

    lv_disp_render_mode_t render_mode;
    uint32_t antialiasing : 1;       /**< 1: anti-aliasing is enabled on this display.*/

    /** 1: The current screen rendering is in progress*/
    uint32_t rendering_in_progress : 1;

    lv_color_format_t   color_format;

    /** Invalidated (marked to redraw) areas*/
    lv_area_t inv_areas[LV_INV_BUF_SIZE];
    uint8_t inv_area_joined[LV_INV_BUF_SIZE];
    uint16_t inv_p;
    int32_t inv_en_cnt;

    /*---------------------
     * Draw context
     *--------------------*/

    lv_draw_ctx_t * draw_ctx;
    void (*draw_ctx_init)(struct _lv_disp_t * disp, lv_draw_ctx_t * draw_ctx);
    void (*draw_ctx_deinit)(struct _lv_disp_t * disp, lv_draw_ctx_t * draw_ctx);
    size_t draw_ctx_size;

    /*---------------------
     * Screens
     *--------------------*/

    /** Screens of the display*/
    struct _lv_obj_t ** screens;    /**< Array of screen objects.*/
    struct _lv_obj_t * act_scr;     /**< Currently active screen on this display*/
    struct _lv_obj_t * prev_scr;    /**< Previous screen. Used during screen animations*/
    struct _lv_obj_t * scr_to_load; /**< The screen prepared to load in lv_scr_load_anim*/
    struct _lv_obj_t * bottom_layer;    /**< @see lv_disp_get_layer_bottom*/
    struct _lv_obj_t * top_layer;       /**< @see lv_disp_get_layer_top*/
    struct _lv_obj_t * sys_layer;       /**< @see lv_disp_get_layer_sys*/
    uint32_t screen_cnt;
    uint8_t draw_prev_over_act  : 1;/** 1: Draw previous screen over active screen*/
    uint8_t del_prev  : 1; /** 1: Automatically delete the previous screen when the screen load animation is ready*/

    /*---------------------
     * Others
     *--------------------*/

    void * driver_data; /**< Custom user data*/

#if LV_USE_USER_DATA
    void * user_data; /**< Custom user data*/
#endif

    lv_event_list_t event_list;

    uint32_t sw_rotate : 1; /**< 1: use software rotation (slower)*/
    uint32_t rotation  : 2; /**< Element of  @lv_disp_rotation_t*/

    /**< The theme assigned to the screen*/
    struct _lv_theme_t * theme;

    /** A timer which periodically checks the dirty areas and refreshes them*/
    lv_timer_t * refr_timer;

    /*Miscellaneous data*/
    uint32_t last_activity_time;        /**< Last time when there was activity on this display*/

    uint32_t last_render_start_time;

    /** OPTIONAL: Called periodically while lvgl waits for operation to be completed.
     * For example flushing or GPU
     * User can execute very simple tasks here or yield the task*/
    void (*wait_cb)(struct _lv_disp_t * disp_drv);

    /** On CHROMA_KEYED images this color will be transparent.
     * `LV_COLOR_CHROMA_KEY` by default. (lv_conf.h) */
    lv_color_t color_chroma_key;
} lv_disp_t;


/**
 * Display Driver structure to be registered by HAL.
 * Only its pointer will be saved in `lv_disp_t` so it should be declared as
 * `static lv_disp_drv_t my_drv` or allocated dynamically.
 */
#ifdef NOTIN
typedef struct _lv_disp_drv_t {

    lv_coord_t hor_res;         /**< Horizontal resolution.*/
    lv_coord_t ver_res;         /**< Vertical resolution.*/

    lv_coord_t
    physical_hor_res;     /**< Horizontal resolution of the full / physical display. Set to -1 for fullscreen mode.*/
    lv_coord_t
    physical_ver_res;     /**< Vertical resolution of the full / physical display. Set to -1 for fullscreen mode.*/
    lv_coord_t
    offset_x;             /**< Horizontal offset from the full / physical display. Set to 0 for fullscreen mode.*/
    lv_coord_t offset_y;             /**< Vertical offset from the full / physical display. Set to 0 for fullscreen mode.*/

    /** Pointer to a buffer initialized with `lv_disp_draw_buf_init()`.
     * LVGL will use this buffer(s) to draw the screens contents*/
    lv_disp_draw_buf_t * draw_buf;

    uint32_t direct_mode : 1;        /**< 1: Use screen-sized buffers and draw to absolute coordinates*/
    uint32_t full_refresh : 1;       /**< 1: Always make the whole screen redrawn*/
    uint32_t sw_rotate : 1;          /**< 1: use software rotation (slower)*/
    uint32_t antialiasing : 1;       /**< 1: anti-aliasing is enabled on this display.*/
    uint32_t rotated : 2;            /**< 1: turn the display by 90 degree. @warning Does not update coordinates for you!*/
    uint32_t screen_transp : 1;      /**Handle if the screen doesn't have a solid (opa == LV_OPA_COVER) background.
                                       * Use only if required because it's slower.*/

    uint32_t dpi : 10;              /** DPI (dot per inch) of the display. Default value is `LV_DPI_DEF`.*/

    /** MANDATORY: Write the internal buffer (draw_buf) to the display. 'lv_disp_flush_ready()' has to be
     * called when finished*/
    void (*flush_cb)(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

    /** OPTIONAL: Extend the invalidated areas to match with the display drivers requirements
     * E.g. round `y` to, 8, 16 ..) on a monochrome display*/
    void (*rounder_cb)(struct _lv_disp_drv_t * disp_drv, lv_area_t * area);

    /** OPTIONAL: Set a pixel in a buffer according to the special requirements of the display
     * Can be used for color format not supported in LittelvGL. E.g. 2 bit -> 4 gray scales
     * @note Much slower then drawing with supported color formats.*/
    void (*set_px_cb)(struct _lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
                      lv_color_t color, lv_opa_t opa);

    void (*clear_cb)(struct _lv_disp_drv_t * disp_drv, uint8_t * buf, uint32_t size);


    /** OPTIONAL: Called after every refresh cycle to tell the rendering and flushing time + the
     * number of flushed pixels*/
    void (*monitor_cb)(struct _lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px);

    /** OPTIONAL: Called periodically while lvgl waits for operation to be completed.
     * For example flushing or GPU
     * User can execute very simple tasks here or yield the task*/
    void (*wait_cb)(struct _lv_disp_drv_t * disp_drv);

    /** OPTIONAL: Called when lvgl needs any CPU cache that affects rendering to be cleaned*/
    void (*clean_dcache_cb)(struct _lv_disp_drv_t * disp_drv);

    /** OPTIONAL: called when driver parameters are updated */
    void (*drv_update_cb)(struct _lv_disp_drv_t * disp_drv);

    /** OPTIONAL: called when start rendering */
    void (*render_start_cb)(struct _lv_disp_drv_t * disp_drv);

    /** On CHROMA_KEYED images this color will be transparent.
     * `LV_COLOR_CHROMA_KEY` by default. (lv_conf.h)*/
    lv_color_t color_chroma_key;

    lv_draw_ctx_t * draw_ctx;
    void (*draw_ctx_init)(struct _lv_disp_drv_t * disp_drv, lv_draw_ctx_t * draw_ctx);
    void (*draw_ctx_deinit)(struct _lv_disp_drv_t * disp_drv, lv_draw_ctx_t * draw_ctx);
    size_t draw_ctx_size;

#if LV_USE_USER_DATA
    void * user_data; /**< Custom display driver user data*/
#endif

} lv_disp_drv_t;
#endif

typedef struct _lv_disp_draw_buf_t {
    void * buf1; /**< First display buffer.*/
    void * buf2; /**< Second display buffer.*/

    /*Internal, used by the library*/
    void * buf_act;
    uint32_t size; /*In pixel count*/
    /*1: flushing is in progress. (It can't be a bit field because when it's cleared from IRQ Read-Modify-Write issue might occur)*/
    volatile int flushing;
    /*1: It was the last chunk to flush. (It can't be a bit field because when it's cleared from IRQ Read-Modify-Write issue might occur)*/
    volatile int flushing_last;
    volatile uint32_t last_area         : 1; /*1: the last area is being rendered*/
    volatile uint32_t last_part         : 1; /*1: the last part of the current area is being rendered*/
} lv_disp_draw_buf_t;



/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_disp_drv_init(lv_disp_drv_t * driver);
void lv_disp_draw_buf_init(lv_disp_draw_buf_t * draw_buf, void * buf1, void * buf2, uint32_t size_in_px_cnt);
lv_disp_t * lv_disp_drv_register(lv_disp_drv_t * driver);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DISP_PRIVATE_H*/
