#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include <stddef.h>
#include <stdint.h>

#include "menu_core.h"

/**
 * @brief 根据菜单模型绘制当前同级菜单页到 LCD 帧缓存。
 * @param items 完整菜单项数组。
 * @param item_count 菜单项数组元素数量。
 * @param current_item 当前选中的菜单项。
 * @return 无。
 * @note 本函数只写帧缓存，不调用 LCD_Update。
 */
void MenuView_System_Navigation_Common(const MenuItem *items,
                                       size_t item_count,
                                       const MenuItem *current_item);

/**
 * @brief 绘制通用功能提示页到 LCD 帧缓存。
 * @param title 页面标题。
 * @param line1 第一行说明文字。
 * @param line2 第二行说明文字。
 * @return 无。
 * @note 所有文字都会限制在 16 像素圆角安全区内。
 */
void MenuView_User_FunctionPage_Common(const char *title, const char *line1,
                                       const char *line2);

/**
 * @brief 绘制按键映射测试页面到 LCD 帧缓存。
 * @param value 当前测试计数值。
 * @return 无。
 */
void MenuView_User_KeyRemapTest_8(int32_t value);

#endif
