#include <stdio.h>
#include <pthread.h>
#include "trans_file.h"
#include "test_api.h"

extern int kd_test_is_aging_mode(void);
extern int aging_mode_update_time(void);
extern int kd_test_aging_mode_enter(void);
extern int kd_test_aging_mode_exit(void);


pthread_mutex_t mutex;  

static void *test_task_func(void *arg)
{
    printf("test_task_func \n");
    sleep(5);
    while (1) {
       pthread_mutex_lock(&mutex);
       aging_mode_update_time();
       pthread_mutex_unlock(&mutex);
       sleep(10);
    };

}

int factory_test_task_init(void)
{

    printf("factory_test_task_init\n ");

    static pthread_t test_thread_id = -1;
    int ret  = pthread_create(&test_thread_id, (void *)0, test_task_func, (void *)0);
    if (0 != ret)
    {
        printf(" thread create failed\n");
        return ret;
    }
    return 0;

}


extern void test_cli(void);
typedef enum {
    GNSS_SIGNAL_INVALID   = -1,  /* [0.0, 0.5) */
    GNSS_SIGNAL_GOOD      = 1,   /* [0.5, 2.0) */
    GNSS_SIGNAL_MEDIUM    = 2,   /* [2.0 - 4.0)  */
    GNSS_SIGNAL_WEAK      = 3,   /* [4.0 - 6.0) */
    GNSS_SIGNAL_POOR      = 4,   /* [6.0 - 99.0) */
} gnss_signal_level_t;
gnss_signal_level_t gnss_pdop_to_level(double pdop)
{
    if (pdop < 0.5) {
        return GNSS_SIGNAL_INVALID;
    } else if(pdop < 2.0) {
        return GNSS_SIGNAL_GOOD;
    } else if(pdop < 4.0) {
        return GNSS_SIGNAL_MEDIUM;
    } else if(pdop < 6.0) {
        return GNSS_SIGNAL_WEAK;
    } else if(pdop < 99.0){
        return GNSS_SIGNAL_POOR;
    } else {
        return GNSS_SIGNAL_INVALID;
    }
}
void test_gnss(void)
{
 double arry[10] = {0, 0.3, 0.5, 1.0, 1.3, 3.3,5.4,8.5, 99.0};
 for(int i = 0; i < 10; i++) {
 
     printf("[%d]= [%d]\n",i,gnss_pdop_to_level(arry[i]));
 }

}

void print_2_power(void)
{
    for(int i = 0; i <32; i++){
       printf("[%d][%x]\n",i, 0x01 << i);
    }
}
#include "time.h"
void test_time(void)
{
    uint64_t time_test = time(NULL) * 100000;
    unsigned int time_long = time(NULL);
    uint64_t  time_temp = time_long * 10000;
    uint32_t val1 = 4000000000; 
    uint32_t val2 = 1000; 
    uint64_t value = (uint64_t)val1 * val2;

    printf("time1:[%lu]\n",value);
    printf("time1:[%llu]\n",time_test);
    printf("time2:[%lu]\n",time_temp);
    return ;
}
#if 1
int main(void)
{
    printf("hello\n");
    // print_2_power();
    //test_gnss();
    // test_cli();
    //trans_test_init();
   // test_sprintf();a
   test_time();
    printf("end\n");

}
#else
int main (void)
{
  
  printf("hello\n");
  int ret = kd_test_is_aging_mode();
  printf("is aging mode:%d\n",ret);
  kd_test_aging_mode_enter();
  ret = pthread_mutex_init(&mutex,NULL);  //初始化互斥锁
	if(ret != 0){
		printf("mute init error\n");
		exit(1);
	}
  factory_test_task_init();
  static int cnt = 0;
  while (1) {

   
    sleep(5);
    pthread_mutex_lock(&mutex);
    
    ret = kd_test_is_aging_mode();
    printf("is aging mode:%d\n",ret);
    cnt++;
    if (cnt > 10) {
         kd_test_aging_mode_exit();
    }
  
    pthread_mutex_unlock(&mutex);
  };
  return 0;
}
#endif
