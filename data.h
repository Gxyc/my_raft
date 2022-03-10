#ifndef DATA_H
#define DATA_H

#include<stdio.h>
#include<stdlib.h>

//#include"raft.h"


#define pool_max_size 1024

#define hash_num 10
#define array_maxsize 100

#define pool_init_error -11
#define pool_empty_error -21
#define pool_notuse_error -31
#define pool_check_use_error -41
#define pool_check_free_error -42
#define pool_destroy_error -51

#define hash_empty_error -11
#define hash_notfind_error -12
#define hash_exists_error -21
#define hash_full_error -22


typedef struct node{
    int key;
    int value;
    struct node* next;
}Node;

typedef struct hash{
    int size;
    int array_size;
    Node** array;
}Hash;

typedef struct memory_node{
    int size;
    //void* pointer;
    struct memory_node* next;
}M_N;

typedef struct memory_pool{
    int size;
    void* p;
    M_N* free_head;
    M_N* use_head;
}M_P;


int memory_pool_init(M_P* mp,int size);
int get_memory(M_P* mp,void* pointer,int size);
int back_memory(M_P* mp,void* pointer,int size);
int memory_pool_check(M_P* mp);
int memory_pool_destroy(M_P* mp);

int linklist_insert(M_N* node,M_N* head);
int linklist_delete(M_N* node,M_N* head);

int hash_init(Hash* h,M_P* mp);
int hash(int key);
int hash_find(int key,Hash* h,int* val);
int hash_insert(int key,int val,Hash* h,M_P* mp);
int hash_delete(int key,int val,Hash* h,M_P* mp);
int hash_print(Hash* h);


#endif