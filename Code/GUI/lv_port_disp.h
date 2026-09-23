/**
 * @file    lv_port_disp.h
 * @brief   LVGL 显示端口接口：把 LVGL 渲染结果接入 ST7789 硬件层。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#include "lvgl.h"

/** @brief 创建 LVGL 显示对象并注册刷新回调。 */
void LvPortDisp_Init(void);

#endif /* LV_PORT_DISP_H */
