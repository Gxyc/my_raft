#ifndef _MP_
#define _MP_

#include<stdio.h>
#include<stdlib.h>

#define data_size 16
#define mp_max_size 65536
//#define mp_extend_size 4096

#define mp_empty_error -1
#define mp_outsize_error -2

#define mp_using_error -3

#define out_mp_error -4

#define malloc_error -11


typedef union mem_node{
    union mem_node* next;
    char data[1];
}M_N;


typedef struct mem_pool{
    int size;
    //int index;
    void* p;
    //void* link;
    M_N* free_list;
    
}M_P;


int mp_init(M_P* mp,int size);
void* mp_getmem(M_P* mp);
int mp_backmem(M_P* mp,char* p);
int mp_extend(M_P* mp,int size);
int mp_destroy(M_P* mp);
int mp_force(M_P* mp);
int mp_print(M_P* mp);

#endif