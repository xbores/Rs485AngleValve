#include "modbuscrc.h"

/*
 * 函数名称: CrcGet
 * 功能说明: 计算 Modbus RTU CRC16，初值 0xFFFF，多项式 0xA001。
 * 输入参数: buf - 待计算数据，len 为 0 时允许为 NULL。
 *           len - 数据长度，不包含 CRC 自身。
 * 返回值  : CRC16 结果；buf 为 NULL 且 len 非 0 时返回 0。
 */
uint16_t CrcGet(const uint8_t *buf, uint16_t len)
{
	uint16_t crc = 0xFFFFU;
	uint16_t i;
	uint16_t j;

	if ((buf == NULL) && (len != 0U))
	{
		return 0;
	}
	for (j = 0; j < len; j++)
	{
		crc ^= buf[j];
		for (i = 0; i < 8U; i++)
		{
			if (crc & 0x0001U)
			{
				crc >>= 1;
				crc ^= 0xA001U;
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}

/*
 * 函数名称: CrcCal
 * 功能说明: 计算 Modbus RTU CRC16，并按低字节在前、高字节在后的顺序追加到数据尾部。
 * 注意事项: 调用者必须保证 buf 至少有 len + 2 字节可写空间。
 */
void CrcCal(uint8_t *buf, uint16_t len)
{
	uint16_t crc;

	if (buf == NULL)
	{
		return;
	}
	crc = CrcGet(buf, len);
	buf[len] = (uint8_t)(crc & 0xFFU);
	buf[len + 1U] = (uint8_t)(crc >> 8);
}

/*
 * 函数名称: CrcCheck
 * 功能说明: 校验 Modbus RTU CRC16。len 是不含 CRC 的数据长度。
 * 返回值  : 0 表示校验通过；1 表示校验失败或参数非法。
 */
uint8_t CrcCheck(const uint8_t *buf, uint16_t len)
{
	uint16_t crc;

	if (buf == NULL)
	{
		return 1;
	}
	crc = CrcGet(buf, len);
	return ((buf[len] == (uint8_t)(crc & 0xFFU)) && (buf[len + 1U] == (uint8_t)(crc >> 8))) ? 0U : 1U;
}
