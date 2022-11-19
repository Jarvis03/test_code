#include <stdio.h>
#include <stdint.h>

#include "msgpuck.h"

int main (void)
{
   char buf[1024];
   printf("encode\n");
   encode(buf, 1024);
   printf("decode\n");

   decode(buf, 1024);
   return 0;
}

int encode(char *buf, int len)
{
    char *w = buf;
    w = mp_encode_array(w, 4);
    w = mp_encode_uint(w, 10);
    w = mp_encode_str(w, "hello world", strlen("hello world"));
    w = mp_encode_bool(w, true);
    w = mp_encode_double(w, 3.1415);
    return 0;
}

int decode(char* buf, int len)
{
    uint32_t size;
    uint64_t ival;
    const char *sval;
    uint32_t sval_len;
    bool bval;
    double dval;

    const char *r = buf;
    size = mp_decode_array(&r);
    printf("size[%d]\n",size);
    /* size is 4 */

    ival = mp_decode_uint(&r);
    printf("ival[%ld]\n",ival);
    /* ival is 10; */

    sval = mp_decode_str(&r, &sval_len);
    printf("sval[%s]\n",sval);
    /* sval is "hello world", sval_len is strlen("hello world") */

    bval = mp_decode_bool(&r);
    printf("bval[%d]\n",bval);

    /* bval is true */

    dval = mp_decode_double(&r);
    printf("dval[%f]\n",dval);

    /* dval is 3.1415 */

    assert(r == w);
    return 0;
}