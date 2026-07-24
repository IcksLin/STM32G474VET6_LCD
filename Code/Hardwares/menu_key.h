#ifndef MENU_KEY_H
#define MENU_KEY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MENU_KEY_COUNT 4U /**< 菜单系统使用的物理按键数量。 */

typedef enum {
  MENU_ACTION_UP = 0,    /**< 向上键短按。 */
  MENU_ACTION_DOWN,      /**< 向下键短按。 */
  MENU_ACTION_OK,        /**< 确认键短按。 */
  MENU_ACTION_BACK,      /**< 返回键短按。 */
  MENU_ACTION_WAITING,   /**< 当前没有待处理按键事件。 */
  MENU_ACTION_UP_LONG,   /**< 向上键长按。 */
  MENU_ACTION_DOWN_LONG, /**< 向下键长按。 */
  MENU_ACTION_OK_LONG,   /**< 确认键长按。 */
  MENU_ACTION_BACK_LONG  /**< 返回键长按。 */
} MenuAction;

/**
 * @brief 复位四个菜单按键的消抖、计时和事件状态。
 * @return 无。
 * @note GPIO 初始化由 STM32CubeMX 管理。
 */
void MenuKey_Init(void);

/**
 * @brief 对 PA4、PA6、PA5、PA7 执行一次按键扫描。
 * @return 无。
 * @note 应每 10 ms 调用一次，所有输入均按低电平有效处理。
 */
void MenuKey_Scan(void);

/**
 * @brief 消费下一个短按或长按菜单动作。
 * @return 有事件时返回对应 MenuAction，否则返回 MENU_ACTION_WAITING。
 */
MenuAction MenuKey_GetAction(void);

#ifdef __cplusplus
}
#endif

#endif /* MENU_KEY_H */
