#include"data1.h"

#define error_key_exists -11
#define error_key_nofind -21
//#define error_val_wrong -22

int hash(int key){
    return key%hash_num;
}

int hash_init(Hash* h){
    h->size = hash_num;
    int i = 0;
    for(i = 0;i < h->size;i++){
        h->array[i] = NULL;
    }
    h->array_size = hash_array_maxsize;
    return 1;
}

int hash_insert(Hash* h,int key,int val){
    if(h->array[hash(key)] == NULL){
        h->array[hash(key)] = (Node*)malloc(sizeof(Node));
        h->array[hash(key)]->key = key;
        h->array[hash(key)]->val = val;
        h->array[hash(key)]->next = NULL;
        return 1;
    }
    Node* p = h->array[hash(key)];
    while(p->next != NULL){
        if(p->key == key){
            printf("key already exists\n");
            return error_key_exists;
        }
        p = p->next;
    }
    if(p->key == key){
        printf("key already exists\n");
        return error_key_exists;
    }
    Node* n = (Node*)malloc(sizeof(Node));
    n->key = key;
    n->val = val;
    n->next = NULL;
    p->next = n;
    return 1;    
}

int hash_delete(Hash* h,int key,int val){
    if(h->array[hash(key)] == NULL){
        printf("hash empty error\n");
        return error_key_nofind;
    }
    Node* p1 = h->array[hash(key)];
    Node* p2 = p1->next;
    if(p1->key == key){
        //if(p1->val == val){
            h->array[hash(key)] = p2;
            free(p1);
            return 1;
        //}
        //printf("wrong value\n");
        //return error_val_wrong;
    }
    while(p2 != NULL){
        if(p2->key == key){
            //if(p2->val == val){
                p1->next = p2->next;
                free(p2);
                return 1;
            //}
            //printf("wrong value\n");
        }
        p1 = p1->next;
        p2 = p1->next;
    }
    printf("not find key\n");
    return error_key_nofind;
}

int hash_updata(Hash* h,int key,int val){
    if(h->array[hash(key)] == NULL){
        Node* n = (Node*)malloc(sizeof(Node));
        n->key = key;
        n->val = val;
        n->next = NULL;
        h->array[hash(key)] = n;
        return 1;
    }
    Node* p1 = h->array[hash(key)];
    if(p1->key == key){
        p1->val = val;
        return 1;
    }
    Node* p2 = p1->next;
    while(p2 != NULL){
        if(p2->key == key){
            p2->val = val;
            return 1;
        }
        p1 = p1->next;
        p2 = p1->next;
    }
    Node* n = (Node*)malloc(sizeof(Node));
    n->key = key;
    n->val = val;
    n->next = NULL;
    p1->next = n;
    return 1;
}

int hash_find(Hash* h,int key,int* val){
    if(h->array[hash(key)] == NULL){
        printf("hash empty error");
        return error_key_nofind;
    }
    Node* p = h->array[hash(key)];
    while(p != NULL){
        if(p->key == key){
            *val = p->val;
            return 1;
        }
        p = p->next;
    }
    printf("not find key\n");
    return error_key_nofind;
}

int hash_print(Hash* h){
    int i = 0;
    int j = 0;
    Node* p = NULL;
    for(i = 0;i < h->size;i++){
        j = 0;
        p = h->array[i];
        //printf("%d\t",i);
        while(p != NULL){
            printf("hash_print %d %d key %d val %d\n",i,++j,p->key,p->val);
            p = p->next;
        }
        //printf("\n");
    }
    return 1;
}

int hash_destory(Hash* h){
    int i = 0;
    Node* p = NULL;
    Node* d = NULL;
    for(i = 0;i < h->size;i++){
        p = h->array[i];
        while(p != NULL){
            d = p->next;
            free(p);
            p = d;
        }
    }
    return 1;
}


