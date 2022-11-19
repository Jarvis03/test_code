#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define ERR_PARAM  (-1)

typedef int (*cmd_proc) (void *arg, int num);
struct cmd_format {
    char *cmd;
    cmd_proc func;
};

static int cli_proc_kd_ut(void *arg, int num);
static int cli_proc_kd_ut_all(void *arg, int num);
static char* cli_get_arg(uint32_t index);


#define CMD_KD_NUM_MAX  2 

struct cmd_format s_kd_cmd_list[CMD_KD_NUM_MAX]= {
    {"utall", cli_proc_kd_ut_all},
    {"ut", cli_proc_kd_ut},
};



#define SLICE_NUM_MAX  5
#define SLICE_LEN_MAX  20

static char str_slice[SLICE_NUM_MAX][SLICE_LEN_MAX + 1];

static char* cli_get_arg(uint32_t index) 
{
  if(index > SLICE_NUM_MAX) {
      return NULL;
  }   
  return str_slice[index];
}

static uint16_t cli_arg_num_max(void)
{
     return SLICE_NUM_MAX;
} 

static int cmd_str_slice(uint8_t *str, uint32_t size)
{

    if (str == NULL || size == 0) {
        return -1;
    }
    if (*str != '$') {
        return -2;
    }
    uint8_t p_index = 0;
    uint8_t *p_start = (str + 1); /* skip '$' */
    int8_t p_seg_index = -1;
    uint32_t seg_size= 0;
    memset(str_slice, 0 , (SLICE_LEN_MAX + 1) * SLICE_NUM_MAX);
    for(int i = 0; i < size; i++) {
        if (*(p_start + i) == ',') {
            if (i - p_seg_index <= 1) {
                p_seg_index = i;
                continue;
            }
            seg_size = i - p_seg_index - 1 > SLICE_LEN_MAX ? SLICE_LEN_MAX : i - p_seg_index -1;

            memcpy(str_slice[p_index], (p_start + p_seg_index + 1), seg_size);
            p_index++;
            p_seg_index = i;
        }
        if (*(p_start + i) == '\n' || *(p_start + i) == '\r') {
            if (p_seg_index <= 0) {
                break;
            }
            seg_size = i - p_seg_index - 1 > SLICE_LEN_MAX ? SLICE_LEN_MAX : i - p_seg_index -1;
            memcpy(str_slice[p_index], (p_start + p_seg_index + 1), seg_size);
            p_index++;
            break;
        } else if (i == size - 1){
            if (p_seg_index <= 0) {
                break;
            }

            seg_size = size - p_seg_index - 1 > SLICE_LEN_MAX ? SLICE_LEN_MAX : size - p_seg_index -1;
            memcpy(str_slice[p_index], (p_start + p_seg_index + 1), seg_size);
            p_index++;
        }
        if (p_index >= SLICE_NUM_MAX) {
            break;
        }
    }
    return p_index;
}
static int cli_proc_kd_ut_all(void *arg, int num)
{
    if (num != 1) {
        printf(" param error[%d]\n", num);
        return ERR_PARAM;
    }
    int32_t on = *(int32_t*)arg;
    if (on) {
        printf("kd factory start\n");
        // factory_test_task_init();
    } else {
        printf("kd factory end\n");
        // factory_test_task_deinit();
    }
}
static int cli_proc_kd_ut(void *arg, int num) 
{
    if (num != 1 || arg == NULL) {
        printf(" param error[%d]\n", num);
        return ERR_PARAM;
    }
    uint64_t *p_data = (uint32_t*)arg;
    char *arg1 = p_data[0];
    printf("arg1 %llx= %llx\n",arg1,p_data[0]);
    int32_t index = atoi(arg1);
    printf(" kd ut proc: %d\n", index);
    return 0 ; //factory_test_case(index, NULL); 
}

static int cli_kd_proc(uint8_t num)
{
    if (num == 0 || num > cli_arg_num_max() - 1) {
        return -1;
    }
    char* cmd = cli_get_arg(1);
    int  arg_num = num - 1;
    uint64_t* p_arg = NULL;
    if (arg_num != 0) {
        p_arg = malloc(sizeof(char*) * arg_num);
        if (p_arg == NULL) {
            printf("kd malloc failed\n");
            return -1;
        }
    
        for (int i = 0; i <  num; i++) {
            *(p_arg + i) = cli_get_arg(i + 2);
        }
    }
    printf(" addr %p, %s\n",cli_get_arg(2), cli_get_arg(2));
    printf(" addr %llx, %d\n",*(p_arg + 0),sizeof(char*) );
    for(int i = 0; i < CMD_KD_NUM_MAX; i++) {
        if (0 == strcmp(s_kd_cmd_list[i].cmd, cmd)) {
            s_kd_cmd_list[i].func(p_arg, arg_num);
        } else {
            printf(" kd cmd[%s]\n", cmd);
        }
    } 
    if (p_arg != NULL) {
        free(p_arg);
    }
}

void test_cli(void) 
{
   int num = cmd_str_slice("$kd,ut,1989", 10);
   if (num != 0) {
       if (0 == strcmp(cli_get_arg(0), "kd")) {
            cli_kd_proc(num -1);
       } else {
          printf("cmd slice arg[0]: %s\n",cli_get_arg(0));
       }
   }
}