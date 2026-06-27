#ifndef _FSTR_H_
#define _FSTR_H_

#include "bsp.h"
#include <stdio.h>

/* 在以 \0 作为有效结束标志的接收缓存中查找字符串，找到返回偏移，失败返回 0xFFFF。 */
uint16_t Findstr(const char *str, const uint8_t *buffer, uint32_t maxlen);

/* 在原始内存中查找字符串，不把 \0 当作缓存结束符，适合二进制帧或未结束缓存。 */
uint16_t FindstrMem(const char *target_str, const uint8_t *target_address, uint32_t len);

/* 检查字符串是否全部为可打印 ASCII，成功返回字符串长度，失败返回 0。 */
uint8_t is_str(const char *str, uint16_t maxlen);

/* 从 ESP 返回报文中提取 Object: value 形式的 value，带输出缓存长度保护。 */
uint16_t ESP_Analysis_StrN(const uint8_t *Buffer, const char *Object, char *str, uint16_t maxlen, uint16_t outmax);
uint16_t ESP_Analysis_Str(const uint8_t *Buffer, const char *Object, char *str, uint16_t maxlen);

void printfhex(const uint8_t *buffer, uint16_t len, const char *frontstr);
int32_t Value_Cmp(int32_t value1, int32_t value2);
uint32_t DecNum_Mult(uint8_t decnum);
void Swap_Byte(uint8_t *a, uint8_t *b);

#endif
