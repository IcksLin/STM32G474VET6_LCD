/**
 * @file    app_lvgl.c
 * @brief   基于 LVGL 原生控件的菜单演示界面，含浮动高亮块平滑跟随焦点。
 * @note    用户自建代码；调色板沿用旧工程的茶褐色方案（RGB565 值经转换后使用）。
 */

#include "app_lvgl.h"

#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "stm32g4xx_hal.h"

#include <stdio.h>

/** @brief 页面背景色（RGB565，浅茶褐）。 */
#define APP_COLOR_BACKGROUND   ((uint16_t)0xBCCF)
/** @brief 未选中菜单卡片背景色（RGB565，灰棕）。 */
#define APP_COLOR_ITEM         ((uint16_t)0x9C0C)
/** @brief 菜单文字统一颜色（RGB565，奶油白，与高亮块配色一致）。 */
#define APP_COLOR_TEXT         ((uint16_t)0xFF9A)
/** @brief 选中高亮块背景色（RGB565，暖深棕）。 */
#define APP_COLOR_SELECTED     ((uint16_t)0x7A66)
/** @brief 滚动条颜色（RGB565，暖中棕）。 */
#define APP_COLOR_SCROLLBAR    ((uint16_t)0x5A45)

#define APP_ITEM_COUNT         6    /**< 菜单条目数量。 */
#define APP_ITEM_HEIGHT        34   /**< 单个菜单条目高度，单位像素。 */
#define APP_ITEM_GAP           4    /**< 相邻条目间距，单位像素。 */
#define APP_PAD_TOP            6    /**< 容器顶部内边距，单位像素。 */
#define APP_PAD_LEFT           6    /**< 容器左侧内边距，单位像素。 */
#define APP_PAD_RIGHT          14   /**< 容器右侧内边距，为滚动条预留空间。 */
#define APP_CONTAINER_WIDTH    240  /**< 滚动容器宽度，单位像素。 */
#define APP_CONTAINER_HEIGHT   190  /**< 滚动容器高度，单位像素。 */
/** @brief 菜单条目宽度（扣除左右内边距后）。 */
#define APP_ITEM_WIDTH         (APP_CONTAINER_WIDTH - APP_PAD_LEFT - APP_PAD_RIGHT)
#define APP_TEXT_INSET         10   /**< 文字相对条目左边界的缩进。 */
#define APP_SCROLLBAR_WIDTH    6    /**< 滚动条宽度，单位像素。 */
#define APP_SCROLLBAR_PAD_RIGHT 2   /**< 滚动条右侧留白，越小越靠右。 */
#define APP_ANIM_MS            200  /**< 高亮块滑动动画时长，单位毫秒。 */

/** @brief 未选中条目卡片样式（背景、圆角、取消主题边框）。 */
static lv_style_t card_style;
/** @brief 用于抵消主题在焦点/按下状态附加的描边与形变。 */
static lv_style_t cancel_style;
/** @brief 浮动高亮块对象，在焦点条目之间平滑滑动。 */
static lv_obj_t *highlight;
/** @brief 首个菜单条目，用于开机定位焦点。 */
static lv_obj_t *first_button;

/**
 * @brief 将 RGB565 数值展开为 LVGL 使用的 RGB888 颜色。
 * @param rgb565 16 位 RGB565 颜色值。
 * @return 对应的 lv_color_t 颜色。
 */
static lv_color_t app_color(uint16_t rgb565)
{
  uint8_t r = (uint8_t)((rgb565 >> 11) & 0x1FU);   /**< 5 位红色分量。 */
  uint8_t g = (uint8_t)((rgb565 >> 5) & 0x3FU);    /**< 6 位绿色分量。 */
  uint8_t b = (uint8_t)(rgb565 & 0x1FU);           /**< 5 位蓝色分量。 */

  return lv_color_make((uint8_t)((r << 3) | (r >> 2)),
                       (uint8_t)((g << 2) | (g >> 4)),
                       (uint8_t)((b << 3) | (b >> 2)));
}

/**
 * @brief 初始化本界面使用的三个局部样式。
 */
static void app_init_styles(void)
{
  lv_style_init(&card_style);
  lv_style_set_bg_opa(&card_style, LV_OPA_COVER);
  lv_style_set_bg_color(&card_style, app_color(APP_COLOR_ITEM));
  lv_style_set_radius(&card_style, 10);
  lv_style_set_border_width(&card_style, 0);
  lv_style_set_shadow_width(&card_style, 0);
  lv_style_set_outline_width(&card_style, 0);
  lv_style_set_transform_width(&card_style, 0);
  lv_style_set_transform_height(&card_style, 0);

  lv_style_init(&cancel_style);
  lv_style_set_border_width(&cancel_style, 0);
  lv_style_set_shadow_width(&cancel_style, 0);
  lv_style_set_outline_width(&cancel_style, 0);
  lv_style_set_outline_pad(&cancel_style, 0);
  lv_style_set_outline_opa(&cancel_style, LV_OPA_TRANSP);
  lv_style_set_transform_width(&cancel_style, 0);
  lv_style_set_transform_height(&cancel_style, 0);
}

/**
 * @brief 以动画方式把高亮块移动到目标位置和尺寸。
 * @param x 目标 X 坐标（相对容器内容区）。
 * @param y 目标 Y 坐标（相对容器内容区）。
 * @param width 目标宽度。
 * @param height 目标高度。
 */
static void app_anim_highlight(int32_t x, int32_t y, int32_t width,
                               int32_t height)
{
  lv_anim_t anim;   /**< 复用的动画描述结构。 */

  lv_anim_delete(highlight, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_delete(highlight, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_delete(highlight, (lv_anim_exec_xcb_t)lv_obj_set_width);
  lv_anim_delete(highlight, (lv_anim_exec_xcb_t)lv_obj_set_height);

  lv_anim_init(&anim);
  lv_anim_set_var(&anim, highlight);
  lv_anim_set_duration(&anim, APP_ANIM_MS);
  lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);

  lv_anim_set_values(&anim, lv_obj_get_x(highlight), x);
  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_start(&anim);

  lv_anim_set_values(&anim, lv_obj_get_y(highlight), y);
  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_start(&anim);

  lv_anim_set_values(&anim, lv_obj_get_width(highlight), width);
  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_width);
  lv_anim_start(&anim);

  lv_anim_set_values(&anim, lv_obj_get_height(highlight), height);
  lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_height);
  lv_anim_start(&anim);
}

/**
 * @brief 条目焦点事件回调：仅滑动高亮块并滚动到可见区，不改变文字颜色。
 * @param event LVGL 事件对象。
 */
static void app_item_event_cb(lv_event_t *event)
{
  lv_obj_t *button = lv_event_get_target(event);   /**< 触发事件的条目按钮。 */

  app_anim_highlight(lv_obj_get_x(button), lv_obj_get_y(button),
                     lv_obj_get_width(button), lv_obj_get_height(button));
  lv_obj_scroll_to_view(button, LV_ANIM_ON);
}

void App_Lvgl_Init(void)
{
  lv_obj_t *screen;      /**< 根屏幕对象。 */
  lv_obj_t *container;   /**< 承载菜单条目与高亮块的滚动容器。 */
  lv_group_t *group;     /**< keypad 输入设备的焦点组。 */
  uint32_t i;            /**< 条目循环下标。 */

  lv_init();
  lv_tick_set_cb(HAL_GetTick);

  LvPortDisp_Init();
  LvPortIndev_Init();
  app_init_styles();

  group = LvPortIndev_GetGroup();

  screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(screen, app_color(APP_COLOR_BACKGROUND),
                            LV_PART_MAIN);
  lv_screen_load(screen);

  container = lv_obj_create(screen);
  lv_obj_set_size(container, APP_CONTAINER_WIDTH, APP_CONTAINER_HEIGHT);
  lv_obj_center(container);
  lv_obj_set_scroll_dir(container, LV_DIR_VER);
  lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(container, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(container, app_color(APP_COLOR_BACKGROUND),
                            LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(container, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(container, 10, LV_PART_MAIN);

  lv_obj_set_style_width(container, APP_SCROLLBAR_WIDTH, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(container, APP_SCROLLBAR_WIDTH / 2,
                          LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_right(container, APP_SCROLLBAR_PAD_RIGHT,
                             LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(container, LV_OPA_COVER, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(container, app_color(APP_COLOR_SCROLLBAR),
                            LV_PART_SCROLLBAR);

  for (i = 0U; i < APP_ITEM_COUNT; ++i) {
    lv_obj_t *button = lv_button_create(container);
    int32_t y = APP_PAD_TOP + (int32_t)i * (APP_ITEM_HEIGHT + APP_ITEM_GAP); /**< 条目纵向位置。 */

    lv_obj_set_pos(button, APP_PAD_LEFT, y);
    lv_obj_set_size(button, APP_ITEM_WIDTH, APP_ITEM_HEIGHT);
    lv_obj_add_style(button, &card_style, LV_PART_MAIN);
    lv_obj_add_style(button, &cancel_style, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(button, &cancel_style, LV_STATE_FOCUSED);
    lv_obj_add_style(button, &cancel_style, LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, app_item_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_group_add_obj(group, button);
    if (i == 0U) {
      first_button = button;
    }
  }

  highlight = lv_obj_create(container);
  lv_obj_set_pos(highlight, APP_PAD_LEFT, APP_PAD_TOP);
  lv_obj_set_size(highlight, APP_ITEM_WIDTH, APP_ITEM_HEIGHT);
  lv_obj_set_style_bg_opa(highlight, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(highlight, app_color(APP_COLOR_SELECTED),
                            LV_PART_MAIN);
  lv_obj_set_style_radius(highlight, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(highlight, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(highlight, 0, LV_PART_MAIN);
  lv_obj_remove_flag(highlight, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(highlight, LV_OBJ_FLAG_SCROLLABLE);

  for (i = 0U; i < APP_ITEM_COUNT; ++i) {
    char text[16];                                                          /**< 条目文字缓冲。 */
    lv_obj_t *label;                                                        /**< 新建的文字对象。 */
    int32_t y = APP_PAD_TOP + (int32_t)i * (APP_ITEM_HEIGHT + APP_ITEM_GAP); /**< 条目纵向位置。 */

    (void)snprintf(text, sizeof(text), "Item %u", (unsigned)(i + 1U));
    label = lv_label_create(container);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, APP_PAD_LEFT + APP_TEXT_INSET, y + 9);
    lv_obj_set_style_text_color(label, app_color(APP_COLOR_TEXT), 0);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_group_focus_obj(first_button);
}

void App_Lvgl_Process(void)
{
  lv_timer_handler();
}
