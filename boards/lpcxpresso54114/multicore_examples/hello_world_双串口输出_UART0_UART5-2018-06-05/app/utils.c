#include "utils.h"
uint8_t checksum(const void *data, int len)
{
    uint8_t cs = 0;

    while (len-- > 0)
        cs += *((uint8_t *) data + len);
    return cs;
}

uint32_t get_le_val(const uint8_t * p, int bytes)//小端，数组到uint32
{
    uint32_t ret = 0;

    while (bytes-- > 0)
    {
        ret <<= 8;
        ret |= *(p + bytes);
    }
    return ret;
}
uint32_t get_be_val(const uint8_t * p, int bytes)
{
    uint32_t ret = 0;
    while (bytes-- > 0)
    {
        ret <<= 8;
        ret |= *p++;
    }

    return ret;
}
void put_le_val(uint32_t val, uint8_t * p, int bytes)//将uint32位的数据转换成uint8型的数组，低字节放在数组中的低地址中。小端
{
    while (bytes-- > 0)
    {
        *p++ = val & 0xFF;
        val >>= 8;
    }
}
void put_be_val(uint32_t val, uint8_t * p, int bytes)//将uint32位的数据转换成uint8型的数组，低字节放在数组中的高地址中。大端
{
    while (bytes-- > 0)
    {
        *(p + bytes) = val & 0xFF;
        val >>= 8;
    }
}

int is_all_xx(const void *s1, uint8_t val, int n)//不相同返回0，相同返回1
{
    while (n && *(uint8_t *) s1 == val)
    {
        s1 = (uint8_t *) s1 + 1;
        n--;
    }
    return !n;
}

void hex2bcd(uint32_t value, uint8_t * bcd, uint8_t bytes)
{
    uint8_t x;

    if (bytes > 5)
    {
        bytes = 5;
    }
    while (bytes--)
    {
        x = value % 100u;
        *bcd = bin2bcd(x);
        bcd++;
        value /= 100u;
    }
}

uint32_t bcd2hex(uint8_t * bcd, uint8_t bytes)
{
    uint32_t ret = 0;

    if (bytes > 4)
    {
        bytes = 4;
    }
    while (bytes-- > 0)
    {
        ret *= 100u;
        ret += bcd2bin(bcd[bytes]);
    }
    return ret;
}
uint32_t xbcd2hex(const uint8_t * bcd, uint8_t bytes)
{
    uint32_t ret = 0;

    if (bytes > 4)
    {
        bytes = 4;
    }
    while (bytes-- > 0)
    {
        ret *= 100u;
        ret += bcd2bin(bcd[bytes]);
    }
    return ret;
}
bool is_all_bcd(const uint8_t *data, size_t n)
{
    while (n--)
    {
        if ((data[n] & 0x0F) > 0x09 || (data[n] & 0xF0) > 0x90)
            return false;
    }
    return true;
}
