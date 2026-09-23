/**
 * @file    zdt_x57.c
 * @brief   ZDT X57 步进驱动器实现：命令编码、CAN 分包发送与 FDCAN 绑定。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#include "zdt_x57.h"

#include <stddef.h>

#include "stm32g4xx_hal.h"

/** @brief ZDT 串口命令固定的校验字节。 */
#define ZDT_X57_CHECKSUM       0x6BU
/** @brief 驱动器允许的最大转速，单位 RPM。 */
#define ZDT_X57_MAX_SPEED_RPM  4000.0f

/** @brief 当前注册的单帧 CAN 发送函数。 */
static ZDT_X57_CanTransmit zdt_can_transmit;
/** @brief 发送函数使用的外部上下文。 */
static void *zdt_can_context;

#ifdef HAL_FDCAN_MODULE_ENABLED
/**
 * @brief 使用 STM32 HAL 向 FDCAN 发送一个 ZDT 扩展数据帧。
 * @param extended_id 29 位扩展帧标识符。
 * @param data 数据区。
 * @param length 数据长度，范围 1-8 字节。
 * @param context FDCAN_HandleTypeDef 指针。
 * @return true 已进入发送 FIFO；false 参数或 HAL 发送失败。
 */
static bool zdt_fdcan_transmit(uint32_t extended_id, const uint8_t *data,
                               uint8_t length, void *context)
{
  FDCAN_TxHeaderTypeDef header = {0};
  static const uint32_t dlc_table[9] = {
      FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2,
      FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
      FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8};

  if ((context == NULL) || (data == NULL) ||
      (length > ZDT_X57_CAN_MAX_DATA_LENGTH)) {
    return false;
  }
  header.Identifier = extended_id;
  header.IdType = FDCAN_EXTENDED_ID;
  header.TxFrameType = FDCAN_DATA_FRAME;
  header.DataLength = dlc_table[length];
  header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header.BitRateSwitch = FDCAN_BRS_OFF;
  header.FDFormat = FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  header.MessageMarker = 0U;
  return HAL_FDCAN_AddMessageToTxFifoQ(
             (FDCAN_HandleTypeDef *)context, &header, (uint8_t *)data) ==
         HAL_OK;
}
#endif

/**
 * @brief 将完整串口形式命令转换成 ZDT CAN 扩展帧并自动分包。
 * @param command 含地址、功能码、数据和校验字节的完整命令。
 * @param length 完整命令长度。
 * @return true 全部分包发送成功；false 参数非法、未绑定或任一分包失败。
 */
static bool zdt_send_command(const uint8_t *command, uint8_t length)
{
  uint8_t offset = 2U;
  uint8_t packet_index = 0U;

  if ((command == NULL) || (length < 3U) || (zdt_can_transmit == NULL)) {
    return false;
  }

  while (offset < length) {
    uint8_t frame[ZDT_X57_CAN_MAX_DATA_LENGTH];
    uint8_t remaining = (uint8_t)(length - offset);
    uint8_t chunk = remaining > 7U ? 7U : remaining;
    uint32_t extended_id =
        ((uint32_t)command[0] << 8U) | (uint32_t)packet_index;

    frame[0] = command[1];
    for (uint8_t i = 0U; i < chunk; ++i) {
      frame[i + 1U] = command[offset + i];
    }
    if (!zdt_can_transmit(extended_id, frame, (uint8_t)(chunk + 1U),
                          zdt_can_context)) {
      return false;
    }
    offset = (uint8_t)(offset + chunk);
    ++packet_index;
  }
  return true;
}

void ZDT_X57_SetCanTransmit(ZDT_X57_CanTransmit transmit, void *context)
{
  zdt_can_transmit = transmit;
  zdt_can_context = context;
}

bool ZDT_X57_IsCanReady(void)
{
  return zdt_can_transmit != NULL;
}

bool ZDT_X57_BindFdcan(void *fdcan_handle)
{
#ifdef HAL_FDCAN_MODULE_ENABLED
  FDCAN_HandleTypeDef *handle = (FDCAN_HandleTypeDef *)fdcan_handle;
  FDCAN_FilterTypeDef filter = {0};

  if (handle == NULL) {
    return false;
  }

  /*
   * ZDT 回复同样使用扩展帧。掩码为 0 表示先接收全部扩展帧，
   * 后续加入接收解析时无需再次修改 CubeMX 生成的 fdcan.c。
   */
  filter.IdType = FDCAN_EXTENDED_ID;
  filter.FilterIndex = 0U;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = 0U;
  filter.FilterID2 = 0U;
  if ((HAL_FDCAN_ConfigFilter(handle, &filter) != HAL_OK) ||
      (HAL_FDCAN_ConfigGlobalFilter(
           handle, FDCAN_REJECT, FDCAN_REJECT,
           FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) ||
      (HAL_FDCAN_Start(handle) != HAL_OK)) {
    return false;
  }
  ZDT_X57_SetCanTransmit(zdt_fdcan_transmit, handle);
  return true;
#else
  (void)fdcan_handle;
  return false;
#endif
}

bool ZDT_X57_RunVelocity(uint8_t address, float velocity_rpm,
                         uint16_t ramp_rpm_s)
{
  uint8_t command[9];
  float magnitude = velocity_rpm < 0.0f ? -velocity_rpm : velocity_rpm;
  uint16_t velocity_scaled;

  if ((magnitude > ZDT_X57_MAX_SPEED_RPM) || (address == 0U)) {
    return false;
  }
  velocity_scaled = (uint16_t)(magnitude * 10.0f + 0.5f);
  command[0] = address;
  command[1] = 0xF6U;
  command[2] = velocity_rpm < 0.0f ? 1U : 0U;
  command[3] = (uint8_t)(ramp_rpm_s >> 8U);
  command[4] = (uint8_t)ramp_rpm_s;
  command[5] = (uint8_t)(velocity_scaled >> 8U);
  command[6] = (uint8_t)velocity_scaled;
  command[7] = 0U;
  command[8] = ZDT_X57_CHECKSUM;
  return zdt_send_command(command, sizeof(command));
}

bool ZDT_X57_RunPosition(uint8_t address, float position_degree,
                         float velocity_rpm, bool absolute)
{
  uint8_t command[12];
  float angle = position_degree < 0.0f ? -position_degree : position_degree;
  uint16_t velocity_scaled;
  uint32_t position_scaled;

  if ((address == 0U) || (velocity_rpm < 0.0f) ||
      (velocity_rpm > ZDT_X57_MAX_SPEED_RPM)) {
    return false;
  }
  velocity_scaled = (uint16_t)(velocity_rpm * 10.0f + 0.5f);
  position_scaled = (uint32_t)(angle * 10.0f + 0.5f);
  command[0] = address;
  command[1] = 0xFBU;
  command[2] = position_degree < 0.0f ? 1U : 0U;
  command[3] = (uint8_t)(velocity_scaled >> 8U);
  command[4] = (uint8_t)velocity_scaled;
  command[5] = (uint8_t)(position_scaled >> 24U);
  command[6] = (uint8_t)(position_scaled >> 16U);
  command[7] = (uint8_t)(position_scaled >> 8U);
  command[8] = (uint8_t)position_scaled;
  command[9] = absolute ? 1U : 0U;
  command[10] = 0U;
  command[11] = ZDT_X57_CHECKSUM;
  return zdt_send_command(command, sizeof(command));
}

bool ZDT_X57_Stop(uint8_t address)
{
  uint8_t command[5] = {address, 0xFEU, 0x98U, 0U, ZDT_X57_CHECKSUM};
  return (address != 0U) && zdt_send_command(command, sizeof(command));
}

bool ZDT_X57_Home(uint8_t address, uint8_t origin_mode)
{
  uint8_t command[5] = {address, 0x9AU, origin_mode, 0U,
                        ZDT_X57_CHECKSUM};
  return (address != 0U) && (origin_mode <= 3U) &&
         zdt_send_command(command, sizeof(command));
}
