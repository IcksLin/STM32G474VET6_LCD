/**
 * @file    lcd_hw.h
 * @brief   ST7789 显示硬件层接口：初始化、RGB565 区域 DMA 写入与完成回调注册。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef LCD_HW_H
#define LCD_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 面板物理宽度，单位像素。 */
#define LCD_HW_WIDTH  240U
/** @brief 面板物理高度，单位像素。 */
#define LCD_HW_HEIGHT 280U

/** @brief 像素 DMA 传输完成回调类型。 */
typedef void (*LCD_HW_TxCompleteCallback)(void);

/**
 * @brief 初始化 ST7789 控制器并开启 LCD 背光
 * @return 0 初始化成功，-1 表示 SPI 通信失败
 */
int LCD_HW_Init(void);

/**
 * @brief 以 DMA 方式启动一次 RGB565 区域写入（异步）
 * @param x 目标区域左上角横坐标
 * @param y 目标区域左上角纵坐标
 * @param width 目标区域宽度，单位为像素
 * @param height 目标区域高度，单位为像素
 * @param pixels 按行连续排列的像素数据，且每个像素已经是面板字节序（高字节在前）
 * @return 0 表示 DMA 已启动，-1 表示参数非法或启动失败
 * @note 本函数立即返回；传输完成后由中断触发已注册的完成回调。
 */
int LCD_HW_WriteAreaRGB565(uint16_t x, uint16_t y, uint16_t width,
                           uint16_t height, const uint16_t *pixels);

/**
 * @brief 注册一次 DMA 传输完成后的回调
 * @param callback 完成回调，可为 NULL 以取消
 */
void LCD_HW_SetTxCompleteCallback(LCD_HW_TxCompleteCallback callback);

/**
 * @brief 在 SPI 发送完成中断中调用，收尾片选并触发完成回调
 * @note 仅供 HAL_SPI_TxCpltCallback 调用。
 */
void LCD_HW_HandleTxComplete(void);

/**
 * @brief 在 SPI DMA 错误回调中调用，释放片选并解除显示层等待
 * @note 本函数把本次传输作为失败结束；上层下一次失效刷新可重画区域。
 */
void LCD_HW_HandleTxError(void);

#ifdef __cplusplus
}
#endif

#endif /* LCD_HW_H */
