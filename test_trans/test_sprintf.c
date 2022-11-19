#include "test_api.h"
#include <unistd.h>
#include <sys/stat.h>

#define  RU_REG_MAXNUM 10

typedef struct
{
    uint8_t status[RU_REG_MAXNUM] ;
    char RegInfo[RU_REG_MAXNUM][30];
}OBDRegGetInfo;
static char* obd_ru_reg[RU_REG_MAXNUM] = {
    "IGN",
    "WIPER",
    "D_SEAT_BELT",
    "P_SEAT_BELT",
    "L_TURN",
    "R_TURN",
    "LF_DOOR",
    "RF_DOOR",
    "LB_DOOR",
    "RB_DOOR",
};
static char* obd_ru_val[RU_REG_MAXNUM] = {
    "on",
    "18",
    "89",
    "off",
    "stop",
    "on",
    "78.9",
    "10",
    "off",
    "on",
};
OBDRegGetInfo ru_reg ={{1,1,1,1,1,1,1,1,1,1},};
static inline int time_format_convert(char*str_buf, int size, int64_t ts) 
{
    if (str_buf == NULL) {
        return -1;
    }
    time_t tmt = 0;
    if (ts != 0) {
        tmt = (long)(ts / 1000);
    }
    struct tm *p = gmtime(&tmt);
    strftime(str_buf, size, "%Y-%m-%d %H:%M:%S", p);
    return 0;
}



int test_sprintf(void)
{
    char printf_buffer[1024];
    int64_t ts;
    char timebuff[32] = "";
    for(int i = 0; i < RU_REG_MAXNUM; i++) {
        strcpy(ru_reg.RegInfo[i], obd_ru_val[i]);
    }

    time_format_convert(timebuff, 32, ts);
    snprintf(printf_buffer, sizeof(printf_buffer), "%s", timebuff);
    for(int i = 0; i < RU_REG_MAXNUM; i++) {
         sprintf(printf_buffer + strlen(printf_buffer),"[%s]=[%s],",obd_ru_reg[i],ru_reg.RegInfo[i]);
    }
    printf("re:{%s}\n",printf_buffer);
}