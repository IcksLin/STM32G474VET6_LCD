/**
 * @file    interrupt_callback.c
 * @brief   HAL 回调集中实现：TIM17 周期扫描按键，SPI1 TX-DMA 完成通知 LCD。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#include "lcd_hw.h"
#include "menu_key.h"

#include "stm32g4xx_hal.h"

/**
 * @brief 处理 HAL 定时器周期完成回调。
 * @param htim 触发周期完成事件的定时器句柄。
 * @return 无。
 * @note TIM17 周期配置为 10 ms，用于执行一次菜单按键扫描。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if ((htim != NULL) && (htim->Instance == TIM17)) {
    MenuKey_Scan();
  }
}

/**
 * @brief 处理 SPI 发送完成回调。
 * @param hspi 完成发送的 SPI 句柄。
 * @return 无。
 * @note SPI1 的像素 DMA 传输结束后在此收尾并通知显示层。
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if ((hspi != NULL) && (hspi->Instance == SPI1)) {
    LCD_HW_HandleTxComplete();
  }
}

/**
 * @brief 处理 SPI 错误回调。
 * @param hspi 出错的 SPI 句柄。
 * @return 无。
 * @note 无论错误类型如何都必须释放 LCD 片选并解除 LVGL 的刷新等待，
 *       否则一次 DMA 错误就会让后续界面永久停在 flushing 状态。
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if ((hspi != NULL) && (hspi->Instance == SPI1)) {
    if ((hspi->ErrorCode & HAL_SPI_ERROR_FLAG) != 0U) {
      __HAL_SPI_CLEAR_OVRFLAG(hspi);
    }
    LCD_HW_HandleTxError();
  }
}
