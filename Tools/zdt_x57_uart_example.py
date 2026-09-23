#!/usr/bin/env python3
"""
ZDT X57 V2 自定义串口协议测试工具。

从机菜单要求：
    P_Serial = UART_FUN
    UartBaud = 115200
    ID_Addr  = 1
    Checksum = 0x6B
    Response = Receive
    S_PosTDP = Disable

安装依赖：
    python -m pip install pyserial

使用示例：
    python zdt_x57_uart_example.py --port COM5 enable
    python zdt_x57_uart_example.py --port COM5 velocity --rpm 300 --ramp 1000
    python zdt_x57_uart_example.py --port COM5 position --degree -360 --rpm 300
    python zdt_x57_uart_example.py --port COM5 stop
    python zdt_x57_uart_example.py --port COM5 home --mode 0
    python zdt_x57_uart_example.py --port COM5 status

串口帧格式：
    [电机地址] [功能码] [辅助码/数据...] [校验字节]

本工具只实现 Checksum=0x6B 的自定义协议，不适用于 Modbus、XOR 或 CRC-8。
"""

from __future__ import annotations

import argparse
import struct
import sys
import time
from typing import Final

try:
    import serial
except ImportError:
    print("缺少 pyserial，请执行：python -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)


CHECKSUM: Final[int] = 0x6B
MAX_RPM: Final[float] = 4000.0


def build_frame(address: int, function: int, payload: bytes = b"") -> bytes:
    """构造地址、功能码、数据和固定 0x6B 校验组成的完整串口帧。"""
    if not 1 <= address <= 255:
        raise ValueError("电机地址必须在 1..255 之间")
    return bytes((address, function)) + payload + bytes((CHECKSUM,))


def command_enable(address: int, enabled: bool = True) -> bytes:
    """构造电机使能控制命令：F3 AB state sync 6B。"""
    return build_frame(address, 0xF3, bytes((0xAB, int(enabled), 0x00)))


def command_velocity(address: int, rpm: float, ramp_rpm_s: int) -> bytes:
    """构造速度控制命令；rpm 符号决定方向，数值放大 10 倍发送。"""
    if abs(rpm) > MAX_RPM:
        raise ValueError(f"转速绝对值不能超过 {MAX_RPM:g} RPM")
    if not 0 <= ramp_rpm_s <= 65535:
        raise ValueError("速度斜率必须在 0..65535 RPM/s 之间")

    direction = 0x01 if rpm < 0 else 0x00
    speed_x10 = round(abs(rpm) * 10.0)
    payload = bytes((direction,)) + struct.pack(">HHB", ramp_rpm_s,
                                                 speed_x10, 0x00)
    return build_frame(address, 0xF6, payload)


def command_position(
    address: int,
    degree: float,
    rpm: float,
    absolute: bool = False,
) -> bytes:
    """
    构造直通限速位置命令。

    degree 的符号决定方向；S_PosTDP=Disable 时角度放大 10 倍。
    absolute=False 表示相对位置，True 表示绝对位置。
    """
    if not 0.0 <= rpm <= MAX_RPM:
        raise ValueError(f"位置模式限速必须在 0..{MAX_RPM:g} RPM 之间")

    direction = 0x01 if degree < 0 else 0x00
    speed_x10 = round(rpm * 10.0)
    degree_x10 = round(abs(degree) * 10.0)
    if degree_x10 > 0xFFFFFFFF:
        raise ValueError("目标角度超出 32 位协议字段范围")

    payload = (
        bytes((direction,))
        + struct.pack(">HI", speed_x10, degree_x10)
        + bytes((int(absolute), 0x00))
    )
    return build_frame(address, 0xFB, payload)


def command_stop(address: int) -> bytes:
    """构造适用于所有运动模式的立即停止命令。"""
    return build_frame(address, 0xFE, bytes((0x98, 0x00)))


def command_home(address: int, mode: int = 0) -> bytes:
    """
    构造触发回零命令。

    mode: 0 单圈就近，1 单圈定向，2 无限位碰撞，3 限位开关。
    """
    if mode not in range(4):
        raise ValueError("回零模式必须是 0、1、2 或 3")
    return build_frame(address, 0x9A, bytes((mode, 0x00)))


def command_read_status(address: int) -> bytes:
    """构造读取电机状态标志命令。"""
    return build_frame(address, 0x3A)


def format_hex(data: bytes) -> str:
    """将字节序列格式化为便于对照说明书的十六进制文本。"""
    return " ".join(f"{value:02X}" for value in data)


def explain_reply(reply: bytes) -> str:
    """解释控制命令常见的四字节响应。"""
    if len(reply) != 4:
        return f"响应长度异常：{len(reply)} 字节"
    if reply[-1] != CHECKSUM:
        return "响应末尾不是 0x6B，请检查校验模式或串口干扰"

    status = reply[2]
    if status == 0x02:
        return "驱动器已正确接收命令"
    if status == 0xE2:
        return "条件不满足：检查使能、堵转保护、校准或回零条件"
    if status == 0xEE:
        return "命令格式错误"
    return f"状态/标志字节：0x{status:02X}"


def exchange(port: str, baudrate: int, frame: bytes,
             response_length: int = 4) -> bytes:
    """打开串口、发送一帧并读取固定长度响应。"""
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
        print(f"TX: {format_hex(frame)}")
        uart.write(frame)
        uart.flush()
        time.sleep(0.02)
        reply = uart.read(response_length)
        print(f"RX: {format_hex(reply) if reply else '<timeout>'}")
        return reply


def parse_arguments() -> argparse.Namespace:
    """解析串口、地址和控制命令参数。"""
    parser = argparse.ArgumentParser(description="ZDT X57 V2 UART 测试工具")
    parser.add_argument("--port", required=True, help="串口名称，例如 COM5")
    parser.add_argument("--baud", type=int, default=115200, help="默认 115200")
    parser.add_argument("--address", type=int, default=1, help="默认电机地址 1")

    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("enable", help="使能电机")
    commands.add_parser("disable", help="关闭电机使能")

    velocity = commands.add_parser("velocity", help="速度模式")
    velocity.add_argument("--rpm", type=float, required=True,
                          help="有符号转速，负数表示反向")
    velocity.add_argument("--ramp", type=int, default=1000,
                          help="速度斜率 RPM/s")

    position = commands.add_parser("position", help="直通限速位置模式")
    position.add_argument("--degree", type=float, required=True,
                          help="有符号目标角度")
    position.add_argument("--rpm", type=float, default=300,
                          help="最大转速")
    position.add_argument("--absolute", action="store_true",
                          help="使用绝对位置；默认相对位置")

    commands.add_parser("stop", help="立即停止")
    home = commands.add_parser("home", help="触发回零")
    home.add_argument("--mode", type=int, choices=range(4), default=0)
    commands.add_parser("status", help="读取电机状态标志")
    return parser.parse_args()


def main() -> int:
    """根据命令行参数构造并发送对应的 ZDT 串口帧。"""
    args = parse_arguments()
    builders = {
        "enable": lambda: command_enable(args.address, True),
        "disable": lambda: command_enable(args.address, False),
        "velocity": lambda: command_velocity(args.address, args.rpm,
                                             args.ramp),
        "position": lambda: command_position(args.address, args.degree,
                                             args.rpm, args.absolute),
        "stop": lambda: command_stop(args.address),
        "home": lambda: command_home(args.address, args.mode),
        "status": lambda: command_read_status(args.address),
    }

    try:
        reply = exchange(args.port, args.baud, builders[args.command]())
    except (ValueError, serial.SerialException) as error:
        print(f"错误：{error}", file=sys.stderr)
        return 1

    if not reply:
        print("驱动器未响应：检查 TX/RX 交叉、共地、UART_FUN 和波特率")
        return 1

    print(explain_reply(reply))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
