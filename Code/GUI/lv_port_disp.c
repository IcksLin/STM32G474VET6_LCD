/**
 * @file    lv_port_disp.c
 * @brief   LVGL 显示端口：横屏逻辑画面转置为物理竖屏，并经 LCD 硬件层 DMA 发送。
 * @note    用户自建代码；物理方向与旧工程保持一致（phys_x=y, phys_y=279-x）。
 *          每个 LVGL 脏矩形对应一次完整 DMA 事务，避免分块窗口切换造成错行。
 */

#include "lv_port_disp.h"

#include "lcd_hw.h"

#define LV_PORT_DISP_HOR_RES   280  /**< LVGL 逻辑宽度（横屏）。 */
#define LV_PORT_DISP_VER_RES   240  /**< LVGL 逻辑高度（横屏）。 */
#define LV_PORT_DISP_BUF_LINES 32   /**< LVGL 局部渲染缓冲行数。 */

#define LV_PORT_PHYS_WIDTH     240  /**< 面板物理宽度，单位像素。 */
#define LV_PORT_PHYS_HEIGHT    280  /**< 面板物理高度，单位像素。 */
#define LV_PORT_TRANSPOSE_PIXELS \
  (LV_PORT_DISP_HOR_RES * LV_PORT_DISP_BUF_LINES) /**< 单次 LVGL 刷新的最大像素数。 */

/** @brief 当前 LVGL 显示对象，供 DMA 完成回调通知刷新结束。 */
static lv_display_t *s_disp;
/** @brief LVGL 局部绘制缓冲，大小 = 逻辑宽 × 缓冲行数 × 2 字节。 */
static uint8_t disp_buf[LV_PORT_DISP_HOR_RES * LV_PORT_DISP_BUF_LINES * 2]
    __attribute__((aligned(4)));
/**
 * @brief 脏矩形转置缓冲，容量与 LVGL 局部绘制缓冲的像素容量相同。
 * @note LVGL 可能提交窄而高的区域，因此缓存按总像素容量分配，不能仅按
 *       “区域高度不超过缓冲行数”这一假设分配。
 */
static uint16_t transpose_buf[LV_PORT_TRANSPOSE_PIXELS]
    __attribute__((aligned(4)));

/** @brief 非零表示 LVGL 当前存在一笔尚未确认完成的刷新。 */
static volatile uint8_t s_flush_active;

/**
 * @brief 交换 16 位像素的两个字节，得到面板需要的高字节在前顺序。
 * @param value 原生小端 RGB565 值。
 * @return 字节序交换后的值。
 */
static uint16_t lv_port_swap16(uint16_t value)
{
  return (uint16_t)((uint16_t)(value << 8) | (uint16_t)(value >> 8));
}

/**
 * @brief LVGL 刷新回调：记录区域参数并启动第一块转置发送。
 * @param disp 触发刷新的显示对象。
 * @param area 本次刷新的逻辑坐标区域。
 * @param px_map RGB565 像素数据，行优先排列。
 * @note 成功后不在此处调用 flush_ready，而由最后一块 DMA 完成中断触发。
 */
static void lv_port_disp_flush(lv_display_t *disp, const lv_area_t *area,
                               uint8_t *px_map)
{
  const uint16_t *src;
  int32_t logical_width;
  int32_t logical_height;
  uint16_t phys_width;
  uint16_t phys_height;
  uint16_t phys_x;
  uint16_t phys_y;
  uint16_t row;
  uint16_t col;
  uint32_t pixel_count;

  if (disp == NULL || area == NULL || px_map == NULL ||
      area->x1 < 0 || area->y1 < 0 ||
      area->x2 >= LV_PORT_DISP_HOR_RES ||
      area->y2 >= LV_PORT_DISP_VER_RES ||
      area->x2 < area->x1 || area->y2 < area->y1 ||
      s_flush_active != 0U) {
    if (disp != NULL) lv_display_flush_ready(disp);
    return;
  }

  logical_width = area->x2 - area->x1 + 1;
  logical_height = area->y2 - area->y1 + 1;
  pixel_count = (uint32_t)logical_width * (uint32_t)logical_height;
  if (pixel_count == 0U || pixel_count > LV_PORT_TRANSPOSE_PIXELS) {
    lv_display_flush_ready(disp);
    return;
  }

  src = (const uint16_t *)px_map;
  phys_width = (uint16_t)logical_height;
  phys_height = (uint16_t)logical_width;
  phys_x = (uint16_t)area->y1;
  phys_y = (uint16_t)((LV_PORT_PHYS_HEIGHT - 1) - area->x2);

  for (row = 0U; row < phys_height; ++row) {
    for (col = 0U; col < phys_width; ++col) {
      transpose_buf[(uint32_t)row * phys_width + col] =
          lv_port_swap16(src[(uint32_t)col * (uint16_t)logical_width +
                             ((uint16_t)logical_width - 1U - row)]);
    }
  }

  s_flush_active = 1U;
  if (LCD_HW_WriteAreaRGB565(phys_x, phys_y, phys_width, phys_height,
                             transpose_buf) != 0) {
    s_flush_active = 0U;
    lv_display_flush_ready(disp);
  }
}

void LvPortDisp_FlushComplete(void)
{
  if (s_flush_active != 0U) {
    s_flush_active = 0U;
    lv_display_flush_ready(s_disp);
  }
}

void LvPortDisp_Init(void)
{
  lv_display_t *disp = lv_display_create(LV_PORT_DISP_HOR_RES,
                                         LV_PORT_DISP_VER_RES);
  s_disp = disp;

  lv_display_set_flush_cb(disp, lv_port_disp_flush);
  lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  LCD_HW_SetTxCompleteCallback(LvPortDisp_FlushComplete);
}
