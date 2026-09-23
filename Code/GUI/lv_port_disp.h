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

/**
 * @brief 像素 DMA 传输完成中断中调用，结束本次 LVGL 异步刷新。
 * @note 必须在 DMA 完成时直接通知 LVGL；部分渲染模式可能在
 *       lv_timer_handler() 内等待该通知，不能延迟到下一轮主循环。
 */
void LvPortDisp_FlushComplete(void);

#endif /* LV_PORT_DISP_H */
