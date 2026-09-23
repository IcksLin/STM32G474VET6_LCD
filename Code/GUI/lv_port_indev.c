/**
 * @file    lv_port_indev.c
 * @brief   LVGL 输入端口实现：消费 menu_key 的按键事件并映射为 keypad 键值。
 * @note    用户自建代码；按键扫描仍由 menu_key 模块负责去抖与长短按判定。
 */

#include "lv_port_indev.h"

#include "menu_key.h"

/** @brief keypad 输入设备使用的焦点组。 */
static lv_group_t *indev_group;
/** @brief 最近一次映射出的 LVGL 键值，用于松开时保持键码。 */
static uint32_t last_key = LV_KEY_ENTER;

/**
 * @brief LVGL 读取回调：把一次菜单动作转换为 keypad 按键状态。
 * @param indev 调用本回调的输入设备。
 * @param data 需填充的输入数据结构。
 * @note 组导航使用 LV_KEY_PREV/LV_KEY_NEXT，确认/返回映射为 ENTER/ESC。
 */
static void lv_port_indev_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  MenuAction action = MenuKey_GetAction();
  (void)indev;

  data->key = last_key;
  data->state = LV_INDEV_STATE_RELEASED;

  switch (action) {
    case MENU_ACTION_UP:
    case MENU_ACTION_UP_LONG:
      last_key = LV_KEY_PREV;
      break;
    case MENU_ACTION_DOWN:
    case MENU_ACTION_DOWN_LONG:
      last_key = LV_KEY_NEXT;
      break;
    case MENU_ACTION_OK:
    case MENU_ACTION_OK_LONG:
      last_key = LV_KEY_ENTER;
      break;
    case MENU_ACTION_BACK:
    case MENU_ACTION_BACK_LONG:
      last_key = LV_KEY_ESC;
      break;
    default:
      data->continue_reading = false;
      return;
  }

  data->key = last_key;
  data->state = LV_INDEV_STATE_PRESSED;
  data->continue_reading = false;
}

void LvPortIndev_Init(void)
{
  lv_indev_t *indev;   /**< 新建的 keypad 输入设备。 */

  MenuKey_Init();

  indev_group = lv_group_create();
  lv_group_set_default(indev_group);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev, lv_port_indev_read);
  lv_indev_set_group(indev, indev_group);
}

lv_group_t *LvPortIndev_GetGroup(void)
{
  return indev_group;
}
