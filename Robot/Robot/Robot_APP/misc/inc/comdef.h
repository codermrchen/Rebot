/******************************************************************************
 * @brief    通用宏定义
 *
 * Copyright (c) 2018~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-01     Morro        Initial version.
 ******************************************************************************/

#ifndef _COM_DEF_H_
#define _COM_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define STR(x)          #x   //To string
#define CONTACT(x,y)    x##y //拼接

//二级
#define STR2(x)         STR(x)
#define CONTACT2(x,y)   CONTACT(x,y)

/*匿名类型定义 -----------------------------------------------------------*/
#define  ANONY_CONN(type, var, line)    type  var##line
#define  ANONY_DEF(type, prefix, line)  ANONY_CONN(type, prefix, line)
#define  ANONY_TYPE(type, prefix)       ANONY_DEF(type, prefix, __LINE__)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#if !defined(__GNUC__)
#define container_of(ptr, type, member) ( \
        (type *)( (char *)(ptr) - offsetof(type,member) ))
#else
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#if defined(__CC_ARM) || defined(__GNUC__) /* ARM,GCC*/
    #define SECTION(x)                  __attribute__((section(x)))
    #define UNUSED                      __attribute__((unused))
    #define USED                        __attribute__((used))
    #define ALIGN(n)                    __attribute__((aligned(n)))
    #define WEAK                        __attribute__((weak))
#elif defined (__ICCARM__)              /*IAR */
    #define SECTION(x)                  @ x
    #define UNUSED
    #define USED                        __root
    #define WEAK                        __weak
#else
    #error "Current tool chain haven't supported yet!"
#endif

/* Array element num ---------------------------------------------------------------*/
#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof(Array[0]))

#ifdef __cplusplus
}
#endif

#endif
