#include "Findstr.h"
#include <string.h>

/*
 * 函数名称: Findstr
 * 功能说明: 在以 \0 结尾的接收缓存中查找目标字符串。
 *           这个函数遇到 buffer[j] == 0 会认为报文已经结束，因此不要用于二进制缓存；
 *           二进制缓存请使用 FindstrMem()。
 * 输入参数: str    - 待查找的 ASCII 字符串，不能为空，且不能是空串。
 *           buffer - 接收缓存首地址，不能为空。
 *           maxlen - 最多检查的缓存长度，防止没有 \0 时越界。
 * 返回值  : 找到时返回首次匹配偏移；失败、参数非法或偏移超过 0xFFFE 时返回 0xFFFF。
 */
uint16_t Findstr(const char *str, const uint8_t *buffer, uint32_t maxlen)
{
	uint32_t str_len;
	uint32_t limit;
	uint32_t j;
	uint32_t i;

	if ((str == NULL) || (buffer == NULL) || (maxlen == 0U))
	{
		return 0xFFFF;
	}
	str_len = strlen(str);
	if ((str_len == 0U) || (str_len > maxlen) || (maxlen > 0xFFFFU))
	{
		return 0xFFFF;
	}

	limit = maxlen - str_len;
	for (j = 0; j <= limit; j++)
	{
		if (buffer[j] == 0)
		{
			return 0xFFFF;
		}
		for (i = 0; i < str_len; i++)
		{
			if ((uint8_t)str[i] != buffer[j + i])
			{
				break;
			}
		}
		if (i == str_len)
		{
			return (uint16_t)j;
		}
	}
	return 0xFFFF;
}

/*
 * 函数名称: FindstrMem
 * 功能说明: 在原始内存中查找目标字符串，不把 \0 视为缓存结束。
 *           适用于 AT 返回缓存、串口二进制帧等可能包含 0 字节的数据。
 * 输入参数: target_str     - 待查找字符串，不能为空，且不能是空串。
 *           target_address - 待搜索内存首地址，不能为空。
 *           len            - 待搜索内存长度。
 * 返回值  : 找到时返回首次匹配偏移；失败或参数非法返回 0xFFFF。
 */
uint16_t FindstrMem(const char *target_str, const uint8_t *target_address, uint32_t len)
{
	uint32_t target_len;
	uint32_t limit;
	uint32_t i;
	uint32_t j;

	if ((target_str == NULL) || (target_address == NULL) || (len == 0U) || (len > 0xFFFFU))
	{
		return 0xFFFF;
	}
	target_len = strlen(target_str);
	if ((target_len == 0U) || (target_len > len))
	{
		return 0xFFFF;
	}

	limit = len - target_len;
	for (j = 0; j <= limit; j++)
	{
		for (i = 0; i < target_len; i++)
		{
			if ((uint8_t)target_str[i] != target_address[j + i])
			{
				break;
			}
		}
		if (i == target_len)
		{
			return (uint16_t)j;
		}
	}
	return 0xFFFF;
}

/*
 * 函数名称: is_str
 * 功能说明: 检查字符串是否由可打印 ASCII 字符组成，并且在 maxlen 范围内以 \0 结束。
 * 输入参数: str    - 待检查字符串。
 *           maxlen - 最大检查长度。
 * 返回值  : 合法时返回字符串长度；非法、空串、未结束或包含不可打印字符时返回 0。
 */
uint8_t is_str(const char *str, uint16_t maxlen)
{
	uint16_t i;

	if ((str == NULL) || (maxlen == 0U))
	{
		return 0;
	}
	for (i = 0; i < maxlen; i++)
	{
		if (str[i] == 0)
		{
			return (uint8_t)((i <= 0xFFU) ? i : 0U);
		}
		if (!((str[i] >= 0x20) && (str[i] <= 0x7E)))
		{
			return 0;
		}
	}
	return 0;
}

/*
 * 函数名称: ESP_Analysis_StrN
 * 功能说明: 从 ESP 模块返回的文本报文中提取 Object 后面的值。
 *           解析格式为 “Object ... : value\r\n”，冒号前允许存在其他字符，冒号后会跳过空格。
 *           输出始终以 \0 结束；如果 value 超过 outmax - 1，会返回失败，避免静默截断。
 * 输入参数: Buffer - 输入报文缓存。
 *           Object - 要查找的字段名。
 *           str    - 输出缓存。
 *           maxlen - 输入缓存有效长度。
 *           outmax - 输出缓存长度。
 * 返回值  : 成功返回 value 在 Buffer 中的起始偏移；失败返回 0xFFFF。
 */
uint16_t ESP_Analysis_StrN(const uint8_t *Buffer, const char *Object, char *str, uint16_t maxlen, uint16_t outmax)
{
	uint16_t offset;
	uint16_t i;

	if ((Buffer == NULL) || (Object == NULL) || (str == NULL) || (maxlen == 0U) || (outmax == 0U))
	{
		return 0xFFFF;
	}
	str[0] = 0;

	offset = FindstrMem(Object, Buffer, maxlen);
	if (offset == 0xFFFF)
	{
		return 0xFFFF;
	}

	while ((offset < maxlen) && (Buffer[offset] != ':'))
	{
		offset++;
	}
	if (offset >= maxlen)
	{
		return 0xFFFF;
	}
	offset++;

	while ((offset < maxlen) && (Buffer[offset] == ' '))
	{
		offset++;
	}
	if (offset >= maxlen)
	{
		return 0xFFFF;
	}

	for (i = 0; i < (outmax - 1U); i++)
	{
		if ((offset + i) >= maxlen)
		{
			break;
		}
		if (((offset + i + 1U) < maxlen) && (Buffer[offset + i] == 0x0d) && (Buffer[offset + i + 1U] == 0x0a))
		{
			break;
		}
		str[i] = (char)Buffer[offset + i];
	}
	str[i] = 0;

	if (((offset + i) < maxlen) && !(((offset + i + 1U) < maxlen) && (Buffer[offset + i] == 0x0d) && (Buffer[offset + i + 1U] == 0x0a)))
	{
		return 0xFFFF;
	}
	return offset;
}

/*
 * 函数名称: ESP_Analysis_Str
 * 功能说明: ESP_Analysis_StrN() 的兼容封装，输出长度沿用输入长度。
 * 注意事项: 新代码优先使用 ESP_Analysis_StrN()，显式传入输出缓存大小更安全。
 */
uint16_t ESP_Analysis_Str(const uint8_t *Buffer, const char *Object, char *str, uint16_t maxlen)
{
	return ESP_Analysis_StrN(Buffer, Object, str, maxlen, maxlen);
}

/*
 * 函数名称: Value_Cmp
 * 功能说明: 返回两个有符号 32 位数的绝对差值。
 */
int32_t Value_Cmp(int32_t value1, int32_t value2)
{
	return (value1 > value2) ? (value1 - value2) : (value2 - value1);
}

/*
 * 函数名称: printfhex
 * 功能说明: 按十六进制格式打印缓冲区数据，仅用于调试输出。
 */
void printfhex(const uint8_t *buffer, uint16_t len, const char *frontstr)
{
	uint16_t i;

	if ((buffer == NULL) || (frontstr == NULL))
	{
		return;
	}
	printf("\r\n%s:", frontstr);
	for (i = 0; i < len; i++)
	{
		printf(" %02x", buffer[i]);
	}
}

/*
 * 函数名称: DecNum_Mult
 * 功能说明: 根据小数位数返回 10 的 decnum 次方，用于 JSON/显示值缩放。
 */
uint32_t DecNum_Mult(uint8_t decnum)
{
	uint32_t value = 1;
	uint8_t i;

	for (i = 0; i < decnum; i++)
	{
		value *= 10U;
	}
	return value;
}

/*
 * 函数名称: Swap_Byte
 * 功能说明: 交换两个字节。若两个指针相同，不执行操作，避免异或交换把数据清零。
 */
void Swap_Byte(uint8_t *a, uint8_t *b)
{
	uint8_t tmp;

	if ((a == NULL) || (b == NULL) || (a == b))
	{
		return;
	}
	tmp = *a;
	*a = *b;
	*b = tmp;
}
