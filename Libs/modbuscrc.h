#ifndef _CRC16_H_
#define _CRC16_H_

#include "bsp.h"
#include <stdio.h>

/*
 * 计算 Modbus RTU CRC16，并把低字节、高字节追加到 buf[len]、buf[len + 1]。
 * 调用者必须保证 buf 至少有 len + 2 字节空间。
 */
void CrcCal(uint8_t *buf, uint16_t len);

/*
 * 校验 Modbus RTU CRC16。len 为不含 CRC 的数据长度，函数读取 buf[len] 和 buf[len + 1]。
 * 返回 0 表示 CRC 正确，返回 1 表示错误或参数非法。
 */
uint8_t CrcCheck(const uint8_t *buf, uint16_t len);

/* 仅计算 Modbus RTU CRC16，不写回缓存。buf 为 NULL 且 len 非 0 时返回 0。 */
uint16_t CrcGet(const uint8_t *buf, uint16_t len);

#endif
