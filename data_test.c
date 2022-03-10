#include"data.h"

int main(){
    M_P mp;
    
    int n = memory_pool_init(&mp,pool_max_size);
    printf("%d\n",n);

    int a;
    get_memory(&mp,&a,sizeof(int));
    printf("get a\n");

    Hash h;
    n = hash_init(&h,&mp);
    printf("get hash\n",n);

    n = hash_insert(1,36,&h,&mp);
    //printf("%d\n",n);

    int re = 0;    
    n = hash_find(1,&h,&re);
    //printf("%d\n",n);

    //printf("%d\n",re);
    
    n = hash_print(&h);
    //printf("%d\n",n);

    memory_pool_destroy(&mp);
}
