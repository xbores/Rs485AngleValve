#ifndef __CRC32_H
#define __CRC32_H

#include "bsp.h"

/*
 * 计算标准 CRC32/IEEE 校验值。
 * 参数:
 *   buf  - 待校验数据首地址；size 为 0 时允许为 NULL。
 *   size - 参与计算的数据字节数。
 * 返回:
 *   CRC32 结果。buf 为 NULL 且 size 非 0 时返回 0，避免异常访问。
 */
uint32_t crc32(const uint8_t *buf, uint32_t size);

/* CRC32 查表数据，放在只读区，供 crc32() 使用。 */
extern const uint32_t crc32tab[];

/*
 * 计算 CRC8/MAXIM 风格校验值，当前多用于小数据块快速校验。
 * 参数 len 为 0 时允许 ptr 为 NULL。
 */
uint8_t getCRC8(const uint8_t *ptr, uint16_t len);

#endif
