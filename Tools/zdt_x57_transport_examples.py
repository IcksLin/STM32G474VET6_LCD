#!/usr/bin/env python3
"""
ZDT X57 V2 多传输方式移植参考。

依赖：
    python -m pip install pyserial python-can

协议关系：
    TTL UART / RS232 / RS485:
        直接发送完整命令：
        [地址] [功能码] [参数...] [0x6B]

    CAN:
        扩展帧 ID = (地址 << 8) | 分包序号
        数据区      = [功能码] + 最多 7 字节参数

RS232 与 TTL UART 的软件帧完全相同，仅物理收发器不同。
RS485 的命令帧相同，但半双工接口可能需要控制 DE/RE 方向。
驱动器不支持 I2C，因此本文件不提供 I2C 伪实现。
"""

from __future__ import annotations

import time
from collections.abc import Iterable

import can
import serial
from serial.rs485 import RS485Settings

from zdt_x57_uart_example import (
    command_enable,
    command_home,
    command_position,
    command_read_status,
    command_stop,
    command_velocity,
    format_hex,
)


def ttl_uart_exchange(
    port: str,
    frame: bytes,
    baudrate: int = 115200,
    response_length: int = 4,
) -> bytes:
    """
    通过 TTL UART 或带硬件电平转换的 RS232 发送完整 ZDT 命令。

    TTL 接线：主机 TX -> ZDT R，主机 RX <- ZDT T，双方 GND 共地。
    RS232 接线：主机 UART 和 ZDT 之间必须分别经过 RS232 电平转换器。
    """
    with serial.Serial(
        port=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.3,
        write_timeout=0.3,
    ) as uart:
        uart.reset_input_buffer()
        uart.write(frame)
        uart.flush()
        return uart.read(response_length)


def rs485_exchange(
    port: str,
    frame: bytes,
    baudrate: int = 115200,
    response_length: int = 4,
) -> bytes:
    """
    通过两线半双工 RS485 发送完整 ZDT 命令。

    USB-RS485 适配器通常自动控制收发方向；若驱动不支持自动方向，
    需要按适配器文档调整 RTS 电平，或在 MCU 中手动控制 DE/RE。
    """
    with serial.Serial(
        port=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.3,
        write_timeout=0.3,
    ) as uart:
        uart.rs485_mode = RS485Settings(
            rts_level_for_tx=True,
            rts_level_for_rx=False,
            delay_before_tx=0.0,
            delay_before_rx=0.0,
        )
        uart.reset_input_buffer()
        uart.write(frame)
        uart.flush()
        return uart.read(response_length)


def command_to_can_frames(command: bytes) -> Iterable[tuple[int, bytes]]:
    """
    把完整串口形式命令转换为 ZDT CAN 扩展帧。

    参数 command 必须仍包含地址和 0x6B；函数会把地址移入扩展帧 ID，
    并让每个分包的数据首字节重复功能码。
    """
    if len(command) < 3:
        raise ValueError("ZDT 命令至少需要地址、功能码和校验字节")

    address = command[0]
    function = command[1]
    remaining = command[2:]
    packet_index = 0

    while remaining:
        chunk = remaining[:7]
        remaining = remaining[7:]
        extended_id = (address << 8) | packet_index
        yield extended_id, bytes((function,)) + chunk
        packet_index += 1


def can_send(
    interface: str,
    channel: str,
    command: bytes,
    bitrate: int = 500_000,
) -> None:
    """
    使用 python-can 发送 ZDT 扩展帧。

    interface/channel 取决于适配器，例如：
      SocketCAN: interface="socketcan", channel="can0"
      PCAN:      interface="pcan",      channel="PCAN_USBBUS1"
      Vector:    interface="vector",    channel="0"
    """
    with can.Bus(interface=interface, channel=channel,
                 bitrate=bitrate) as bus:
        for extended_id, payload in command_to_can_frames(command):
            message = can.Message(
                arbitration_id=extended_id,
                is_extended_id=True,
                is_fd=False,
                bitrate_switch=False,
                data=payload,
            )
            bus.send(message, timeout=0.3)
            print(f"CAN TX ID={extended_id:08X} DATA={format_hex(payload)}")


def demo_uart(port: str = "COM5") -> None:
    """演示 TTL UART/RS232 下的使能、速度、停止和状态读取。"""
    commands = (
        command_enable(1, True),
        command_velocity(1, rpm=300.0, ramp_rpm_s=1000),
        command_stop(1),
        command_read_status(1),
    )
    for frame in commands:
        reply = ttl_uart_exchange(port, frame)
        print(f"TX: {format_hex(frame)}")
        print(f"RX: {format_hex(reply) if reply else '<timeout>'}")
        time.sleep(0.1)


def demo_rs485(port: str = "COM6") -> None:
    """演示 RS485 下的相对位置控制和单圈就近回零。"""
    commands = (
        command_position(1, degree=-360.0, rpm=300.0, absolute=False),
        command_home(1, mode=0),
    )
    for frame in commands:
        reply = rs485_exchange(port, frame)
        print(f"TX: {format_hex(frame)}")
        print(f"RX: {format_hex(reply) if reply else '<timeout>'}")
        time.sleep(0.1)


def demo_can(interface: str = "socketcan", channel: str = "can0") -> None:
    """演示经典 CAN 扩展帧下的位置命令自动分包。"""
    command = command_position(
        address=1,
        degree=360.0,
        rpm=300.0,
        absolute=False,
    )
    can_send(interface, channel, command)


if __name__ == "__main__":
    # 按实际硬件取消其中一个示例的注释。
    # demo_uart("COM5")
    # demo_rs485("COM6")
    # demo_can("pcan", "PCAN_USBBUS1")
    print("请在文件末尾选择并取消一个 demo_* 调用的注释。")
