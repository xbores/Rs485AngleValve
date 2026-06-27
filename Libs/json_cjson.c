#include "json.h"
#include "cJSON.h"
#include <string.h>
/*
 * 函数名称: cJSON_GetObjectItemRecursive
 * 功能说明: 从 root 开始深度优先递归查找字段名为 name 的节点。
 *           用于兼容云端报文中字段可能嵌套在不同对象层级的情况。
 * 输入参数: root - cJSON 根节点或任意对象节点。
 *           name - 要查找的字段名，不能为空。
 * 返 回 值: 找到返回节点指针，失败返回 NULL。
 */
cJSON *cJSON_GetObjectItemRecursive(cJSON *root, const char *name)
{
	cJSON *item;
	cJSON *find_item;

	if ((root == NULL) || (name == NULL) || (name[0] == 0))
	{
		return NULL;
	}
	for (item = root->child; item != NULL; item = item->next)
	{
		if ((item->string != NULL) && (strcmp(item->string, name) == 0))
		{
			return item;
		}
		if (item->child != NULL)
		{
			find_item = cJSON_GetObjectItemRecursive(item, name);
			if (find_item != NULL)
			{
				return find_item;
			}
		}
	}
	return NULL;
}

/*
 * 函数名称: cJSON_GetItemWithChannel
 * 功能说明: 递归查找名称为 prefix + 十进制通道号 的节点，并解析通道号。
 *           例如 prefix 为 "Part" 时，可匹配 "Part0"、"Part12"。
 * 输入参数: root - cJSON 根节点。prefix - 字段名前缀。channel - 输出通道号。max - 通道上限。
 * 返 回 值: 找到返回原始节点，失败返回 NULL。
 */
static cJSON *cJSON_GetItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max)
{
	cJSON *item;
	cJSON *find_item;
	const char *p;
	uint32_t prefix_len;
	uint32_t channel_value;
	uint32_t digit;
	uint8_t has_channel_digit;

	if ((root == NULL) || (prefix == NULL) || (channel == NULL) || (max == 0U))
	{
		return NULL;
	}
	prefix_len = strlen(prefix);
	if (prefix_len == 0U)
	{
		return NULL;
	}

	for (item = root->child; item != NULL; item = item->next)
	{
		if ((item->string != NULL) && (strncmp(item->string, prefix, prefix_len) == 0))
		{
			p = item->string + prefix_len;
			channel_value = 0;
			has_channel_digit = 0;
			while ((*p >= '0') && (*p <= '9'))
			{
				digit = (uint32_t)(*p - '0');
				if (channel_value > ((0xFFFFFFFFU - digit) / 10U))
				{
					break;
				}
				channel_value = channel_value * 10U + digit;
				has_channel_digit = 1;
				p++;
			}
			if ((has_channel_digit != 0U) && (*p == 0) && (channel_value < max))
			{
				*channel = channel_value;
				return item;
			}
		}
		if (item->child != NULL)
		{
			find_item = cJSON_GetItemWithChannel(item, prefix, channel, max);
			if (find_item != NULL)
			{
				return find_item;
			}
		}
	}
	return NULL;
}

/*
 * 函数名称: cJSON_GetObjectItemWithChannel
 * 功能说明: 查找 prefix + 通道号 字段，并要求节点类型匹配当前函数名称。
 * 输入参数: root - cJSON 根节点。prefix - 字段名前缀。channel - 输出通道号。max - 通道上限。
 * 返 回 值: 类型匹配时返回节点，否则返回 NULL。
 */
cJSON *cJSON_GetObjectItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max)
{
	cJSON *item = cJSON_GetItemWithChannel(root, prefix, channel, max);
	return ((item != NULL) && (item->type == cJSON_Object)) ? item : NULL;
}

/*
 * 函数名称: cJSON_GetNumberItemWithChannel
 * 功能说明: 查找 prefix + 通道号 字段，并要求节点类型匹配当前函数名称。
 * 输入参数: root - cJSON 根节点。prefix - 字段名前缀。channel - 输出通道号。max - 通道上限。
 * 返 回 值: 类型匹配时返回节点，否则返回 NULL。
 */
cJSON *cJSON_GetNumberItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max)
{
	cJSON *item = cJSON_GetItemWithChannel(root, prefix, channel, max);
	return ((item != NULL) && (item->type == cJSON_Number)) ? item : NULL;
}

/*
 * 函数名称: cJSON_GetStringItemWithChannel
 * 功能说明: 查找 prefix + 通道号 字段，并要求节点类型匹配当前函数名称。
 * 输入参数: root - cJSON 根节点。prefix - 字段名前缀。channel - 输出通道号。max - 通道上限。
 * 返 回 值: 类型匹配时返回节点，否则返回 NULL。
 */
cJSON *cJSON_GetStringItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max)
{
	cJSON *item = cJSON_GetItemWithChannel(root, prefix, channel, max);
	return ((item != NULL) && (item->type == cJSON_String)) ? item : NULL;
}

/*
 * 函数名称: cJSON_FindNumberI32
 * 功能说明: 查找有符号 32 位数字字段，按 mult 放大后输出。常用于把小数转成定点整数。
 * 输入参数: root - cJSON 根节点。name - 字段名。value - 输出值。mult - 放大倍率，必须大于 0。
 * 返 回 值: 0 表示成功，1 表示字段不存在、类型错误或越界。
 */
uint8_t cJSON_FindNumberI32(cJSON *root, const char *name, int32_t *value, int32_t mult)
{
	cJSON *item;
	double double_value;

	if ((value == NULL) || (name == NULL) || (mult <= 0))
	{
		return 1;
	}
	item = cJSON_GetObjectItemRecursive(root, name);
	if ((item == NULL) || (item->type != cJSON_Number))
	{
		return 1;
	}
	double_value = item->valuedouble * (double)mult;
	if (!(double_value >= -2147483648.0) || !(double_value <= 2147483647.0))
	{
		return 1;
	}
	*value = (int32_t)double_value;
	return 0;
}

/*
 * 函数名称: cJSON_FindNumberU32
 * 功能说明: 查找无符号 32 位数字字段，按 mult 放大后输出，并拒绝负数。
 * 输入参数: root - cJSON 根节点。name - 字段名。value - 输出值。mult - 放大倍率，必须大于 0。
 * 返 回 值: 0 表示成功，1 表示字段不存在、类型错误或越界。
 */
uint8_t cJSON_FindNumberU32(cJSON *root, const char *name, uint32_t *value, int32_t mult)
{
	cJSON *item;
	double double_value;

	if ((value == NULL) || (name == NULL) || (mult <= 0))
	{
		return 1;
	}
	item = cJSON_GetObjectItemRecursive(root, name);
	if ((item == NULL) || (item->type != cJSON_Number))
	{
		return 1;
	}
	double_value = item->valuedouble * (double)mult;
	if (!(double_value >= 0.0) || !(double_value <= 4294967295.0))
	{
		return 1;
	}
	*value = (uint32_t)double_value;
	return 0;
}

/*
 * 函数名称: cJSON_FindNumberI64
 * 功能说明: 查找有符号 64 位数字字段，按 mult 放大后输出。
 * 输入参数: root - cJSON 根节点。name - 字段名。value - 输出值。mult - 放大倍率，必须大于 0。
 * 返 回 值: 0 表示成功，1 表示字段不存在、类型错误或越界。
 */
uint8_t cJSON_FindNumberI64(cJSON *root, const char *name, int64_t *value, int32_t mult)
{
	cJSON *item;
	double double_value;

	if ((value == NULL) || (name == NULL) || (mult <= 0))
	{
		return 1;
	}
	item = cJSON_GetObjectItemRecursive(root, name);
	if ((item == NULL) || (item->type != cJSON_Number))
	{
		return 1;
	}
	double_value = item->valuedouble * (double)mult;
	if (!(double_value >= -9223372036854775808.0) || !(double_value < 9223372036854775808.0))
	{
		return 1;
	}
	*value = (int64_t)double_value;
	return 0;
}

const uint8_t ascii_255[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	 0,	 0,	 0,	 0,	 0,	 0, 0, 0, 0, 0, 0,	0,	0,	0,	0,	0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,	 7,	 8,	 9,	 0,	 0,	 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0,	0,	0,	0,	0,	0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	 0,	 0,	 0,	 0,	 0,	 0, 0, 0, 0, 0, 0,	0,	0,	0,	0,	0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	 0,	 0,	 0,	 0,	 0,	 0, 0, 0, 0, 0, 0,	0,	0,	0,	0,	0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	 0,	 0,	 0,	 0,	 0,	 0, 0, 0, 0, 0, 0,	0,	0,	0,	0,	0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
