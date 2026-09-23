/**
 * @file    menu_user.h
 * @brief   用户菜单配置接口：菜单表获取、业务状态初始化与跨页面动作处理。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef MENU_USER_H
#define MENU_USER_H

#include <stddef.h>

#include "menu_core.h"

/**
 * @brief 初始化用户菜单页面所使用的业务状态。
 * @return 无。
 */
void MenuUser_Init(void);

/**
 * @brief 获取由用户配置的完整菜单结构表。
 * @param item_count 用于返回菜单项数量的指针，不可为 NULL。
 * @return 可由菜单核心读取和替换回调的菜单项数组。
 */
MenuItem *MenuUser_GetItems(size_t *item_count);

/**
 * @brief 在菜单核心执行默认导航前处理跨页面用户动作。
 * @param item 当前菜单项。
 * @param action 本次按键动作。
 * @return true 动作已被用户层消费；false 继续执行菜单核心默认逻辑。
 */
bool MenuUser_HandleGlobalAction(const MenuItem *item, MenuAction action);

/**
 * @brief 用户自定义功能页面的可编译模板。
 * @param action 菜单核心分发的按键动作。
 * @return 无。
 * @note 默认未挂载；复制或直接挂载到 user_menu_items 即可使用。
 */
void MenuUser_PageTemplate(MenuAction action);

#endif
