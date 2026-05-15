/******************************************************************************
 * @brief    œµÕ≥≈‰÷√
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-21     Morro        Initial version
 ******************************************************************************/
#include "config.h"
#include "modbus.h"
#include "platform.h"

#define CFG_FILE_INPECT_POINT       DATA_TYPE_FILE
#define CFG_FILE_INPECT_PLAN        DATA_TYPE_FILE2
#define CFG_FILE_INPECT_RECORD      DATA_TYPE_FILE3

static st_eprom_cfg *_pdef_cfg = NULL;

#if (ROBOT_VER == 0)
const char net_dev_sn[14] = "K012410250001";
#else 
const char net_dev_sn[14] = "K012410250002";
#endif

char cloud_client_ID[18] = {0};
char nvidia_client_ID[18] = {0};

static int _sys_state_urc(unsigned short cmd, unsigned char *pdata, unsigned short num)
{
    // index 0 is front, 1 is back; state 0 is open, 1 is close, 9 is error
    static st_dev_state  _hall_state[2] = {
                                [0] = {.state = SYS_STATE_PWRDN},
                                [1] = {.state = SYS_STATE_PWRDN},
                            };
    // index 0 is front, 1 is back; state 0 is open, 1 is close, 9 is error
    static st_dev_state  _led_state[2] = {
                                [0] = {.state = SYS_STATE_PWRDN},
                                [1] = {.state = SYS_STATE_PWRDN},
                            };
    // index 0 is in, 1 is out; state 0 is open, 1 is close, 9 is error
    static st_dev_state  _fan_state[2] = {
                                [0] = {.state = SYS_STATE_PWRDN},
                                [1] = {.state = SYS_STATE_PWRDN},
                            };
    // 0 close, 1 sleep, 2 idle, 3 charging, 4, inspecting, 5 manual, 9 error
    static st_dev_state  _sys_state = {
                                .state = SYS_STATE_PWRDN,
                            };
    static uint32_t _sys_cfg = 0;
    // 0 auto, 1 manual, 2 deploy
    static uint8_t _sys_mode = 0;

    if (NULL != pdata && !(*pdata) && !num) {
        st_cfg_cmd *pcmd = (st_cfg_cmd *)&cmd;
        switch (pcmd->type) {
            case CFG_SYS_MODE:
                *(void **)pdata = &_sys_mode;
                return sizeof(_sys_mode);
            case CFG_SYS_CFG:
                *(void **)pdata = (uint32_t **)&_sys_cfg;
                return sizeof(_sys_cfg);
            case CFG_SYS_ERR:
                *(void **)pdata = &_sys_state.error;
                return sizeof(_sys_state.error);
            case CFG_SYS_STATE:
                *(void **)pdata = &_sys_state.state;
                return sizeof(_sys_state.state);
            case CFG_FAN_ERR:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = &_fan_state[pcmd->index].error;
                return sizeof(_fan_state[pcmd->index].error);
            case CFG_FAN_STATE:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = &_fan_state[pcmd->index].state;
                return sizeof(_fan_state[pcmd->index].state);
            case CFG_LED_ERR:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = &_led_state[pcmd->index].error;
                return sizeof(_led_state[pcmd->index].error);
            case CFG_LED_STATE:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = (void *)&_led_state[pcmd->index].state;
                return sizeof(_led_state[pcmd->index].state);
            case CFG_HALL_ERR:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = (void *)&_hall_state[pcmd->index].error;
                return sizeof(_hall_state[pcmd->index].error);
            case CFG_HALL_STATE:
                if (2 <= pcmd->index) return 0;
                *(void **)pdata = (void *)&_hall_state[pcmd->index].state;
                return sizeof(_hall_state[pcmd->index].state);
            default: break;
        }
    }

    *(void **)pdata = NULL;
    return 0;
}
sys_urc_register(SYS_MODULE_CFG, _sys_state_urc);

static int _cfg_file_dataProc(uint16_t cmd, uint8_t *pdata, uint16_t index, uint16_t len)
{
    static st_bitmap_info *_inspect_point_map = NULL;
    static st_bitmap_info *_inspect_plan_map = NULL;
    static st_bitmap_info *_inspect_record_map = NULL;

    st_cfg_cmd *pcmd = (st_cfg_cmd *)&cmd;
    st_bitmap_info **ppmap;
    st_bitmap_info *pmap;
    st_bitmap_info bitmap;
    uint8_t file_type = DATA_TYPE_FILE;
    uint8_t byte_stream = 0;
    uint32_t base = 0;
    int ret = -1;

    switch (pcmd->type) {
    case CFG_PLACE_ID:
       base = offsetof(st_inspect_point, rfid);
       byte_stream = 0x1; // lsb padding zero
   case CFG_INSPECT_POS:
      if (CFG_INSPECT_POS == pcmd->type) {
        base = offsetof(st_inspect_point, pos);
      }
    case CFG_INSPECT_POINT:
        ppmap = &_inspect_point_map;
        file_type = CFG_FILE_INPECT_POINT;
        bitmap.num = CFG_INSPECT_POINTS;
        bitmap.unit = sizeof(st_inspect_point);
        break;
    case CFG_INSPECT_ID:
       base = offsetof(st_inspect_plan, id);
    case CFG_INSPECT_PLAN:
        ppmap = &_inspect_plan_map;
        file_type = CFG_FILE_INPECT_PLAN;
        bitmap.num = CFG_INSPECT_PLANS;
        bitmap.unit = sizeof(st_inspect_plan);
        break;
    case CFG_INSPECT_RECORD:
        ppmap = &_inspect_record_map;
        file_type = CFG_FILE_INPECT_RECORD;
        bitmap.num = CFG_INSPECT_RECORDS;
        bitmap.unit = sizeof(st_inspect_record);
        break;
    default: return -1;
    }

    bitmap.size = CFG_BITMAP_SIZE(bitmap.num);
    if (NULL != (*ppmap)) {
        pmap = *ppmap;
    }
    else {
        pmap = sys_malloc(bitmap.size);
        if (NULL != pmap) {
            *ppmap = pmap;
            if (bitmap.size == sys_cfg_read(
                CFG_DATA_ADDR(0, file_type, byte_stream, 0), (void *)pmap, bitmap.size)
                && !sys_memcmp((void *)&bitmap, (void *)pmap, sizeof(bitmap))) {
                // error
            }
            else {
                ret = 0;
            }
        }
        else {
            return -1;
        }
    }

    if (0xFFFF == index) {
        if (0 != pcmd->read) {
            return -1;
        }

        for (index = 0; bitmap.num > index; index++) {
            if (!(pmap->bit_map[index >> 3] & (0x1 << (index & 0x7)))) {
                break;
            }
        }

        if (bitmap.num == index) {
            return -1;
        }
    }

    base += pmap->size + pmap->unit * index;
    if (!pcmd->read || !ret) { // write
        if (0 > ret) {
            ret = sys_cfg_write(CFG_DATA_ADDR(base, file_type, byte_stream, 0), pdata, len);
            if (ret != len) return -1;
            if (!pdata && !len) {
                (pmap)->bit_map[index >> 3] &= ~(0x1 << (index & 0x7));
            }
            else {
                (pmap)->bit_map[index >> 3] |= (0x1 << (index & 0x7));
            }
        }
        else {
            sys_memcpy((void *)pmap, (void *)&bitmap, sizeof(bitmap));
            sys_memset(pmap->bit_map, 0, bitmap.size - sizeof(bitmap));
        }

        ret = sys_cfg_write(CFG_DATA_ADDR(0, file_type, byte_stream, 0), (void *)pmap, bitmap.size);
    }
    else if (pmap->bit_map[index >> 3] &(0x1 << (index & 0x7))) { // read
        ret = sys_cfg_read(CFG_DATA_ADDR(base, file_type, byte_stream, 0), pdata, len);
    }
    else {
        for (; ++index < bitmap.num; ) {
            if (pmap->bit_map[index >> 3] &(0x1 << (index & 0x7))) {
                return 0;
            }
        }
    }

    return ret;
}

static uint32_t _cfg_eprom_dataAddr(uint8_t type, uint16_t index)
{
    switch (type) {
        case CFG_CHARGE_WARNTEMP: // EPROM
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, charge_warntemp), DATA_TYPE_EPROM, 0, sizeof(uint8_t));
        case CFG_CHARGE_WARNBAT:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, charge_warnbat), DATA_TYPE_EPROM, 0, sizeof(uint8_t));
        case CFG_LOCAL_INFO:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, local_info), DATA_TYPE_EPROM, 0, sizeof(st_NET_info));
        case CFG_LOCAL_PORT:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, local_info) +
                offsetof(st_NET_info, port), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_LOCAL_IP:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, local_info) +
                offsetof(st_NET_info, ip), DATA_TYPE_EPROM, 0, sizeof(uint32_t));
        case CFG_LOCAL_GW:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, local_info) +
                offsetof(st_NET_info, gw), DATA_TYPE_EPROM, 0, sizeof(uint32_t));
        case CFG_LOCAL_SN:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, local_info) +
                offsetof(st_NET_info, sn), DATA_TYPE_EPROM, 0, sizeof(uint32_t));
        case CFG_NVIDIA_INFO:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, nvidia_info), DATA_TYPE_EPROM, 0, sizeof(st_SVR_info));
        case CFG_NVIDIA_PORT:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, nvidia_info) +
                offsetof(st_SVR_info, port), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_NVIDIA_HOST:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, nvidia_info) +
                offsetof(st_SVR_info, host), DATA_TYPE_EPROM, 0, CFG_NAME_MAXLEN);
        case CFG_CLOUD_INFO:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, cloud_info), DATA_TYPE_EPROM, 0, sizeof(st_SVR_info));
        case CFG_CLOUD_PORT:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, cloud_info) +
                offsetof(st_SVR_info, port), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_CLOUD_HOST:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, cloud_info) +
                offsetof(st_SVR_info, host), DATA_TYPE_EPROM, 0, CFG_NAME_MAXLEN);
        case CFG_CLOUD_PKTSER:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, pkt_ser), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_WALK_CYCLE:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, walk_cycle), DATA_TYPE_EPROM, 0, sizeof(float));
        case CFG_WALK_SPEED:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, walk_speed), DATA_TYPE_EPROM, 0, 3);
        case CFG_WALK_POS:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, walk_curpos), DATA_TYPE_EPROM, 0, 3);
        case CFG_LIFT_HIGH:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, lift_curpos), DATA_TYPE_EPROM, 0, 3);
        case CFG_LIFT_MAX:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, lift_max), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_LIFT_MIN:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, lift_min), DATA_TYPE_EPROM, 0, sizeof(uint16_t));
        case CFG_SYS_SN:
            return CFG_DATA_ADDR(offsetof(st_eprom_cfg, dev_sn), DATA_TYPE_EPROM, 0, sizeof(st_dev_sn));
        default: break;
    }

    return CFG_INVALID_ADDR;
}

int _cfg_mem_ref(uint8_t **ppdata, uint8_t type, uint16_t index)
{
    switch (type) { // memory
//        case CFG_RFID_EPC: // 0x7
//        case CFG_RFID_POS:
//        case CFG_RFID_ERR:
//        case CFG_RFID_STATE:
//        case CFG_RFID_TIME:
//            return CFG_DATA_BIGEND |
//                sys_urcProc(SYS_MODULE_RFID, CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        case CFG_ULTRA_ADC:
            return sys_urcProc(SYS_MODULE_ULTRA, CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        case CFG_BAT_CAPACITY: // precision 2
        case CFG_BAT_TEMP: // precision 2
        case CFG_BAT_STATE:
        case CFG_BAT_ERR:
        case CFG_CHARGE_CURRENT: // precision 3
        case CFG_CHARGE_VOLTAGE: // precision 2
        case CFG_CHARGE_TEMP: // precision 2
        case CFG_CHARGE_STATE:
        case CFG_CHARGE_ERR:
            return sys_urcProc(SYS_MODULE_CHARGE, CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        case CFG_TASK_STATE:
        case CFG_LINE_POS:
        case CFG_LINE_ERR:
        case CFG_WALK_STATE:
        case CFG_WALK_ERR:
            return sys_urcProc(SYS_MODULE_WALK, CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        case CFG_LIFT_TEMP:
        case CFG_LIFT_STATE:
        case CFG_LIFT_ERR:
            return sys_urcProc(SYS_MODULE_LIFT, CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        case CFG_SYS_MODE:
        case CFG_SYS_STATE:
        case CFG_SYS_ERR:
        case CFG_FAN_STATE:
        case CFG_FAN_ERR:
        case CFG_LED_STATE:
        case CFG_LED_ERR:
        case CFG_HALL_STATE:
        case CFG_HALL_ERR:
            return _sys_state_urc(CFG_DATA_STATE_READ(type, index), (void *)ppdata, 0);
        default: break;
    }

    *ppdata = NULL;
    return -1;
}

int cfg_item_ref(uint8_t **ppdata, uint8_t type, uint16_t index)
{
    int size = _cfg_mem_ref(ppdata, type, index);

    if (0 < size) {
        size &= ~CFG_DATA_BIGEND;
    }
    return (size);
}

int cfg_item_read(uint8_t *pdata, uint8_t type, uint16_t index, uint16_t len)
{
    uint32_t base = _cfg_eprom_dataAddr(type, index);
    uint8_t *pobj = NULL;
    int size = 0;

    if (CFG_INVALID_ADDR == base) {
        size = cfg_item_ref(&pobj, type, index); // get memory obj
        if (0 >= size) { // read file
            size = _cfg_file_dataProc(CFG_DATA_STATE_READ(type, 0), pdata, index, len);
        }
    }
    else {
        size = sys_cfg_read(base, pdata, len); // read eprom
        if (0 < size) {
            //return size;
        }
        else if (NULL != _pdef_cfg) {
            st_cfg_addr *paddr = (st_cfg_addr *)&base;

            pobj = (uint8_t *)_pdef_cfg + paddr->base;
            size = (!paddr->size ? len : paddr->size);
        }
    }

    if (0 < size) {
        if (NULL != pobj && NULL != pdata && 0 < len) {
            int i, j;

            if (CFG_DATA_BIGEND & size) {
                size &= ~CFG_DATA_BIGEND;
                for (i = 0, j = size; len > j; pdata[i++] = 0x0, j++);
                for (j = (len < size ? (size - len) : 0); size > j; pdata[i++] = pobj[j++]);
            }
            else {
                for (i = 0, j = (len < size ? len : size); j > i; pdata[i] = pobj[i], i++);
                for (; len > j; pdata[i++] = 0, j++);
            }
        }

        return (size < len ? size : len);
    }
    return -1;
}

int cfg_item_write(uint8_t *pdata, uint8_t type, uint16_t index, uint16_t len)
{
    uint32_t base = _cfg_eprom_dataAddr(type, index);
    uint8_t *pobj = NULL;
    int size = 0;

    if (CFG_INVALID_ADDR == base) {
        size = cfg_item_ref(&pobj, type, index); // get memory obj

        if (0 >= size) {
            size = _cfg_file_dataProc(CFG_DATA_STATE_WRITE(type, 0), pdata, index, len);
        }
    }
    else {
        size = sys_cfg_write(base, pdata, len); // write eprom
        if (0 < size) {
            //return size;
        }
        else if (NULL != _pdef_cfg) {
            st_cfg_addr *paddr = (st_cfg_addr *)&base;

            pobj = (uint8_t *)_pdef_cfg + paddr->base;
            size = (!paddr->size ? len : paddr->size);
        }
    }

    if (0 < size && NULL != pobj) {
        int i, j;

        if (!pdata) len = 0;
        if (CFG_DATA_BIGEND & size) {
            size &= ~CFG_DATA_BIGEND;
            for (i = 0, j = len; size > j; pobj[i++] = 0x0, j++);
            for (j = (size < len ? (len - size) : 0); len > j; pobj[i++] = pdata[j++]);
        }
        else {
            for (i = 0, j = (len < size ? len : size); j > i; pobj[i] = pdata[i], i++);
            for (; size > j; pobj[i++] = 0, j++);
        }

        return (size < len ? size : len);
    }
    return size;
}

int cfg_data_del(uint8_t *pdata, uint8_t type, uint16_t len)
{
    int pos = cfg_data_match(pdata, type, len);

    if (0 > pos) {
        return pos;
    }

    return cfg_item_write(NULL, type, pos, 0);
}

int cfg_item_clear(uint8_t type, uint16_t index)
{
    return cfg_item_write(NULL, type, index, 0);
}

int cfg_data_update(uint8_t type, float fold, float fnew)
{
    if ((int32_t)fold != (int32_t)fnew) {
        int32_t ntmp = (int32_t)fold;
        int index = cfg_data_match((void *)&ntmp, type, sizeof(ntmp));

        if (0 > index) {
            return -1;
        }

        if (0 < cfg_item_read((void *)&ntmp, type, index - 1, sizeof(ntmp))
            && (int)fnew <= ntmp) { // must be greater than last
            return -1;
        }
        else if (0 < cfg_item_read((void *)&ntmp, type, index + 1, sizeof(ntmp))
            && (int)fnew >= ntmp) { // must be leass than next
            return -1;
        }

        ntmp = (int32_t)fnew;
        cfg_item_write((void *)&ntmp, type, index, sizeof(ntmp));
    }
    return 0;
}

// config info is 0xff by default
// Length isn't enough, padding zero with forward.
// return free pos
int cfg_data_match(uint8_t *pdata, uint8_t type, uint16_t len)
{
    uint8_t tmpbuf[len];
    int size = 0;
    int i = 0;

    while(true) {
        size = cfg_item_read(tmpbuf, type, i, len);
        if (0 > size) {
            break;
        }
        else if (0 < size && !sys_memcmp(tmpbuf, pdata, size)) {
            return i;
        }
        else {
            i ++;
        }
    }
//if (CFG_PLACE_ID == type) return 0;
    return -1;
}

// type: 'H' is hex, 'B' is bcd, 'S'
int sys_datetime_fill(unsigned char *pdata, uint8_t type)
{
    uint8_t pos = 0;

    if ('H' == type) {
        pdata[pos++] = gCurrentTime.Year;
        pdata[pos++] = gCurrentTime.Month;
        pdata[pos++] = gCurrentTime.Day;
        pdata[pos++] = gCurrentTime.Hour;
        pdata[pos++] = gCurrentTime.Minute;
        pdata[pos++] = gCurrentTime.Second;
    } else if ('B' == type) {
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Year, 1);
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Month, 1);
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Day, 1);
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Hour, 1);
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Minute, 1);
        pdata[pos++] = sys_hex2bcd(gCurrentTime.Second, 1);
    } else {
        pos = sprintf((char *)pdata, "%02d%02d%02d%02d%02d%02d", gCurrentTime.Year, gCurrentTime.Month,
                    gCurrentTime.Day, gCurrentTime.Hour, gCurrentTime.Minute, gCurrentTime.Second);
    }
    return pos;
}

void sys_datetime_set(unsigned char *pdata, unsigned char num)
{
    if (0 < num) {
        struct tm timeinfo = {0};
        uint8_t pos = 0;

        sys_memset(&timeinfo, 0, sizeof(timeinfo));
        if (num > pos) timeinfo.tm_year = sys_bcd2hex(pdata[pos++], 1);
        if (num > pos) timeinfo.tm_mon  = sys_bcd2hex(pdata[pos++], 1) - 1;
        if (num > pos) timeinfo.tm_mday = sys_bcd2hex(pdata[pos++], 1);
        if (num > pos) timeinfo.tm_hour = sys_bcd2hex(pdata[pos++], 1);
        if (num > pos) timeinfo.tm_min  = sys_bcd2hex(pdata[pos++], 1);
        if (num > pos) timeinfo.tm_sec  = sys_bcd2hex(pdata[pos++], 1);
        BSP_rtc_datetime(&timeinfo);
    }
}

/** breif device SN(13bytes),
  * for example:K012410250001
 **/
uint32_t sys_dev_sn(uint8_t *pdata, uint8_t len)
{
    st_dev_sn dev_sn;

    cfg_data_read((void *)&dev_sn, CFG_SYS_SN, sizeof(dev_sn));
    if (NULL != pdata && 0 < len) { //
        snprintf((char *)pdata, len, "K%02d%02d%02d%02d%04d", dev_sn.type,
                dev_sn.year, dev_sn.month, dev_sn.day, dev_sn.id);
    }

    return *(uint32_t *)&dev_sn;
}

int cfg_data_init(void)
{
    int ret = sys_cfg_init();

    if (!ret) {
        uint32_t base = CFG_DATA_ADDR(0, DATA_TYPE_EPROM, 0, 0);
        uint16_t magic, num;

        num = sizeof(magic);
        if (num != sys_cfg_read(base, (void *)&magic, num) || MAGIC_CFG != magic) {
            st_eprom_cfg eprom_cfg = {
                .magic = MAGIC_CFG,
                .version = 0x1,
                .dev_sn  = {
                    .type  = PRJ_TYPE,
                    .year  = 24,
                    .month = 10,
                    .day   = 25,
                    .id    = 0x1,
                },
                .local_info = {
                    .mac  = { 0x00, 0x08, 0xdc, 0x00, 0xab, 0xcd },
                    .ip   = { 192, 168, 0, 130 },
                    .sn   = { 255, 255, 255, 0 },
                    .gw   = { 192, 168, 0, 1 },
                    .dns  = { 8, 8, 8, 8 },
                    .dhcp = NETINFO_STATIC,
                    .port = 1000,
                }, \
                .cloud_info = {
                    .user = MQTT_USR_NAME,
                    .pwd  = MQTT_USR_PWD,
                    .host = MQTT_SVR_IP,
                    .port = MQTT_SVR_PORT,
                }, \
                .nvidia_info = {
                    .user = NVIDIA_USR_NAME,
                    .pwd  = NVIDIA_USR_PWD,
                    .host = NVIDIA_SVR_IP,
                    .port = NVIDIA_SVR_PORT,
                },
                .pkt_ser = 0x1,
                .charge_warnbat = 20, /*unit %*/
                .charge_warntemp = 45, /*unit °„C*/
                .walk_cycle  = 150.796,
                .walk_speed  = {0x98, 0x3a, 0}, /*unit mm/s*/
                .walk_curpos = 0,
                .lift_curpos = 0,
                .lift_min = 0x172,
                .lift_max = 0x304,
            };

            _pdef_cfg = sys_malloc(sizeof(eprom_cfg));
						if (!_pdef_cfg) debug_printf("memory alloc is fail\r\n");
            if (sizeof(eprom_cfg) == sys_cfg_write(base, (void *)&eprom_cfg, sizeof(eprom_cfg))
                && ((!_pdef_cfg && sizeof(magic) == sys_cfg_read(base, (void *)&magic, sizeof(magic))
                && magic == eprom_cfg.magic) || (NULL != _pdef_cfg && sizeof(eprom_cfg) ==
                sys_cfg_read(base, (void *)_pdef_cfg, sizeof(eprom_cfg))
                && !sys_memcmp((void *)&eprom_cfg, (void *)_pdef_cfg, sizeof(eprom_cfg))))) {
                //sys_free(_pdef_cfg);
                //_pdef_cfg = NULL;
            }
            else {
                sys_memcpy((void *)_pdef_cfg, (void *)&eprom_cfg, sizeof(eprom_cfg));
            }
        }

//        _cfg_file_dataProc(CFG_DATA_STATE_READ(CFG_INSPECT_POINT, 0), NULL, 0, 0);
//        _cfg_file_dataProc(CFG_DATA_STATE_READ(CFG_INSPECT_PLAN, 0), NULL, 0, 0);
//        _cfg_file_dataProc(CFG_DATA_STATE_READ(CFG_INSPECT_RECORD, 0), NULL, 0, 0);
    }
    return ret;
}
