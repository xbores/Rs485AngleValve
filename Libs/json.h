#ifndef _JSON_H_
#define _JSON_H_

#include "bsp.h"
#include <stdio.h>

typedef struct cJSON cJSON;

/* ASCII 到十六进制数值的快速映射表：'0'~'9'/'A'~'F'/'a'~'f' 有效，其余为 0。 */
extern const uint8_t ascii_255[];

/* 递归查找任意层级的 JSON 字段，找到返回节点指针，失败返回 NULL。 */
cJSON *cJSON_GetObjectItemRecursive(cJSON *root, const char *name);

/* 查找形如 prefix + 数字通道号 的对象/数字/字符串节点，例如 ChannelParamSet003。 */
cJSON *cJSON_GetObjectItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max);
cJSON *cJSON_GetNumberItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max);
cJSON *cJSON_GetStringItemWithChannel(cJSON *root, const char *prefix, uint32_t *channel, uint32_t max);

/* 查找并缩放数字字段。返回 0 表示成功，返回 1 表示字段不存在、类型不对或越界。 */
uint8_t cJSON_FindNumberI32(cJSON *root, const char *name, int32_t *value, int32_t mult);
uint8_t cJSON_FindNumberU32(cJSON *root, const char *name, uint32_t *value, int32_t mult);
uint8_t cJSON_FindNumberI64(cJSON *root, const char *name, int64_t *value, int32_t mult);

#endif
