#include"data.h"




int memory_pool_init(M_P* mp,int size){
    if(size > pool_max_size){
        return pool_init_error;
    }

    mp->free_head = malloc(sizeof(M_N));
    mp->use_head = malloc(sizeof(M_N));

    mp->size = size;
    
    mp->use_head = NULL;
    //mp->use_head->next = NULL;
    //mp->use_head->pointer = NULL;
    //mp->use_head->size = 0;
    mp->use_head = NULL;
    
    mp->p = malloc(size);

    mp->free_head = mp->p;
    //mp->free_head->pointer = mp->p;
    mp->free_head->next = NULL;
    mp->free_head->size = size;

    return 1;
}

int get_memory(M_P* mp,void* pointer,int size){
    if(mp->free_head == NULL)return pool_empty_error;
        
    if(mp->free_head->size >= size){
        M_N* mn;
        mn->size = size;
        mn->next = NULL;
        mn = mp->free_head;

        printf("size ok\n");

        mp->free_head->size -= size;
        if(mp->free_head->size == 0){
            if(mp->free_head->next != NULL){
                //mp->free_head->size = mp->free_head->next->size;
                //mp->free_head->pointer = mp->free_head->next;
                mp->free_head = mp->free_head->next;
            }
            else{
                mp->free_head = NULL;
            }
        }
        else{
            mp->free_head += size;
        }

        linklist_insert(mn,mp->use_head);
        pointer = mn;

    }
    else{
        return pool_empty_error;
    }
    return 1;
}

int back_memory(M_P* mp,void* pointer,int size){
    M_N* mn;
    mn = pointer;
    mn->next = NULL;
    mn->size = size;


    //check? out?
    if(linklist_delete(mn,mp->use_head) == 1){
        linklist_insert(mn,mp->free_head);
        return 1;
    }
    else return pool_notuse_error;

}

int memory_pool_check(M_P* mp){
    if(mp->use_head != NULL)return pool_check_use_error;
    M_N* p1 = mp->free_head;
    M_N* p2 = mp->free_head->next;
    if(p1 == NULL) return pool_check_free_error;
    int size = p1->size;
    while(p2){
        size += p2->size;
        //check index?
    }
    if(size != mp->size)return pool_check_free_error;
    return 1;
}

int memory_pool_destroy(M_P* mp){
    if(memory_pool_check(mp) == 1){
        free(mp->p);
        free(mp->free_head);
        free(mp->use_head);
        mp->p = NULL;
        mp->free_head = NULL;
        mp->use_head = NULL;
        mp->size = 0;
        return 1;
    }
    else return pool_destroy_error;
}

/*int memory_pool_destroy_force(M_P* mp){

}*/

int linklist_insert(M_N* node,M_N* head){
    if(head == NULL){
        head = node;
        return 1;
    }
    else{
        if(head > node){
            node->next = head;
            head = node;
            return 1;
        }
        M_N* p1 = head;
        M_N* p2 = head->next;
        while(p2 != NULL){
            if(p2 > node){
                node->next = p2;
                p1->next = node;
                return 1;
            }
            p1 = p1->next;
            p2 = p1->next;
        }
        p1->next = node;
        return 1;
    }

}

int linklist_delete(M_N* node,M_N* head){
    if(head == NULL)return pool_notuse_error;

    M_N* p1 = head;
    M_N* p2 = head->next;
    if(p1 == node){
        head = p2;
        return 1;        
    }

    while(p2 != NULL){
        if(p2 == node){
            p1->next = p2->next;
            return 1;
        }
        p1 = p1->next;
        p2 = p2->next;
    }
    return pool_notuse_error;
}

int hash_init(Hash* h,M_P* mp){
    h->size = hash_num;
    h->array_size = array_maxsize;
    Node* p[h->size];
    printf("?\n");
    get_memory(mp,p,sizeof(Node)*h->size);
    printf("?\n");
    int i = 0;
    for(i = 0;i < h->size;i++){
        p[i] = NULL;
    }
    h->array = p;
    return 1;
}

int hash(int key){
    return key%hash_num;
}

int hash_find(int key,Hash* h,int* val){
    if(h->array[hash(key)] == NULL)return hash_empty_error;
    if(h->array[hash(key)]->key == key) {
        *val = h->array[hash(key)]->value;
        return 1;
    }
    Node* p = h->array[hash(key)];
    while(p->next){
        if(p->key == key){
            *val = p->value;
            return 1;
        }
        p = p->next;
    }
    return hash_notfind_error;
}

int hash_insert(int key,int val,Hash* h,M_P* mp){
    Node* node;
    node = NULL;
    get_memory(mp,node,sizeof(*node));
    node->key = key;
    node->value = val;
    node->next = NULL;
    if(h->array[hash(key)] == NULL){
        h->array[hash(key)] == node;
        return 1;
    }
    else{
        Node* p1 = h->array[hash(key)];
        Node* p2 = p1->next;
        int num = 1;
        while(p2 != NULL){
            if(p2 -> key == key){
                back_memory(mp,node,sizeof(*node));
                return hash_exists_error;
            }
            p1 = p1->next;
            p2 = p1->next;
            num++;
        }
        if(num > array_maxsize){
            back_memory(mp,node,sizeof(*node));
            return hash_full_error;
        }
        else{
            p1->next = node;
            //node->next = p2;
            return 1;
        }
    }
    return;
}

int hash_delete(int key,int val,Hash* h,M_P* mp){
    if(h->array[hash(key)] == NULL) return hash_notfind_error;
    
    if(h->array[hash(key)]->key == key &&
     h->array[hash(key)]->value == val){
         Node* node = h->array[hash(key)];
         h->array[hash(key)] = node->next;
         back_memory(mp,node,sizeof(*node));
         return 1;
    }

    Node* p1 = h->array[hash(key)];
    Node* p2 = p1->next;
    while(p2 != NULL){
        if(p2->key == key && p2->value == val){
            p1->next = p2->next;
            back_memory(mp,p2,sizeof(Node));
            return 1;
        }
        p1 = p1->next;
        p2 = p1->next;
    }

    return hash_notfind_error;
}

int hash_print(Hash* h){
    int i = 0;
    for(i = 0;i < h->size;i++){
        Node* p = h->array[i];
        while(p != NULL){
            printf("key %d val %d\t",p->key,p->value);
            p = p->next;
        }
        printf("\n");
    }
    return 1;
}

















