#include"mp.h"

#define debug_mp 1


int mp_init(M_P* mp,int size){
    mp->size = size;
    mp->p = malloc(size);
    mp->free_list = mp->p;

    M_N* mn = mp->p;
    int i = 0;
    for(i = 0;i < mp->size/data_size - 1;i++){
        mn->next = (M_N*)((char*)mn + data_size);
        //printf("i:%d p:%d next:%d\n",i,mn,mn->next);
        mn = mn->next;
    }
    mn->next = NULL;
}

void* mp_getmem(M_P* mp){
    if(mp->free_list == NULL){
        //if(mp_extend(&mp,mp_extend_size) != 0)
        return NULL;
        
    }
    void* p = mp->free_list;
    mp->free_list = mp->free_list->next;
    return p;
}

int mp_backmem(M_P* mp,char* p){
    if((void*)p < mp->p || (void*)p >= mp->p + mp->size){
        //free(p);
        if(debug_mp)printf("back error\n");
        return out_mp_error;
    }
    M_N* mn = (M_N*)p;
    mn->next = mp->free_list;
    mp->free_list = mn;
    return 1;
}

/*int mp_extend(M_P* mp,int size){
    if(mp->size + size > mp_max_size)
        return mp_outsize_error;
    void* x = mp->p;
    mp->p = malloc(mp->size + size);
    if(mp->p == NULL){
        mp->p = x;
        return malloc_error;
    }
    mp->size = mp->size + size;
    mp->free_list = mp->p;

    M_N* mn = mp->p;
    int i = 0;
    for(i = 0;i < mp->size/data_size - 1;i++){
        mn->next = (char*)mn + 8;
        //printf("i:%d p:%d next:%d\n",i,mn,mn->next);
        mn = mn->next;
    }

    mn->next = NULL;

    free(x);
    
    return 0;
}*/

int mp_destroy(M_P* mp){
    if(mp->free_list != NULL){
        return mp_using_error;
    }
    //printf("ddd");
    free(mp->p);
    return 1;
}

int mp_force(M_P* mp){
    free(mp->p);
    return 1;
}

int mp_print(M_P* mp){
    M_N* p = mp->free_list;
    while(p != NULL){
        printf("%d\t",p->data);
        p = p->next;
    }
    return 0;
}


/* //mp_test
int main(){
    M_P mp;
    mp_init(&mp,32);
    printf("mp.p:%d mp.free%d\n",mp.p,mp.free_list);
    mp_print(&mp);
    printf("\n");
    int* a,*b,*c;
    printf("before a %d\n",a);
    a = (int*)mp_getmem(&mp);
    b = (int*)mp_getmem(&mp);
    //*a = 0;
    printf("after a %d\n",a);
    mp_print(&mp);
    c = (int*)mp_getmem(&mp);
    printf("?\n");
    mp_backmem(&mp,(char*)a);
    mp_backmem(&mp,(char*)b);
    mp_print(&mp);
    //int* d;
    printf("\n");
    //mp_backmem(&mp,d);
    if(mp_destroy(&mp) != 1){
        printf("using\n");
        mp_force(&mp);
    }

    return 0;
}
*/
