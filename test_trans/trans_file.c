#include "trans_file.h"
#include <unistd.h>
#include <sys/stat.h>


#define FILE_DATA_HEAD  0xaaaa5555

#define FILE_DATA_TYPE_ACK       0x01
#define FILE_DATA_TYPE_FILE_DATA 0x02

#define DATA_FIELD_SIZE_MAX  (1024)

#define MESSSAGE_REPEAT_MAX  3

#define DATA_PROTOCOL_ID_SIZE     4
#define DATA_PROTOCOL_TOTAL_SIZE  2
#define DATA_PROTOCOL_INDEX_SIZE  2
#define DATA_PROTOCOL_TYPE_SIZE   2
#define DATA_PROTOCOL_LEN_SIZE    4
#define DATA_PROTOCOL_CRC_SIZE    2

#define DATA_PROTOCOL_HEAD_LEN  (DATA_PROTOCOL_ID_SIZE + DATA_PROTOCOL_TOTAL_SIZE +  \ 
DATA_PROTOCOL_INDEX_SIZE + DATA_PROTOCOL_TYPE_SIZE + DATA_PROTOCOL_LEN_SIZE)
#define DATA_PROTOCOL_LEN_MAX  (DATA_PROTOCOL_HEAD_LEN + DATA_FIELD_SIZE_MAX + DATA_PROTOCOL_CRC_SIZE)


struct channel_data {
    uint32_t head;
    uint16_t total;
    uint16_t index;
    uint16_t type;
    uint32_t len;
    void *data;
    uint16_t crc;
};

struct message_data {
    uint16_t type;
    uint32_t len;
    void *data;
 
};

pthread_cond_t syslink_cond   = PTHREAD_COND_INITIALIZER;  
pthread_mutex_t syslink_mutex = PTHREAD_MUTEX_INITIALIZER;  
int send_to_client(char *buf, int len);
int wait_for_ack(void);

extern uint16_t crc16_ccitt(void *ptr,int len);

static uint16_t s_total_pack_num = 0;

/* [head] [total] [index][type][len] [data] [crc]*/
int get_file_size(char *path)
 {
     struct stat st;
     if ((stat(path, &st)) != 0) {
        printf("fstat() fail\n");
     }
    printf("fsatt size[%d] \n",st.st_size);
    return st.st_size;
 }

void print_msg(struct channel_data * msg)
{

    printf("head[%x]\n",msg->head);
    printf("total[%d]\n",msg->total);
    printf("index[%d]\n",msg->index);
    printf("type[%d]\n",msg->type);
    printf("len[%d]\n",msg->len);
    printf("addr:%p\n",msg->data);
    printf("crc[%d]\n",msg->crc);

}

int syslink_channel_data_send(char *buf, uint32_t len)
{
    printf("syslink channel data send len [%d]\n", len);
    
    
    // send_to_client(buf, len);
    send_to_client(buf, 512);
    // sleep (2);
    // send_to_client(buf + 512, 512);
    // sleep (2);
    // send_to_client(buf + 1024, len -1024);
    return 0;
}

int syslink_channel_data_pack(struct channel_data *msg, char *buf, int buf_size)
{
    if (buf == NULL || msg == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
    if (buf_size < (msg->len + DATA_PROTOCOL_HEAD_LEN + DATA_PROTOCOL_CRC_SIZE)) {
        return -KD_ERROR_MEM_FAILED;
    }
    char *p_index = buf;
    *(uint32_t*)p_index = msg->head;
    p_index += DATA_PROTOCOL_ID_SIZE;
    *(uint16_t*)p_index = msg->total;
    p_index += DATA_PROTOCOL_TOTAL_SIZE;
    *(uint16_t*)p_index = msg->index;
    p_index += DATA_PROTOCOL_INDEX_SIZE;
    *(uint16_t*)p_index = msg->type;
    p_index += DATA_PROTOCOL_TYPE_SIZE;
    *(uint32_t*)p_index = msg->len;
    p_index += DATA_PROTOCOL_LEN_SIZE;
    if (msg->data == NULL) {
        return -KD_ERROR_ADDRESS_INVAILD;
    }
    memcpy(p_index, msg->data, msg->len);
    p_index += msg->len;
    /*not include crc */
    uint16_t crc = crc16_ccitt(buf, buf_size - DATA_PROTOCOL_CRC_SIZE);
    msg->crc = crc;
    *(uint16_t*)p_index = crc;
    return 0;
}


int syslink_message_send_block(uint16_t total, uint16_t index, struct message_data *msg)
{
   
    if (msg == NULL) {
        return -KD_ERROR_PARAM;
    }
    if (msg->data == NULL || msg->len == 0) {
        return -KD_ERROR_PARAM;
    }
    size_t size = msg->len + DATA_PROTOCOL_HEAD_LEN + DATA_PROTOCOL_CRC_SIZE;
    char *p_data = malloc(size);
    if (p_data == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
    struct channel_data channel_data;
    channel_data.head = FILE_DATA_HEAD;
    channel_data.total = total;
    channel_data.index = index;
    channel_data.type = msg->type;
    channel_data.len  = msg->len;
    channel_data.data = msg->data;
  
    int ret = syslink_channel_data_pack(&channel_data, p_data, size);
       printf("syslink channel send crc[%x]\n", channel_data.crc);
    if (ret != 0) {
        printf("syslink channel data pack failed[%d]\n", ret);
        goto FAIL;
    }
    /* uart send */
    ret = syslink_channel_data_send(p_data, size);
    if (ret == 0) {
        /* wait for ack*/
        wait_for_ack();
        printf("syslink channel data ack\n");
        goto FAIL;
    }
    printf("syslink_channel_data_send done \n");
FAIL:  //
    free(p_data);
    return ret;
}

/**
 * @brief If sending fails, repeat sending
 * 
 */
int syslink_message_with_resend(uint16_t total, uint16_t index, struct message_data *msg, uint8_t repeat)
{
    int ret = -1;
    for (int i = 0; i <= repeat; i++) {
        printf("syslink_message_with_resend index:%d\n", i);
        ret = syslink_message_send_block(total, index, msg);
        if (ret == 0) {
            break;
        }
    }
    return ret;
}
int syslink_file_transfer(char* path) 
{
    if (path == NULL) {
        return -KD_ERROR_PARAM;
    }
    /* file is exist?*/
    if(access(path, F_OK) != 0) {
    //    printf("file is not exist,%s\n",strerror(errno));
       return -KD_ERROR_NOT_EXIST;
    }
    FILE * fp = fopen(path,"r");
    if (fp == NULL) {
        return -KD_ERROR_OPEN_FAILED;
    }
    size_t size = get_file_size(path);
   

    printf("file size :%d\n", size);
    int num = (size + DATA_FIELD_SIZE_MAX - 1)/ DATA_FIELD_SIZE_MAX;
    char *p_buf = malloc (DATA_FIELD_SIZE_MAX);
    if (p_buf == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
    size_t read_size = 0, need_size = 0;
    int ret = 0;
    struct message_data msg;
    for (int i = 0; i < num; i++) {
        need_size = size > DATA_FIELD_SIZE_MAX ? DATA_FIELD_SIZE_MAX : size;
        read_size = fread(p_buf, 1, need_size, fp);
        
        size -= read_size;
        msg.data = p_buf;
        msg.len  = read_size;
        msg.type = FILE_DATA_TYPE_FILE_DATA;
        // printf("resend start:%s \n",p_buf);
        ret = syslink_message_with_resend(num, i, &msg, MESSSAGE_REPEAT_MAX);
        if (ret != 0) {
           goto FAIL;
        }
        if (read_size != DATA_FIELD_SIZE_MAX) {
            break;
        }
    }
    FAIL:
    free(p_buf);
    fclose(fp);
    return ret;
}


/************************************************/
/*recive*/
int syslink_channel_data_head(struct channel_data *msg, char *buf, int buf_size)
{
    if (buf == NULL || msg == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
    if (buf_size < (DATA_PROTOCOL_HEAD_LEN)) {
        return -KD_ERROR_DATA_INCOMPLETE;
    }
    char *p_index = buf;
    msg->head = *(uint32_t*)p_index;
    if (msg->head != FILE_DATA_HEAD) {
        return -KD_ERROR_DATA_CHECK_FAIL;
    }
    p_index += DATA_PROTOCOL_ID_SIZE;
    msg->total = *(uint16_t*)p_index;
    p_index += DATA_PROTOCOL_TOTAL_SIZE;
    msg->index = *(uint16_t*)p_index;
    p_index += DATA_PROTOCOL_INDEX_SIZE;
    msg->type = *(uint16_t*)p_index;
    p_index += DATA_PROTOCOL_TYPE_SIZE;
    msg->len = *(uint32_t*)p_index;
    return 0;
}
int syslink_channel_data_check(struct channel_data *msg, char *buf, int buf_size)
{
    if (buf == NULL || buf_size == 0 || msg == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
   
    /*not include crc */
    uint16_t crc = crc16_ccitt(buf, buf_size  - DATA_PROTOCOL_CRC_SIZE);
    msg->crc = *(uint16_t*)(buf + buf_size  - DATA_PROTOCOL_CRC_SIZE);
    printf("syslink channel recv crc[%x]->[%x]\n", msg->crc, crc);
    if (crc != msg->crc) {
        return -KD_ERROR_DATA_CHECK_FAIL;
    }
    return 0;
}
int sys_link_channel_farme_parse(struct channel_data *msg, char *buf, int buf_size)
{
    if (buf == NULL || buf_size == 0 || msg == NULL) {
        return -KD_ERROR_MEM_FAILED;
    }
    
    int ret = syslink_channel_data_head(msg, buf, buf_size);
    if (ret != 0) {
        return ret;
    }
    ret = syslink_channel_data_check(msg, buf, buf_size);

    if (ret != 0) {
        return ret;
    }
    msg->data = buf + DATA_PROTOCOL_HEAD_LEN;
    return 0;
}
#define RECV_BUF_MAX_LEN  DATA_FIELD_SIZE_MAX + 256

static char s_recv_buf[RECV_BUF_MAX_LEN];
static char s_recv_data[RECV_BUF_MAX_LEN];
static uint32_t s_recv_index = 0;
static bool s_buf_recv_is_start = false;

bool buf_splice_frame_is_open(void)
{ 
    return s_buf_recv_is_start;
}
int buf_splice_frame_open(bool on)
{
    s_buf_recv_is_start = on;
    if (on) {
        s_recv_index = 0;
    }
    return 0;
}
int buf_splice_frame_push(char *data, int size)
{
    if (buf_splice_frame_is_open() == false) {
        return -1;
    } 
    uint32_t len = size;
    if (s_recv_index + len > RECV_BUF_MAX_LEN - 1) {
        len = RECV_BUF_MAX_LEN - s_recv_index -1;
    }
    if (len == 0) {
        return -KD_ERROR_DATA_FULL;
    }
    memcpy(&s_recv_buf[s_recv_index], data, len);
    s_recv_index += len;
    return s_recv_index;

}
char* buf_splice_frame_pop( int *size)
{
    
    
    s_recv_index = 0;
    return s_recv_buf;

}

int data_to_file(char *buf, int size, int index)
{
      FILE * fp = NULL;
    if (index = 0) {
    fp = fopen("test_recv.txt","w+");
    if (fp == NULL) {
        return -KD_ERROR_OPEN_FAILED;
    }
    }
    else {
        fp = fopen("test_recv.txt","a+");
        if (fp == NULL) {
        return -KD_ERROR_OPEN_FAILED;
        }
    }
     printf("open file size[%d][%d]{%s}]\n", size,index,buf);
    int write_size = fwrite(buf, 1, size, fp);
    if (write_size == size) {
        printf("write len:[%d]\n", write_size);
    } else {
        printf("write fail [%d]\n", write_size);
    }
    fclose(fp);
    return 0;
}

static char s_recv_data[RECV_BUF_MAX_LEN];
static struct channel_data s_msg;
static int s_recv_data_seccuss = 0;
int syslink_message_receive(char* buf, int size, int *res)
{
    if(buf == NULL | size == 0) {
        return -KD_ERROR_PARAM;
    }
   
    if (syslink_channel_data_head(&s_msg, buf, size) == 0) {
        buf_splice_frame_open(true);
    }
    if (buf_splice_frame_is_open() == false) {
        return -KD_ERROR_DATA_INCOMPLETE;
    }
    // printf("syslink_message_receive: len[%d]\n",);
    int ret = 0;
    int len = buf_splice_frame_push(buf, size);
    printf("syslink_message_receive: len[%d]\n", len);
    if (len >= (s_msg.len + DATA_PROTOCOL_HEAD_LEN + DATA_PROTOCOL_CRC_SIZE)) {
        buf_splice_frame_open(false);
        char *p_buf = buf_splice_frame_pop(NULL);
        ret = sys_link_channel_farme_parse(&s_msg, p_buf, len);
        print_msg(&s_msg);
        if (ret == 0) {
           memcpy(s_recv_data, s_msg.data, s_msg.len);
           printf("channel parse:%s\n",s_recv_data);
           data_to_file(s_recv_data, s_msg.len, s_msg.index);
           *res = 1;
        } else {
            printf("sys_link_channel_farme_parse fail[%d]\n", ret);
        }
    }
    return 0;

}

#define TEST 1
#if TEST

static int srv_recv_flag = 0;
static int srv_recv_len = 0;
static  char srv_recv_buf[2048];

static int  cli_recv_flag = 0;
static int  cli_recv_len = 0;
static char*  cli_recv_addr = 0;
static char cli_recv_buf[2048];
int wait_for_ack(void)
{
    srv_recv_flag = 0;

     pthread_mutex_lock(&syslink_mutex);  
     printf ("lock\n"); 
     while(srv_recv_flag == 0)  {
         pthread_cond_wait(&syslink_cond, &syslink_mutex);  
     }
     printf ("unlock\n");  
    pthread_mutex_unlock(&syslink_mutex); 
}
int send_to_server(void *data)
{
    
    pthread_mutex_lock(&syslink_mutex);  
    srv_recv_flag = 1;
    pthread_mutex_unlock(&syslink_mutex); 
    pthread_cond_signal(&syslink_cond);
 
}

int send_to_client(char *buf, int len)
{
    cli_recv_flag = 1;
    cli_recv_len = len;
    cli_recv_addr = buf;
    memcpy(cli_recv_buf, buf, len);
    printf("len[%d]\n",len);
}



pthread_cond_t taxicond = PTHREAD_COND_INITIALIZER;  
pthread_mutex_t taximutex = PTHREAD_MUTEX_INITIALIZER;  
  
void *traveler_arrive(void *name)  
{  
    char *p = (char *)name;  
  
    printf ("Travelr: %s need a taxi now!\n", p);  
    // 加锁，把信号量加入队列，释放信号量
    pthread_mutex_lock(&taximutex);  
     printf ("lock\n");  
    pthread_cond_wait(&taxicond, &taximutex);  
     printf ("unlock\n");  
    pthread_mutex_unlock(&taximutex);  
    printf ("traveler: %s now got a taxi!\n", p);  
    // pthread_exit(NULL);  
}  

void *taxi_arrive(void *name)  
{  
    char *p = (char *)name;  
    printf ("Taxi: %s arrives.\n", p);
    // 给线程或者条件发信号，一定要在改变条件状态后再给线程发信号
    pthread_cond_signal(&taxicond);  
    // pthread_exit(NULL);  
}  
  


static void *test_recv_task(void *arg)
{
    // printf("test_task_func \n");
    int res;
    while (1) {
    //    if( srv_recv_flag == 1) {
    //         printf("server recv [%d]\n", srv_recv_len);
    //         ret = 0xffff;
           
    //        printf("server recv res[%d]\n", res);
    //    }
    //    traveler_arrive("guo");
       sleep(2);
    };
}

static void *test_client_task(void *arg)
{
    printf("test_client_task \n");
    sleep(2);
     int res;
    while (1) {
#if 1
        if( cli_recv_flag == 1) {
            res = 0xffff;
            //  printf("client recv [%d][%s]\n", cli_recv_len, cli_recv_buf + DATA_PROTOCOL_HEAD_LEN);
            int ret = syslink_message_receive(cli_recv_buf, cli_recv_len,&res);
            printf("client recv res[%d] len[%d]\n", ret,res);
            if (ret == 0) {
                printf("send_to_server\n");
                send_to_server(NULL);
            }
            cli_recv_flag = 0;
        }
#else
      traveler_arrive("xuan");
#endif
       sleep(1);
    };

}

static void *test_send_task(void *arg)
{
    printf("test_send_task \n");
    int ret = 0;
    while (1) {
        sleep(7);
       ret = syslink_file_transfer("test.txt");
#if 1
    //    ret = syslink_file_transfer("test_file.txt");
       printf("test_send_task:send file [%d]\n",ret);
#else 

    taxi_arrive("didi");
        

#endif
        sleep(7);
    };

}
static pthread_t test_recv_id = -1;
static pthread_t test_send_id = -1;
static pthread_t test_client_id = -1;

int trans_test_init(void)
{
    printf(" trans_test_init\n");
    int ret  = pthread_create(&test_recv_id, (void *)0, test_recv_task, (void *)0);
    if (0 != ret)
    {
        printf(" thread create failed\n");
        return ret;
    }
         ret  = pthread_create(&test_send_id, (void *)0, test_send_task, (void *)0);
    if (0 != ret)
    {
        printf(" thread create failed\n");
        return ret;
    }
        ret  = pthread_create(&test_client_id, (void *)0, test_client_task, (void *)0);
    if (0 != ret)
    {
        printf(" thread create failed\n");
        return ret;
    }
    while(1) {
        sleep(100);
    }
    return 0;
}
#endif

