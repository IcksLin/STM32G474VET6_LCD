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
