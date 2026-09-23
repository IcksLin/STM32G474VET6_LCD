/**
 * @file    app_lvgl.h
 * @brief   LVGL 应用层接口：清屏、初始化演示界面并驱动 LVGL 周期任务。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef APP_LVGL_H
#define APP_LVGL_H

/** @brief 初始化 LVGL、显示/输入端口并构建菜单演示界面。 */
void App_Lvgl_Init(void);
/** @brief 主循环调用，驱动 LVGL 定时器与刷新。 */
void App_Lvgl_Process(void);

#endif /* APP_LVGL_H */
