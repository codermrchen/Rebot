#include <string.h>
#include <stdio.h>
#include <time.h>

#define START_YEAR          (2000)

/* 判断是否闰年 */
#define IS_LEAP_YEAR(year)  ((!((year) % 400) || \
    (!((year) % 4) && 0 != ((year) % 100))) ? 1 : 0)

/**
 * @brief 星期的英文简写转换为对应的数字
 *
 * @param str 星期的简写字符串
 * @return int 返回星期对应的数字，-1表示转换失败
 */
int week_abbreviation_to_num(const char *str)
{
    const char *week_str_arr[] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    int i;

    for (i = 0; i < 7; ++i) {
        if (!strcmp(str, week_str_arr[i])) {
            break;
        }
    }

    return (7 <= i ? -1 : i);
}

/**
 * @brief 月份的英文简写转换为对应的数字
 *
 * @param str 月份的简写字符串
 * @return int 返回月份对应的数字(范围0~11)，-1表示转换失败
 */
int month_abbreviation_to_num(const char *str)
{
    const char *month_str_arr[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int i;

    for (i = 0; i < 12; ++i) {
        if (!strcmp(str, month_str_arr[i])) {
            break;
        }
    }

    return (12 <= i ? -1 : i);
}

/**
 * @brief 字符串格式的时间转换为时间戳
 *
 * @param str 要转换的时间字符串,格式为week month day our:minute:second, 例如:
 * "Fri Feb 28 15:58:43 2021"
 * @param t 存放转换结果的time_t变量的地址
 * @return int 0: 转换成功，非0: 转换失败
 */
int strtotime(const char *str, time_t *t)
{
    int year, month, day, hour, minute, second;
    struct tm tm_; /* 定义tm结构体 */

    if (6 != sscanf(str, "%02d%02d%02d%02d%02d%02d",
            &year, &month, &day, &hour, &minute, &second)) {
        return -1;
    }

    /* 年 */
    if (1 > day) return -3;
    switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            if (day > 31) return -3;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            if (day > 30) return -3;
            break;
        case 2:
            if (IS_LEAP_YEAR(year)) {
                if (2 == month && 29 < day) return -3;
            } else {
                if (2 == month && 28 < day) return -3;
            }
            break;
        default: return -4;
    }

    if (0 > hour || 23 < hour) return -5;
    if (0 > minute || 59 < minute) return -6;
    if (0 > second || 59 < second) return -7;

    tm_.tm_hour = hour; /* 时 */
    tm_.tm_min = minute;/* 分 */
    tm_.tm_sec = second;/* 秒 */
    // 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year为int临时变量减去2000
    tm_.tm_year = year + START_YEAR - 1900;
    tm_.tm_mon  = month - 1;/* 月 */
    tm_.tm_mday = day;  /* 日 */
    tm_.tm_isdst = 0;   /* 非夏令时 */
    *t = mktime(&tm_);  /* 将tm结构体转换成time_t格式 */
    return 0;
}
