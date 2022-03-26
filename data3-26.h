#ifndef DATA1
#define DATA1

#include<stdio.h>
#include<stdlib.h>
#include"mp.h"

#define hash_array_maxsize  100

#define hash_num 10


typedef struct node{
    int key;
    int val;
    struct node* next;
}Node;

typedef struct hash{
    int size;
    int array_size;
    Node* array[hash_num];
}Hash;


int hash(int key);
int hash_init(Hash* h);
int hash_insert(Hash* h,int key,int val,M_P* mp);
int hash_delete(Hash* h,int key,int val,M_P* mp);
int hash_updata(Hash* h,int key,int val,M_P* mp);
int hash_find(Hash* h,int key,int* val);
int hash_print(Hash* h);
int hash_destroy(Hash* h,M_P* mp);





#endif
