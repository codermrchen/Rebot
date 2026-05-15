/******************************************************************************
 * @brief    系统配置
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-21     Morro        Initial version
 ******************************************************************************/
//#include "sys_util.h"
#include "platform.h"
#include "sys_cfg.h"
#include "cli.h"
#include "lfs.h"

#include "hal_eprom_24clxx.h"

#define CFG_FILE_PATHFMT           "data_file_%d.bin"

typedef struct {
    void    *peprom;
    void    *pflash;
    void    *hfs;
    uint32_t file_init_flag;
} st_cfg_manager;

/* Private varible declaration -----------------------------------------------*/
static st_cfg_manager _cfg_data_mgr = {
                .file_init_flag = 0,
                .peprom = NULL,
                .pflash = NULL,
                .hfs    = NULL,
            };

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static lfs_file_t* _FS_file_get(lfs_t *hfs, uint8_t index);
extern int lfs_flash_init(struct lfs_config *c, void *pdev);
extern int lfs_erase_flashchip(void);

static lfs_file_t* _FS_file_get(lfs_t *hfs, uint8_t index)
{
    static lfs_file_t _cfg_file[CFG_FILE_MAXNUM];

    if (!hfs || CFG_FILE_MAXNUM < index) {
        return NULL;
    }

    if (!(_cfg_data_mgr.file_init_flag & (0x1 << index))) {
        char fpath[CFG_PATH_MAXLEN];
        int err = 0;

        sprintf(fpath, CFG_FILE_PATHFMT, index);
        err = lfs_file_open(hfs, &_cfg_file[index], fpath, (LFS_O_CREAT | LFS_O_RDWR));
        if (0 != err) {
            return NULL;
        }

        _cfg_data_mgr.file_init_flag |= (0x1 << index);
    }
    else {
        lfs_file_close(hfs, &_cfg_file[index]);
        _cfg_data_mgr.file_init_flag &= ~(0x1 << index);
    }
    return &_cfg_file[index];
}


static int _FS_file_write(uint8_t index, uint32_t addr, void *pdata, uint16_t num)
{
    lfs_t *hfs = _cfg_data_mgr.hfs;
    lfs_file_t *pfile = _FS_file_get(hfs, index);

    if (NULL != pfile) {
        if (lfs_file_size(hfs, pfile) < addr) {
            lfs_file_seek(hfs, pfile, 0, LFS_SEEK_END);
        }
        else {
            lfs_file_seek(hfs, pfile, addr, LFS_SEEK_SET);
        }

        num = lfs_file_write(hfs, pfile, pdata, num);
        _FS_file_get(hfs, index);
        return num;
    }
    return -1;
}

static int _FS_file_read(uint8_t index, uint32_t addr, void *pdata, uint16_t num)
{
    lfs_t *hfs = _cfg_data_mgr.hfs;
    lfs_file_t *pfile = _FS_file_get(hfs, index);

    if (NULL != pfile) {
        if (lfs_file_size(hfs, pfile) < (addr + num)) {
            _FS_file_get(hfs, index);
            return -1;
        }

        lfs_file_seek(hfs, pfile, addr, LFS_SEEK_SET);
        num = lfs_file_read(hfs, pfile, pdata, num);
        _FS_file_get(hfs, index);
        return num;
    }
    return -1;
}

// configuration of the filesystem is provided by this struct
static int _FS_init(lfs_t *hfs)
{
    static struct lfs_config cfg;

#if defined(USING_SFUD)
    int err = lfs_flash_init(&cfg, NULL);

#elif defined(TGT_FLASH_CFG)
    st_FLASH_info flash_cfg = TGT_FLASH_CFG;

    int err = FLASH_init(&flash_cfg, NULL);
#else
    return -1;
#endif

    if (err) {
        return err;
    }
	gLfsDevice = &cfg;

//    if (!hfs) return -1;
//    lfs_unmount(hfs);
//    // mount the filesystem
//    err = lfs_mount(hfs, &cfg);
//    // reformat if we can't mount the filesystem
//    // this should only happen on the first boot
//    if (0 != err) {
//        lfs_erase_flashchip();
//        err = lfs_format(hfs, &cfg);
//        if (0 != err) {
//            return err;
//        }
//
//        err = lfs_mount(hfs, &cfg);
//    }

    return err;
}

static inline int _FLASH_data_write(uint32_t addr, void *pdata, uint16_t num)
{
    return -1;
}

static inline int _FLASH_data_read(uint32_t addr, void *pdata, uint16_t num)
{
    return -1;
}

static inline int _EPROM_data_write(uint32_t addr, void *pdata, uint16_t num)
{
    int ret = EPROM_24CLXX_write(_cfg_data_mgr.peprom, addr, pdata, num);

    return ret;
}

static inline int _EPROM_data_read(uint32_t addr, void *pdata, uint16_t num)
{
    int ret = EPROM_24CLXX_read(_cfg_data_mgr.peprom, addr, pdata, num);

    return ret;
}

static inline int _MEMORY_data_write(uint32_t addr, void *pdata, uint16_t num)
{
    return -1;
}

static inline int _MEMORY_data_read(uint32_t addr, void *pdata, uint16_t num)
{
    return -1;
}

static inline int _cfg_data_write(uint8_t type, uint32_t offset, uint8_t *pdata, uint16_t len)
{
    switch (type) {
        case DATA_TYPE_FILE:
        case DATA_TYPE_FILE2:
        case DATA_TYPE_FILE3:
        case DATA_TYPE_FILE4:
        case DATA_TYPE_FILE5:
            type -= DATA_TYPE_FILE;
            return _FS_file_write(type, offset, pdata, len);
        case DATA_TYPE_FLASH:
            return _FLASH_data_write(offset, pdata, len);
        case DATA_TYPE_EPROM:
            return _EPROM_data_write(offset, pdata, len);
        case DATA_TYPE_MEMORY:
            return _MEMORY_data_write(offset, pdata, len);
        default:
            break;
    }
    return -1;
}

uint8_t sys_tolower(uint8_t c)
{
    if ('A' <= c && 'Z' >= c) {
        return c + 32;
    }
    return c;
}

int sys_memcmp(uint8_t *pdst, uint8_t *psrc, uint16_t num)
{
    if (NULL != pdst && NULL != psrc) {
        short pos = 0;

        if (SYS_NET_BYTE == (num & SYS_NET_BYTE)) {
            num &= ~SYS_NET_BYTE;
            for (pos = 0; 0 < num && sys_tolower(pdst[pos]) == sys_tolower(psrc[--num]); pos++);
        }
        else {
            for (pos = 0; 0 < num && sys_tolower(pdst[pos]) == sys_tolower(psrc[pos]); pos++, num--);
        }

        return num;
    }
    return -1;
}

int sys_memcpy(uint8_t *pdst, uint8_t *psrc, uint16_t num)
{
    if (NULL != pdst && NULL != psrc && 0 < num) {
        short pos = 0;

        if (SYS_NET_BYTE == (num & SYS_NET_BYTE)) {
            num &= ~SYS_NET_BYTE;
            for (pos = 0; 0 < num; pos++) {
                pdst[pos] = psrc[--num];
            }
        }
        else {
            for (pos = 0; 0 < num; pos++, num--) {
                pdst[pos] = psrc[pos];
            }
        }
    }
    return num;
}

char *sys_strstr(char *str1, char *str2)
{
    char *p1, *p2, *str;
    char ch1, ch2;

    if (!*str2) {
        return (str1);
    }

    for (str = str1; *str; str++) {
        for (p1 = str, p2 = (char *)str2; *p1 && *p2; p1++, p2++) {
            ch1 = *p1; ch2 = *p2;
            if ('A' <= ch1 && 'Z' >= ch1) {
                ch1 += 32;
            }

            if ('A' <= ch2 && 'Z' >= ch2) {
                ch2 += 32;
            }

            if (ch1 != ch2) {
                break;
            }
        }

        if (!*p2) {
            return(p1);
        }
    }
    return(0);
}

// 查找字符的函数
int sys_strchr(const char *str, char c)
{
    int i;

    // 遍历字符串直到遇到结束符 '\0'
    for (i = 0; '\0' != str[i]; i++) {
        if (c == str[i]) { // 如果找到目标字符
            return i;
        }
    }
    return -1;
}

int sys_str2float(const char *str, float *pval)
{
    double result = 0.0;
    double fraction = 0.1;
    int sign = 1;

    if (!str || '\0' == *str || !pval) {
        return -1;
    }

    *pval = 0.0;
    if ('-' == *str) { // 处理符号位
        sign = -1;
        str ++;
    } else if ('+' == *str) {
        str ++;
    }

    // 处理整数部分
    for (; '\0' != *str && '.' != *str && 'e' != *str && 'E' != *str; str++) {
        if ('0' <= *str && '9' >= *str) {
            result = result * 10 + (*str - '0');
        } else { // 非法字符
            return -1;
        }
    }

    // 处理小数部分
    if ('.' == *str++) {
        for (; '\0' != *str && 'e' != *str && 'E' != *str; str++) {
            if ('0' <= *str && '9' >= *str) {
                result = result + (*str - '0') * fraction;
                fraction *= 0.1;
            } else { // 非法字符
                return -1;
            }
        }
    }

    // 处理指数部分
    if ('e' == *str || 'E' == *str) {
        int exponent = 0;
        int expSign = 1;

        str ++;
        if ('-' == *str) {  // 处理指数符号
            expSign = -1;
            str ++;
        } else if ('+' == *str) {
            str ++;
        }

        for (; '\0' != *str; str++) { // 处理指数值
            if ('0' <= *str && '9' >= *str) {
                exponent = exponent * 10 + (*str - '0');
            } else { // 非法字符
                return -1;
            }
        }

        // 根据指数值调整结果
        while (0 < exponent) {
            if (1 == expSign) {
                result *= 10;
                exponent --;
            } else {
                result *= 0.1;
                exponent --;
            }
        }
    }

    // 返回最终结果
    *pval = result * sign;
    return 0;
}

/**
 * @brief Convert IP string into IP numeric
 * @param[in]   s - 待转换串
 * @param[in]   n - 转换后数字
 * @return      转换结果
 */
int sys_str2ipnum(const char *s, int *n)
{
    unsigned char *pn = (unsigned char *)n;
    int isnumeric = 1;
    int i = sizeof(int) - 1;

    for (*n = 0; (*s) && (isnumeric); s++)  {
      if ('0' <= *s && '9' >= *s) {
          pn[i] = pn[i] * 10 + (*s  - '0');  // big-endian
      }
      else if ('.' == *s) {
           if (0 == i--) {
               isnumeric = 0;
           }
       }
       else if (' ' != *s) { // remove space
          isnumeric = 0;
       }
    }

    if (!isnumeric || 0 != i)  {
        return -1; // error
    }
    return 0; // successful
}

int sys_str2num(const char *s, int * n)
{
	uint16_t usNum = strlen(s);
	int num = 0;
	
	for(uint16_t i = 0; i < usNum; i++)
	{
		if(s[i] >= '0' && s[i] <= '9')
			num = num * 10 + s[i] - '0';
		else
			return -1;
	}
	*n = num;
	return 0;
}

int sys_hexstr2num(const char *s, int * n)
{
	uint16_t usNum = strlen(s);
	int num = 0;
	
	for(uint16_t i = 0; i < usNum; i++)
	{
		if(s[i] >= '0' && s[i] <= '9')
			num = num * 0x10 + s[i] - '0';
		else if(s[i] >= 'A' && s[i] <= 'F')
			num = num * 0x10 + s[i] - 'A' + 10;
		else if(s[i] >= 'a' && s[1] <= 'f')
			num = num * 0x10 + s[i] - 'a' + 10;
		else
			return -1;
	}
	*n = num;
	return 0;
}


int sys_str2byte(char *str, unsigned char *pdata, char fmt, unsigned char unit, unsigned short num)
{
    uint8_t len = 0;

    if (NULL != str && NULL != pdata && 0 < num) {
        char strfmt[5] = {'%', '0', 0x30 + unit, fmt, 0};
        uint8_t n;

        for (n = 0; '\0' != str[len] && num > n; n++, len += unit, pdata++) {
            if (0 == sscanf(&str[len], strfmt, pdata)) {//To return 1 if is one val
                break;
            }
        }
    }
    return len;
}

/**
 * @brief       Data convert to string
 * @return      none
 **/
int sys_byte2str(char *str, unsigned char *pdata, char *pfmt, unsigned short num)
{
    int len = 0;

    if (NULL != str && NULL != pdata && NULL!= pfmt) {
        uint8_t off;
        int i;

        for (i = 0; num > i; i++, len += off) {
            off = sprintf(&str[len], pfmt, pdata[i]);
            if (!off) {
                break;
            }
        }
    }
    return len;
}

int sys_str2hex(char *str, uint8_t *pdata, uint16_t num)
{
    if (NULL != str && NULL != pdata) {
        int i;

        for (i = 0, pdata--; '\0' != str[i] && 0 < num; i++) {
            if (' ' == str[i]) continue;
            if (0x1 & i) {
                *pdata <<= 4;
                num --;
            }
            else {
                pdata ++;
                *pdata = 0;
            }

            if ('F' >= str[i] && 'A' <= str[i]) {
                *pdata |= (0xa + (str[i] - 'A'));
            }
            else if ('f' >= str[i] && 'a' <= str[i]) {
                *pdata |= (0xa + (str[i] - 'a'));
            }
            else if ('9' >= str[i] && '0' <= str[i]) {
                *pdata |= (str[i] - '0');
            }
            else {
                break;
            }
        }

        return i;
    }
    return -1;
}

/**
 * @brief       Data convert to string
 * @return      none
 **/
int sys_hex2str(char *str, uint8_t *pdata, uint16_t num)
{
    if (NULL != str && NULL != pdata) {
        uint8_t hex = ('a' - 0xA);
        int i;

        num <<= 1;
        for (i = 0; num > i; i++, pdata++) {
            str[i] = (*pdata >> 4);
            if (0x9 < str[i]) {
                str[i] += hex;
            }
            else {
                str[i] += '0';
            }

            str[++i] = (*pdata & 0xF);
            if (0x9 < str[i]) {
                str[i] += hex;
            }
            else {
                str[i] += '0';
            }
        }

        str[i] = '\0';
        return i;
    }
    return -1;
}

/** breif 十六进制转换为BCD码
 * 功能：将十六进制转换为BCD码
 * 传入参数：unsigned char类型的数组，数组最大只能有4个字节
 * 说明：可转换的十进制数范围为：0~2576980377，即BCD码的99 99 99 99；
 * 因为我这里使用的是四个字节表示BCD码，数据再大就需要再增加BCD码字节数了
 * 因为定义的unsigned int类型的取值范围为：0~4294967295，理论上应该也是能达到这么大的，如果BCD码有5个字节显示的话
**/
uint32_t sys_hex2bcd(uint32_t val, uint8_t num)
{
    unsigned char *pdata = (unsigned char *)&val;
    unsigned int nshift = 0;
    unsigned char ntmp;
    unsigned char i;

    for (ntmp = 0, i = 0; num > i && 4 > i; i++, nshift += 8) {
        ntmp |= (((pdata[i] / 10) << 4)
                | (pdata[i] % 10)) << nshift;
    }
    return ntmp;
}

uint32_t sys_bcd2hex(uint32_t val, uint8_t num)
{
    unsigned char *pdata = (unsigned char *)&val;
    unsigned int nbase = 1;
    unsigned char ntmp;
    unsigned char i;

    for (ntmp = 0, i = 0; num > i && 4 > i; i++, nbase *= 100) {
        ntmp += (((pdata[i] >> 4) * 10)
                + (pdata[i] & 0x0F)) * nbase;
    }
    return ntmp;
}

int sys_num2ascii(uint32_t val, uint8_t *pdata, uint8_t num)
{
    if (NULL != pdata && 0 < num) {
        unsigned char *ptemp = (unsigned char *)&val;
        unsigned char i, j;

        for (i = 0, j = 0; sizeof(val) > i && num > j; i++) {
            pdata[j]  = ptemp[i] >> 4;
            pdata[j] += (9 < pdata[j]) ? 'A' : '0'; j++;

            pdata[j]  = ptemp[i] & 0xF;
            pdata[j] += (9 < pdata[j]) ? 'A' : '0'; j++;
        }
        return j;
    }
    return 0;
}

int sys_num2byte(uint32_t val, uint8_t *pdata, uint8_t num)
{
    if (NULL != pdata && 0 < num) {
        unsigned char *ptemp = (unsigned char *)&val;
        unsigned char i, j;

        if (sizeof(val) < num) {
            num = sizeof(val);
        }

        for (i = 0, j = num - 1; num > i; i++, j--) {
            pdata[i] = ptemp[j];
        }

        return i;
    }
    return 0;
}

// htons/htonl
int sys_num2hbyte(uint32_t val, uint8_t *pdata, uint8_t num)
{
    if (NULL != pdata && 0 < num) {
        unsigned char *ptemp = (unsigned char *)&val;
        unsigned char i;

        if (sizeof(val) < num) {
            num = sizeof(val);
        }

        for (i = 0; num > i; i++) {
            pdata[i] = ptemp[i];
        }

        return i;
    }
    return 0;
}

int sys_hex2ascii(uint8_t * Out, uint8_t *Input, int num)
{
	if((NULL != Out) && (NULL != Input) && (num > 0) && (num % 2 == 0))
	{
		for(int i = 0; i < num; i += 2)
			sscanf((const char *)&Input[i], "%2x", (unsigned int *)&Out[i/2]);
		return num / 2;
	}
	return 0;
}

char sys_ascii2num(unsigned char ucAscii)
{
	char num;
	if((ucAscii >= '0') && (ucAscii <= '9'))
		num = ucAscii - '0';
	else if((ucAscii >= 'A') && (ucAscii <= 'F'))
		num = ucAscii - 'A' + 10;
	else if((ucAscii >= 'a') && (ucAscii <= 'F'))
		num = ucAscii - 'a' + 10;
	else
		num = (char)-1;
	return num;
}


uint32_t sys_byte2num(uint8_t *pdata, uint16_t num)
{
    uint8_t net_flag = 0;
    unsigned int data = 0;

    if ((num & SYS_NET_BYTE)) {
        num &= ~SYS_NET_BYTE;
        net_flag = 1;
    }

    if (NULL != pdata && 0 < num) {
        unsigned short i;

        if (!net_flag) {
            for (i = 0; num > i && sizeof(data) > i; i++) {
                data = ((data << 8) | pdata[i]);
            }
        }
        else {
            for (i = 0; num > i && sizeof(data) > i; i++) {
                data |= (pdata[i] << (i * 8));
            }
        }
    }

    return data;
}

/**
 * @brief       data reverse
 * @return      none
 **/
int sys_data_hton(uint8_t *pdata, uint16_t num)
{
    if (0 < num && sizeof(uint32_t) >= num) {
        uint8_t ntmp;
        int16_t i, j;

        for (i = num - 1, j = 0; i > j; i--, j++) {
            ntmp = pdata[j];
            pdata[j] = pdata[i];
            pdata[i] = ntmp;
        }

        return num;
    }
    return 0;
}

/**
 * @brief       Notify process
 * @return      none
 **/
int sys_urcProc(uint8_t module, uint16_t type, void *pdata, uint16_t len)
{
    static const st_sys_urc urc_tbl_start SECTION("_section.sys.urc.0") = { 0, NULL };
    static const st_sys_urc urc_tbl_end SECTION("_section.sys.urc.2") = { 0, NULL };
    const st_sys_urc *it;

    for (it =  &urc_tbl_start + 1; it < &urc_tbl_end; it++) {
        if (module == it->module && it->handler) {
            return it->handler(type, pdata, len);
        }
    }
    return -1;
}

/** brief
 ** Little endition storage
 ** if isn't enough, before padding zero
 **/
int sys_cfg_write(uint32_t addr, uint8_t *pdata, uint16_t len)
{
    st_cfg_addr *paddr = (st_cfg_addr *)&addr;
    uint32_t offset = paddr->base;

    if (CFG_DATA_BIGEND & len) {
        sys_data_hton(pdata, len);
        len &= ~CFG_DATA_BIGEND;
    }

    if (0 < paddr->size) {
        if (!pdata || !len) len = 0;
        if (paddr->size > len) {
            uint8_t tmp_buf[paddr->size];
            int i, j;

            if (0 != paddr->bytes) { // write byte stream, msb -> lsb
                for (i = 0, j = len; paddr->size > j; tmp_buf[i++] = 0x0, j++);
                for (j = 0; len > j; tmp_buf[i++] = pdata[j++]);
            } else { // write string, lsb -> msb
                uint8_t dummy = (!(0x80 & pdata[len - 1]) ? 0x0 : 0xFF);

                for (i = 0, j = len; j > i; tmp_buf[i] = pdata[i], i++);
                for (; paddr->size > i; tmp_buf[i++] = dummy);
            }

            return _cfg_data_write(paddr->type, offset, tmp_buf, len);
        }
        else {
            if (0 != paddr->bytes) {
                pdata += len - paddr->size;
            }

            len = paddr->size;
        }
    }

    return _cfg_data_write(paddr->type, offset, pdata, len);
}

int sys_cfg_read(uint32_t addr, uint8_t *pdata, uint16_t len)
{
    if (NULL != pdata && 0 < len) {
        st_cfg_addr *paddr = (st_cfg_addr *)&addr;
        uint32_t offset = paddr->base;
        uint8_t *pbuf = pdata;
        uint8_t isreverse = 0;
        int num = len;
        int ret;

        if (CFG_DATA_BIGEND & len) {
            len &= ~CFG_DATA_BIGEND;
            isreverse = 1;
        }

        if (0 < paddr->size) {
            if (paddr->size < len) {
                num = paddr->size;
            }
            else if (0 != paddr->bytes) { // read byte stream, msb -> lsb
                offset += (paddr->size - len);
            }
        }

        switch (paddr->type) {
        case DATA_TYPE_FILE:
        case DATA_TYPE_FILE2:
        case DATA_TYPE_FILE3:
        case DATA_TYPE_FILE4:
        case DATA_TYPE_FILE5: {
            uint8_t i = (paddr->type - DATA_TYPE_FILE);

            ret = _FS_file_read(i, offset, pbuf, num);
            break;
        }
        case DATA_TYPE_FLASH:
            ret = _FLASH_data_read(offset, pbuf, num);
            break;
        case DATA_TYPE_EPROM:
            ret = _EPROM_data_read(offset, pbuf, num);
            break;
        case DATA_TYPE_MEMORY:
            ret = _MEMORY_data_read(offset, pbuf, num);
            break;
        default:
            return -1;
        }

        if (num != ret) {
            return -1;
        }

        if (num < len) {
            if (0 != paddr->bytes) { // read byte stream, msb -> lsb
                for (; len > num++; *pdata++ = 0x0);
                isreverse = 0;
            }
            else { // read string, lsb -> msb
                uint8_t dummy = (!(0x80 & pdata[num - 1]) ? 0x0 : 0xFF);

                for (; len > num; pdata[num++] = dummy);
            }
        }

        if (0 != isreverse) {
            sys_data_hton(pdata, len);
        }

        return len;
    }
    return -1;
}

/** brief
 ** memory item: data/array
 ** eprom/flash item: data/array + address
 ** file item: array + bit_map
 **/
int sys_cfg_init(void)
{
#if defined(USING_LITTLEFS)
    if (!_cfg_data_mgr.hfs) {
        static lfs_t _fs_obj;

        if (0 > _FS_init(&_fs_obj)) {
            //error
        }
        else {
            _cfg_data_mgr.hfs = &_fs_obj;
        }
    }
#elif defined(TGT_FLASH_CFG)
    if (!_cfg_data_mgr.pflash) {
        static st_FLASH_info _flash_info = TGT_FLASH_CFG;

        if (0 > FLASH_init(&_flash_info, NULL)) {
            // error
        }
        else {
            _cfg_data_mgr.pflash = &_flash_info;
        }
    }

#endif

#ifdef TGT_EPROM_CFG
    if (!_cfg_data_mgr.peprom) {
        static st_EPROM_info _eprom_info = TGT_EPROM_CFG;

        if (0 > EPROM_24CLXX_init(&_eprom_info)) {
            // error
        }
        else {
            _cfg_data_mgr.peprom = &_eprom_info;
        }
    }
#endif
    return 0;
}
