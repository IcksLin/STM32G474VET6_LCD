#ifndef LCD_USER_H
#define LCD_USER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lcd_fonts.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_FB_WIDTH   240U
#define LCD_FB_HEIGHT  280U
#define LCD_RGB565(r, g, b) \
  ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | \
              (((uint16_t)(g) & 0xFCU) << 3) | ((uint16_t)(b) >> 3)))

typedef struct {
  uint32_t update_count;
  uint32_t pixels_flushed;
  uint32_t palette_overflows;
  uint16_t palette_size;
  uint16_t dirty_tile_count;
} LCD_Stats;

/**
 * @brief LCD logical drawing direction.
 */
typedef enum {
  LCD_DIRECTION_PORTRAIT = 0,
  LCD_DIRECTION_LANDSCAPE = 1
} LCD_Direction;

/**
 * @brief 初始化 ST7789、调色板和帧缓存，并完成首次全屏刷新
 */
void LCD_UserInit(LCD_Direction direction);

/**
 * @brief Return the logical drawing width for the selected direction.
 * @return 240 in portrait mode or 280 in landscape mode.
 */
uint16_t LCD_GetWidth(void);

/**
 * @brief Return the logical drawing height for the selected direction.
 * @return 280 in portrait mode or 240 in landscape mode.
 */
uint16_t LCD_GetHeight(void);

/**
 * @brief Return the direction selected during LCD_UserInit.
 * @return Current logical drawing direction.
 */
LCD_Direction LCD_GetDirection(void);

/**
 * @brief 将帧缓存中的全部脏区统一刷新到 LCD
 * @return 0 全部刷新成功，-1 表示至少一个区域写入失败
 */
int LCD_Update(void);

/**
 * @brief 查询当前帧缓存是否存在待刷新的脏区
 * @return true 存在脏区，false 不存在脏区
 */
bool LCD_IsDirty(void);

/**
 * @brief 将整个帧缓存标记为待刷新
 */
void LCD_InvalidateAll(void);

/**
 * @brief 获取 LCD 用户层运行统计信息
 * @return 指向只读统计结构的指针
 */
const LCD_Stats *LCD_GetStats(void);

/**
 * @brief 设置画笔颜色
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_SetPenColor(uint16_t rgb565);

/**
 * @brief 设置字符背景颜色
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_SetBackgroundColor(uint16_t rgb565);

/**
 * @brief 获取当前画笔颜色
 * @return RGB565 格式画笔颜色
 */
uint16_t LCD_FB_GetPenColor(void);

/**
 * @brief 获取当前背景颜色
 * @return RGB565 格式背景颜色
 */
uint16_t LCD_FB_GetBackgroundColor(void);

/**
 * @brief 设置 ASCII 字体
 * @param font 字体描述结构，NULL 表示保持当前字体
 */
void LCD_FB_SetFont(const pFONT *font);

/**
 * @brief 清空帧缓存
 * @param rgb565 RGB565 格式填充颜色
 */
void LCD_FB_Clear(uint16_t rgb565);

/**
 * @brief 绘制像素
 * @param x 横坐标
 * @param y 纵坐标
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_DrawPixel(int16_t x, int16_t y, uint16_t rgb565);

/**
 * @brief 绘制线段
 * @param x0 起点横坐标
 * @param y0 起点纵坐标
 * @param x1 终点横坐标
 * @param y1 终点纵坐标
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t rgb565);

/**
 * @brief 绘制矩形边框
 * @param x 左上角横坐标
 * @param y 左上角纵坐标
 * @param width 宽度
 * @param height 高度
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_DrawRect(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t rgb565);

/**
 * @brief 绘制实心矩形
 * @param x 左上角横坐标
 * @param y 左上角纵坐标
 * @param width 宽度
 * @param height 高度
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_FillRect(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t rgb565);

/**
 * @brief 绘制圆形边框
 * @param x0 圆心横坐标
 * @param y0 圆心纵坐标
 * @param radius 半径
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_DrawCircle(int16_t x0, int16_t y0, uint16_t radius, uint16_t rgb565);

/**
 * @brief 绘制实心圆
 * @param x0 圆心横坐标
 * @param y0 圆心纵坐标
 * @param radius 半径
 * @param rgb565 RGB565 格式颜色
 */
void LCD_FB_FillCircle(int16_t x0, int16_t y0, uint16_t radius, uint16_t rgb565);

/**
 * @brief 绘制字符
 * @param x 左上角横坐标
 * @param y 左上角纵坐标
 * @param ch ASCII 字符
 */
void LCD_FB_DrawChar(int16_t x, int16_t y, char ch);

/**
 * @brief 绘制字符串
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param text 以空字符结尾的字符串
 */
void LCD_FB_DrawString(int16_t x, int16_t y, const char *text);

/**
 * @brief 绘制 RGB565 图像
 * @param x 左上角横坐标
 * @param y 左上角纵坐标
 * @param width 图像宽度
 * @param height 图像高度
 * @param pixels 按行连续排列的 RGB565 像素数据
 */
void LCD_FB_DrawRGB565(int16_t x, int16_t y, uint16_t width, uint16_t height,
                       const uint16_t *pixels);
                       
/**
 * @brief 绘制 8 位灰度图像
 * @param x 左上角横坐标
 * @param y 左上角纵坐标
 * @param width 图像宽度
 * @param height 图像高度
 * @param pixels 按行连续排列的灰度像素数据
 */
void LCD_FB_DrawGray8(int16_t x, int16_t y, uint16_t width, uint16_t height,
                      const uint8_t *pixels);

/**
 * @brief 按 printf 语法格式化文本并写入帧缓存
 * @param x 文本起始横坐标
 * @param y 文本起始纵坐标
 * @param format printf 风格格式字符串
 * @param ... 与格式字符串对应的可变参数
 * @return 完整格式化结果的字符数，-1 表示格式字符串无效或格式化失败
 */
int LCD_Printf(int16_t x, int16_t y, const char *format, ...)
  __attribute__((format(printf, 3, 4)));

#ifdef __cplusplus
}
#endif

#endif /* LCD_USER_H */
