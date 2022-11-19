#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>


#define FACTORY_TEST_AGING_MODE_CFG "./test_aging_mode.cfg"



#define AGING_CHECK_INTERVAL_MINUTE   10   


struct aging_data {
    uint32_t start_utc_time;
    uint32_t total_times;  /* times !!*/
    uint32_t update_times;
};

static struct aging_data s_aging_data;

int kd_test_is_aging_mode(void)  
{
    int ret = 0;
    if (0 == access(FACTORY_TEST_AGING_MODE_CFG, F_OK)) {
        ret = 1;
    } 
    return ret;
}

int aging_mode_update_time(void)
{
    static uint32_t times = 0;
    FILE *fp = NULL;
    if (0 == kd_test_is_aging_mode()) {
          return -1;
    }
    int ret = 0;
    times++;
    fp = fopen(FACTORY_TEST_AGING_MODE_CFG, "r+");
    if (NULL == fp) {
        printf("fopen[%s] Error:\n", FACTORY_TEST_AGING_MODE_CFG);
        return -2;
    }
    int size = fread(&s_aging_data, 1, sizeof(s_aging_data), fp);
    
    if (size != sizeof(s_aging_data)) {
        printf("up fread error size [%d,%d]\n", size,sizeof(s_aging_data));
        ret = -3;
        goto END;
    }
    fseek(fp, 0, SEEK_SET);
    printf("total times[%d], update times [%d]\n", s_aging_data.total_times, s_aging_data.update_times);
    s_aging_data.total_times++;
    s_aging_data.update_times = times;
    size = fwrite(&s_aging_data, 1, sizeof(s_aging_data), fp);
    if (size != sizeof(s_aging_data)) {
        printf("fwrite error size [%d,%d]\n", size,sizeof(s_aging_data));
        ret = -4;
        goto END;
    }
END:
    fsync(fileno(fp));
    fclose(fp);
    return ret;
}


int kd_test_aging_mode_enter(void)
{
    FILE *fp = NULL;
    memset(&s_aging_data, 0, sizeof(s_aging_data));
    if (0 == kd_test_is_aging_mode()) {
        fp = fopen(FACTORY_TEST_AGING_MODE_CFG, "w+");
        if (NULL == fp) {
            printf("fopen[%s] Error:\n", FACTORY_TEST_AGING_MODE_CFG);
            return -1;
        }
        struct timeval tv;
	    gettimeofday(&tv, NULL);
        s_aging_data.start_utc_time = tv.tv_sec;
        int size = fwrite(&s_aging_data, 1, sizeof(s_aging_data), fp);
        if (size != sizeof(s_aging_data)) {
             printf("fwrite error size [%d,%d]\n", size,sizeof(s_aging_data));
        }
        printf("fwrite  size [%d,%d]\n", size,sizeof(s_aging_data));
   
       if (size != sizeof(s_aging_data)) {
        printf(" fread error size [%d,%d]\n", size,sizeof(s_aging_data));
       
        }
        fsync(fileno(fp));
        fclose(fp);
        printf("aging mode create file\n");
    } 
   
    return 0;
}

int kd_test_aging_mode_exit(void)
{
   
    // led_start(0); 
    struct timeval tv;
	gettimeofday(&tv, NULL);
    struct tm tm;
	localtime_r(&tv.tv_sec, &tm);
    int ret = 0;
    FILE* fp = fopen(FACTORY_TEST_AGING_MODE_CFG, "wb+");
    if (NULL == fp) {
        printf("fopen[%s] Error:    \n", FACTORY_TEST_AGING_MODE_CFG);
        ret = -1;
        goto END;
    }
    int size = fread(&s_aging_data, 1, sizeof(s_aging_data), fp);
    
    if (size != sizeof(s_aging_data)) {
        printf("exit fread error size [%d,%d]\n", size,sizeof(s_aging_data));
        ret = -3;
        goto END;
    }
END:
    fclose(fp);
    remove(FACTORY_TEST_AGING_MODE_CFG);
    printf("total times[%d], update times [%d]\n", s_aging_data.total_times, s_aging_data.update_times);
    printf("start utc time [%d], end utc time [%d] use times[%d]\n", s_aging_data.start_utc_time, tv.tv_sec ,tv.tv_sec - s_aging_data.start_utc_time);
    printf("start [%d%02d%02d-%02d%02d%02d.%03ld]", (tm.tm_year +1900), tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec/1000);
    
    uint32_t total_time = 0;
    if (ret == 0) {
     total_time = AGING_CHECK_INTERVAL_MINUTE * s_aging_data.total_times;
    }
    return total_time;
}