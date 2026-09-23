/**
 * @file    lv_port_disp.c
 * @brief   LVGL 显示端口实现：将逻辑横屏画面转置为物理竖屏后经 SPI 写入 ST7789。
 * @note    用户自建代码；物理方向与旧工程保持一致（phys_x=y, phys_y=279-x）。
 */

#include "lv_port_disp.h"

#include "lcd_hw.h"

#define LV_PORT_DISP_HOR_RES   280  /**< LVGL 逻辑宽度（横屏）。 */
#define LV_PORT_DISP_VER_RES   240  /**< LVGL 逻辑高度（横屏）。 */
#define LV_PORT_DISP_BUF_LINES 32   /**< 局部渲染缓冲的行数。 */

#define LV_PORT_PHYS_WIDTH     240  /**< 面板物理宽度，单位像素。 */
#define LV_PORT_PHYS_HEIGHT    280  /**< 面板物理高度，单位像素。 */

/** @brief LVGL 局部绘制缓冲，大小 = 逻辑宽 × 缓冲行数 × 2 字节。 */
static uint8_t disp_buf[LV_PORT_DISP_HOR_RES * LV_PORT_DISP_BUF_LINES * 2]
    __attribute__((aligned(4)));
/** @brief 转置后的物理方向像素缓冲，供硬件层按行写入。 */
static uint16_t transpose_buf[LV_PORT_DISP_HOR_RES * LV_PORT_DISP_BUF_LINES];

/**
 * @brief LVGL 刷新回调：转置脏矩形并写入面板。
 * @param disp 触发刷新的显示对象。
 * @param area 本次刷新的逻辑坐标区域。
 * @param px_map RGB565 像素数据，行优先排列。
 */
static void lv_port_disp_flush(lv_display_t *disp, const lv_area_t *area,
                               uint8_t *px_map)
{
  const uint16_t *src = (const uint16_t *)px_map;
  uint16_t logical_width = (uint16_t)(area->x2 - area->x1 + 1);   /**< 逻辑区域宽。 */
  uint16_t logical_height = (uint16_t)(area->y2 - area->y1 + 1);  /**< 逻辑区域高。 */
  uint16_t phys_width = logical_height;                           /**< 物理区域宽 = 逻辑高。 */
  uint16_t phys_height = logical_width;                           /**< 物理区域高 = 逻辑宽。 */
  uint16_t phys_x = (uint16_t)area->y1;                           /**< 物理左上角 X。 */
  uint16_t phys_y = (uint16_t)((LV_PORT_PHYS_HEIGHT - 1) - area->x2); /**< 物理左上角 Y。 */
  uint16_t row;
  uint16_t col;

  for (row = 0U; row < phys_height; ++row) {
    for (col = 0U; col < phys_width; ++col) {
      transpose_buf[(uint32_t)row * phys_width + col] =
          src[(uint32_t)col * logical_width +
              (uint32_t)(logical_width - 1U - row)];
    }
  }

  (void)LCD_HW_WriteAreaRGB565(phys_x, phys_y, phys_width, phys_height,
                               transpose_buf);
  lv_display_flush_ready(disp);
}

void LvPortDisp_Init(void)
{
  lv_display_t *disp = lv_display_create(LV_PORT_DISP_HOR_RES,
                                         LV_PORT_DISP_VER_RES);

  lv_display_set_flush_cb(disp, lv_port_disp_flush);
  lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
}
