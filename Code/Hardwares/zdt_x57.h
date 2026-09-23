/**
 * @file    zdt_x57.h
 * @brief   ZDT X57 步进驱动器接口：速度、位置、停止与回零命令。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef ZDT_X57_H
#define ZDT_X57_H

#include <stdbool.h>
#include <stdint.h>

/** @brief ZDT X57 V2 CAN 扩展帧的最大发送数据长度。 */
#define ZDT_X57_CAN_MAX_DATA_LENGTH 8U

/** @brief CAN 单帧发送适配器；extended_id 必须按扩展帧发送。 */
typedef bool (*ZDT_X57_CanTransmit)(uint32_t extended_id,
                                    const uint8_t *data,
                                    uint8_t length,
                                    void *context);

/**
 * @brief 注册由 CubeMX FDCAN 初始化代码提供的底层发送适配器。
 * @param transmit 发送一个 CAN 扩展数据帧的函数；NULL 表示解除绑定。
 * @param context 透传给发送函数的用户上下文，通常为 FDCAN_HandleTypeDef 指针。
 * @return 无。
 */
void ZDT_X57_SetCanTransmit(ZDT_X57_CanTransmit transmit, void *context);

/**
 * @brief 查询 CAN 发送适配器是否已经注册。
 * @return true 已注册，可以发送；false 尚未绑定 FDCAN。
 */
bool ZDT_X57_IsCanReady(void);

/**
 * @brief 将 CubeMX 生成的 FDCAN 句柄绑定为 ZDT CAN 发送适配器并启动外设。
 * @param fdcan_handle FDCAN_HandleTypeDef 指针；使用 void* 以便未启用 FDCAN 时仍可编译。
 * @return true FDCAN 已启动并绑定；false 尚未由 CubeMX 启用或启动失败。
 * @note 在 MX_FDCANx_Init() 之后调用，例如 ZDT_X57_BindFdcan(&hfdcan2)。
 */
bool ZDT_X57_BindFdcan(void *fdcan_handle);

/**
 * @brief 以指定速度连续旋转。
 * @param address 电机地址，出厂默认值为 1。
 * @param velocity_rpm 有符号目标转速，正值为 CW，负值为 CCW。
 * @param ramp_rpm_s 速度斜率，单位 RPM/s。
 * @return true 所有 CAN 分包均已交给适配器；false 参数或发送失败。
 */
bool ZDT_X57_RunVelocity(uint8_t address, float velocity_rpm,
                         uint16_t ramp_rpm_s);

/**
 * @brief 以直通限速位置模式运动到指定角度。
 * @param address 电机地址，出厂默认值为 1。
 * @param position_degree 有符号目标角度，符号决定旋转方向。
 * @param velocity_rpm 最大转速，单位 RPM。
 * @param absolute true 使用绝对位置；false 使用相对位置。
 * @return true 所有 CAN 分包均已交给适配器；false 参数或发送失败。
 */
bool ZDT_X57_RunPosition(uint8_t address, float position_degree,
                         float velocity_rpm, bool absolute);

/**
 * @brief 立即停止当前运动。
 * @param address 电机地址。
 * @return true 命令已交给 CAN 适配器；false 尚未绑定或发送失败。
 */
bool ZDT_X57_Stop(uint8_t address);

/**
 * @brief 按驱动器已保存的回零参数触发回零。
 * @param address 电机地址。
 * @param origin_mode 回零模式：0 就近单圈，1 定向单圈，2 无限位碰撞，3 限位开关。
 * @return true 命令已交给 CAN 适配器；false 尚未绑定或发送失败。
 */
bool ZDT_X57_Home(uint8_t address, uint8_t origin_mode);

#endif
