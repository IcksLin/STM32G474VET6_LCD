/**
 * @file    lv_port_indev.h
 * @brief   LVGL 输入端口接口：把四个菜单按键接入 keypad 输入设备。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include "lvgl.h"

/** @brief 初始化按键扫描与 LVGL keypad 输入设备。 */
void LvPortIndev_Init(void);
/** @brief 获取输入设备使用的焦点组，供页面添加可聚焦控件。 */
lv_group_t *LvPortIndev_GetGroup(void);

#endif /* LV_PORT_INDEV_H */
