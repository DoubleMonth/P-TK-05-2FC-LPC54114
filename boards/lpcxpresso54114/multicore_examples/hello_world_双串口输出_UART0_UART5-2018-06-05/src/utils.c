#include <printk.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <utils.h>

#define OS_TINY_SIZE

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

/**
 * This function will set the content of memory to specified value
 *
 * @param s the address of source memory
 * @param c the value shall be set in content
 * @param count the copied length
 *
 * @return the address of source memory
 */
void *os_memset(void *s, int c, size_t count)
{
#ifdef OS_TINY_SIZE
    char *xs = (char *)s;

    while (count--)
        *xs++ = c;

    return s;
#else
#define LBLOCKSIZE      (sizeof(int32_t))
#define UNALIGNED(X)    ((int32_t)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

    int i;
    char *m = (char *)s;
    uint32_t buffer;
    uint32_t *aligned_addr;
    uint32_t d = c & 0xff;

    if (!TOO_SMALL(count) && !UNALIGNED(s))
    {
        /* If we get this far, we know that n is large and m is word-aligned. */
        aligned_addr = (uint32_t *)s;

        /* Store D into each char sized location in BUFFER so that
         * we can set large blocks quickly.
         */
        if (LBLOCKSIZE == 4)
        {
            buffer = (d << 8) | d;
            buffer |= (buffer << 16);
        }
        else
        {
            buffer = 0;
            for (i = 0; i < LBLOCKSIZE; i ++)
                buffer = (buffer << 8) | d;
        }

        while (count >= LBLOCKSIZE * 4)
        {
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            count -= 4 * LBLOCKSIZE;
        }

        while (count >= LBLOCKSIZE)
        {
            *aligned_addr++ = buffer;
            count -= LBLOCKSIZE;
        }

        /* Pick up the remainder with a bytewise loop. */
        m = (char *)aligned_addr;
    }

    while (count--)
    {
        *m++ = (char)d;
    }

    return s;

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL
#endif
}

/**
 * This function will copy memory content from source address to destination
 * address.
 *
 * @param dst the address of destination memory
 * @param src  the address of source memory
 * @param count the copied length
 *
 * @return the address of destination memory
 */
void *os_memcpy(void *dst, const void *src, size_t count)
{
#ifdef OS_TINY_SIZE
    char *tmp = (char *)dst, *s = (char *)src;

    while (count--)
        *tmp++ = *s++;

    return dst;
#else

#define UNALIGNED(X, Y)                                               \
                        (((int32_t)X & (sizeof(int32_t) - 1)) | \
                         ((int32_t)Y & (sizeof(int32_t) - 1)))
#define BIGBLOCKSIZE    (sizeof(int32_t) << 2)
#define LITTLEBLOCKSIZE (sizeof(int32_t))
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

    char *dst_ptr = (char *)dst;
    char *src_ptr = (char *)src;
    int32_t *aligned_dst;
    int32_t *aligned_src;
    int len = count;

    /* If the size is small, or either SRC or DST is unaligned,
    then punt into the byte copy loop.  This should be rare. */
    if (!TOO_SMALL(len) && !UNALIGNED(src_ptr, dst_ptr))
    {
        aligned_dst = (int32_t *)dst_ptr;
        aligned_src = (int32_t *)src_ptr;

        /* Copy 4X long words at a time if possible. */
        while (len >= BIGBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= BIGBLOCKSIZE;
        }

        /* Copy one long word at a time if possible. */
        while (len >= LITTLEBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            len -= LITTLEBLOCKSIZE;
        }

        /* Pick up any residual with a byte copier. */
        dst_ptr = (char *)aligned_dst;
        src_ptr = (char *)aligned_src;
    }

    while (len--)
        *dst_ptr++ = *src_ptr++;

    return dst;
#undef UNALIGNED
#undef BIGBLOCKSIZE
#undef LITTLEBLOCKSIZE
#undef TOO_SMALL
#endif
}

/**
 * This function will move memory content from source address to destination
 * address.
 *
 * @param dest the address of destination memory
 * @param src  the address of source memory
 * @param n the copied length
 *
 * @return the address of destination memory
 */
void *os_memmove(void *dest, const void *src, size_t n)
{
    char *tmp = (char *)dest, *s = (char *)src;

    if (s < tmp && tmp < s + n)
    {
        tmp += n;
        s += n;

        while (n--)
            *(--tmp) = *(--s);
    }
    else
    {
        while (n--)
            *tmp++ = *s++;
    }

    return dest;
}

/**
 * This function will compare two areas of memory
 *
 * @param cs one area of memory
 * @param ct znother area of memory
 * @param count the size of the area
 *
 * @return the result
 */
int32_t os_memcmp(const void *cs, const void *ct, size_t count)
{
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;

    return res;
}

/**
 * This function will return the first occurrence of a string.
 *
 * @param s1 the source string
 * @param s2 the find string
 *
 * @return the first occurrence of a s2 in s1, or NULL if no found.
 */
char *os_strstr(const char *s1, const char *s2)
{
    int l1, l2;

    l2 = os_strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = os_strlen(s1);
    while (l1 >= l2)
    {
        l1 --;
        if (!os_memcmp(s1, s2, l2))
            return (char *)s1;
        s1 ++;
    }

    return NULL;
}

/**
 * This function will compare two strings while ignoring differences in case
 *
 * @param a the string to be compared
 * @param b the string to be compared
 *
 * @return the result
 */
uint32_t os_strcasecmp(const char *a, const char *b)
{
    int ca, cb;

    do
    {
        ca = *a++ & 0xff;
        cb = *b++ & 0xff;
        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
    }
    while (ca == cb && ca != '\0');

    return ca - cb;
}

/**
 * This function will copy string no more than n bytes.
 *
 * @param dst the string to copy
 * @param src the string to be copied
 * @param n the maximum copied length
 *
 * @return the result
 */
char *os_strncpy(char *dst, const char *src, size_t n)
{
    if (n != 0)
    {
        char *d = dst;
        const char *s = src;

        do
        {
            if ((*d++ = *s++) == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }

    return (dst);
}

/**
 * This function will compare two strings with specified maximum length
 *
 * @param cs the string to be compared
 * @param ct the string to be compared
 * @param count the maximum compare length
 *
 * @return the result
 */
int32_t os_strncmp(const char *cs, const char *ct, size_t count)
{
    register signed char __res = 0;

    while (count)
    {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
        count --;
    }

    return __res;
}

/**
 * This function will compare two strings without specified length
 *
 * @param cs the string to be compared
 * @param ct the string to be compared
 *
 * @return the result
 */
int32_t os_strcmp(const char *cs, const char *ct)
{
    while (*cs && *cs == *ct)
        cs++, ct++;

    return (*cs - *ct);
}

/**
 * The  strnlen()  function  returns the number of characters in the
 * string pointed to by s, excluding the terminating null byte ('\0'), 
 * but at most maxlen.  In doing this, strnlen() looks only at the 
 * first maxlen characters in the string pointed to by s and never 
 * beyond s+maxlen.
 *
 * @param s the string
 * @param maxlen the max size
 * @return the length of string
 */
size_t os_strnlen(const char *s, size_t maxlen)
{
    const char *sc;

    for (sc = s; *sc != '\0' && sc - s < maxlen; ++sc) /* nothing */
        ;

    return sc - s;
}

/**
 * This function will return the length of a string, which terminate will
 * null character.
 *
 * @param s the string
 *
 * @return the length of string
 */
size_t os_strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc) /* nothing */
        ;

    return sc - s;
}

#ifdef OS_USING_HEAP
/**
 * This function will duplicate a string.
 *
 * @param s the string to be duplicated
 *
 * @return the duplicated string pointer
 */
char *os_strdup(const char *s)
{
    size_t len = os_strlen(s) + 1;
    char *tmp = (char *)os_malloc(len);

    if (!tmp)
        return NULL;

    os_memcpy(tmp, s, len);

    return tmp;
}
#endif

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

char *i2str(uint8_t val, char *dest)
{
    const char *charmap = "0123456789ABCDEF";

    *dest++ = charmap[get_bits(val, 4, 7)];
    *dest++ = charmap[get_bits(val, 0, 3)];
    *dest++ = '\0';
    return dest-3;
}
const char *arr2str(const void *arr, int len, void *dest, int maxlen)
{
    const uint8_t *_arr = (const uint8_t *)arr;
    char *_dest = (char *)dest;

    while (len-- && maxlen > 0)
    {
        i2str(*_arr++, _dest);
        _dest += 2;
        *_dest++ = ' ';
        maxlen -= 3;
    }
    if (_dest - (char*)dest > 0) *--_dest = '\0';

    return (const char *)dest;
}
int split(char *str, char **arr, const char *del)
{
    if (!str) return 0;

    char **src = arr;

    for (*arr++ = str; *str; str++)
    {
        if (*str != *del) continue;

        *str = '\0';
        *arr++ = str+1;
    }

    return arr - src;
}

static int get_num(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}
int str2arr(const char *str, void *out, int maxlen)
{
    int i, n = 0;

    while (*str)
    {
        while (*str && !isalnum((int)*str)) str++;

        uint8_t val = 0;
        for (i = 0; i < 2 && isalnum((int)*str); i++)
        {
            val <<= 4;
            val |= get_num(*str++);
        }
        if (i) ((uint8_t*)out)[n++] = val;
        if (n >= maxlen) break;
    }
    return n;
}

void reverse(void *buff, size_t len)
{
    uint8_t *rearp = (uint8_t *) buff + len - 1;
    uint8_t *headp = (uint8_t *) buff;

    while (headp < rearp)
    {
        swap(*headp, *rearp);
        headp++;
        rearp--;
    }
}

uint16_t crc16(uint16_t crc, const void *buf, int size)
{
    uint8_t i;
    uint8_t *_buf = (uint8_t *) buf;

    while (size-- != 0)
    {
        for (i = 0x80; i != 0; i >>= 1)
        {
            if ((crc & 0x8000) != 0)
            {
                crc <<= 1;
                crc ^= 0x1021;
            }
            else
            {
                crc <<= 1;
            }
            if (((*_buf) & i) != 0)
            {
                crc ^= 0x1021;
            }
        }
        _buf++;
    }
    return crc;
}

#ifdef __CC_ARM 
# pragma diag_suppress 1293
#endif
int count_bit_in_long(unsigned long x)
{
    int n = 0;

    if (x)
    {
        do
        {
            n++;
        }
        while (x = x & (x - 1));
    }
    return n;
}

void *memmem(const void *buf1, int len1, const void *buf2, int len2)
{
    int i = 0, j = 0;

    if (len2 > len1) return NULL;

    while (i < len1 && j < len2)
    {
        if (*((uint8_t*)buf1+i) == *((uint8_t*)buf2+j))
        {
            i++; j++;
        }
        else
        {
            i = i - j + 1; j = 0x00;
        }
    }
    if (j >= len2) return (uint8_t*)buf1+(i-j);
    return NULL;
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

bool is_bcd_time_valid(const uint8_t *bcdtime)
{
    int i;
    const uint8_t maxvalues[] = { 0x59, 0x59, 0x23, 0x31, 0x12, 0x99 };

    if (!is_all_bcd(bcdtime, 0x06))
        return false;

    for (i = 0; i < 6; i++)
    {
        if (bcdtime[i] > maxvalues[i])
            return false;
    }
    if (0x00 == bcdtime[3] || 0x00 == bcdtime[4])
        return false;
    return true;
}

uint8_t week(uint8_t y, uint8_t m, uint8_t d)
{
    const uint8_t TAB_X[12] = {6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    if (m < 3 && (y & 0x03) == 0)
    {
        return ((y + (y >> 2) + TAB_X[m - 1] + d - 1) % 7);
    }
    return ((y + (y >> 2) + TAB_X[m - 1] + d) % 7);
}
#define HZ                              (100)
unsigned int jiffies_to_msecs(const unsigned long j)
{
    return 1000 * j / HZ;
}

