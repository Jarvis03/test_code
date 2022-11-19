#ifndef _TRANS_FILE_H_
#define _TRANS_FILE_H_
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#define  KD_ERROR_PARAM             1
#define  KD_ERROR_NOT_EXIST         2
#define  KD_ERROR_OPEN_FAILED       3
#define  KD_ERROR_MEM_FAILED        4
#define  KD_ERROR_ADDRESS_INVAILD   5
#define  KD_ERROR_DATA_CHECK_FAIL   6
#define  KD_ERROR_DATA_INCOMPLETE   7
#define  KD_ERROR_DATA_FULL         8
int trans_test_init(void);

#endif