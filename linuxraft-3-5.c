#include"raft.h"

#define port 12341
#define sock_max 1024

//#include<stdio.h>

pthread_mutex_t server_state;
pthread_mutex_t vote_count;

int main(){
    printf("--start at %ld--\n",get_ms()%simple_time); 
    Server s;
    server_init(&s);
    //printf("%d\n",sizeof(s.s_log));
    pthread_mutex_init(&server_state,NULL);
    pthread_mutex_init(&vote_count,NULL);
    pthread_t pid_listen,pid_election;   
    pthread_create(&pid_listen,NULL,(void*)listen_loop,&s);
    //pthread_create(&pid_election,NULL,(void*)election,(void*)&s);
    //pthread_join(pid_election,NULL);
    //char try[128];
    //memcpy(try,&s,sizeof(s));
    //election(try);
    //int num = 0;
    while(1){
        //printf("while %d term: %d\n",num++,s.currentterm);
        printf("my rand time %ld\n",get_rand_time());
        usleep(get_rand_time()*1000);
        pthread_mutex_lock(&server_state);
        long s_active = s.lastactivetime;
        if((get_ms() - s_active) > overtime){
            printf("%ld %d overtime start election\n",get_ms()%simple_time,port);
            election(&s); 
        }
        else{
            pthread_mutex_unlock(&server_state);
            //printf("s active %ld  %ld election sleep\n",s_active,get_ms());
            usleep((s_active + overtime -get_ms())*1000+little_wait);
            continue;
        }
        pthread_mutex_unlock(&server_state);
    }
    pthread_join(pid_listen,NULL);
    return 0;
}

int server_init(Server* s){
    memset(s,0,sizeof(s));
    s->id = port;
    s->role = follow;
    s->currentterm = 1;
    s->votedfor = none;
    s->lastincludeterm = s->currentterm;
    s->leaderid = empty;
    s->lastactivetime = get_ms();
    s->lastincludeindex = 0;
    s->lastincludeterm = 0;
    /*s->commitindex = 0;
    s->lastapplied = 0;
    s->lastincludeindex = 0;
    s->lastincludeterm = s->currentterm;
    s->leaderid = empty;
    s->lastactivetime = get_ms();
    memset(&s->matchindex,0,sizeof(s->matchindex));
    memset(&s->nextindex,0,sizeof(s->nextindex));
    memset(&s->s_log,0,sizeof(s->s_log));
    */
    return 0;
}

long get_ms(){
    long time_ms;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    time_ms = tv.tv_sec*1000+tv.tv_usec/1000;
    return time_ms;
}

/*int isovertime(Server* s){
    long time_now;
    time_now = get_ms();
    if(time_now - s->lastactivetime > overtime){
        printf("now %ld %d is overtime\n",get_ms(),port);
        return 1;
    }
    return 0;
}*/

/*int send_for_reply(int recv_port,char* request,char* reply){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    
    //printf("size %d\n",sizeof(request));
    write(sock,request,sizeof(request));
    //printf("send something at %d",)
    read(sock,reply,sizeof(reply)-1);
    return 0;
}*/

int send_heart(){

}

int listen_loop(void* argv){
    Server* s = (Server*)argv;
    int serv_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port);
    bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    listen(serv_sock,sock_max);
    int pid_n = 0;
    pthread_t pid[sock_max];
    while(1){
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(serv_sock,
        (struct sockaddr*)&clnt_addr,&clnt_addr_size);
        if(clnt_sock > 0){
            S_T st;
            st.serv = s;
            st.sock =  clnt_sock;
            st.sock_addr = clnt_addr;
            if(pthread_create(&pid[pid_n],NULL,(void*)process,(void*)&st)){
                printf("thread_create %d error\n",ntohs(clnt_addr.sin_port));
            }
            //pthread_join(pid[n],NULL);
            pid_n++;
            if(pid_n > sock_max){
                pid_n = 0;
            }
            //printf("thread %d\n",pid_n);
            /*char request[128];
            read(clnt_sock,request,sizeof(request));
            printf("accept port %d\n",ntohs(clnt_addr.sin_port));
            process(request);*/
        }
    }
}

void process(void* argv){
    char request[128];
    S_T st = *(S_T*)argv;
    printf("\nstart process sock %d\n",ntohs(st.sock_addr.sin_port));
    //printf("server message%d %d\n",st.serv.id,st.serv.votedfor);
    read(st.sock,request,sizeof(request));
    //int k = 0;
    //memcpy(&k,&request[sizeof(int)*2],sizeof(int));
    //printf("**recv %d**\n",k);
    //printf("**recv request %s**\n",request);
    int h = 0;
    //int temp_id = 4;
    memcpy((void*)&h,request,sizeof(h));
    /*
    int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        printf("**i:%d k:%d\n",i,k);
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/

    //memcpy(&temp_id,request+sizeof(int),sizeof(temp_id));
    //printf("h = %d id = %d",h,temp_id);
    if(h == isvs){
        V_s vs;
        memcpy(&vs,request,sizeof(vs));
        printf("recv voterequest from %d term %d ",vs.id,vs.term);
        printf("lastlogindex %d lastlogterm %d",vs.lastlogindex,vs.lastlogterm);
        printf("at %d\n",ntohs(st.sock_addr.sin_port));
        V_r vr;

        //printf("votefor:%d\n",st.serv.votedfor);
        pthread_mutex_lock(&server_state);
        process_vs(st.serv,&vs,&vr);
        pthread_mutex_unlock(&server_state);
        
        char reply[sizeof(vr)+1];
        memcpy(reply,&vr,sizeof(vr));
        /*
        int i,k;
        for(i = 0;i < sizeof(reply);){
            memcpy(&k,&reply[i],sizeof(int));
            printf("**i:%d k:%d\n",i,k);
            i += sizeof(int);
            if(i > 12){
                break;
            }
        }
        */
        write(st.sock,reply,sizeof(reply));
        printf("send %d vs result %d\n",vs.id,vr.votegranted);
        //printf("%d\n",ntohs(st.sock_addr.sin_port));
        close(st.sock);
        printf("%ld close\n\n",get_ms()%simple_time);
        return;
    }
    else if(h == iscs){
        C_s cs;
        memcpy(&cs,request,sizeof(cs));
        printf("recv copyrequest from %d term %d ",cs.id,cs.term);
        printf("prevlogindex %d prevlogterm %d",cs.prevlogindex,cs.prevlogterm);
        printf("at %d\n",ntohs(st.sock_addr.sin_port));
        C_r cr;
        
        pthread_mutex_lock(&server_state);
        process_cs(st.serv,&cs,&cr);
        pthread_mutex_unlock(&server_state);

        char reply[sizeof(cr)+1];
        memcpy(reply,&cr,sizeof(cr));
        write(st.sock,reply,sizeof(reply));
        printf("send %d cs result %d\n",cs.id,cr.success);
        //printf("%d\n",ntohs(st.sock_addr.sin_port));
        close(st.sock);
        printf("%ld close\n",get_ms()%simple_time);
        return;
    }
    else if(h == isclient){
        printf("client?\n");
    }
    else{
        printf("unknow request\n");
    }
    return;
}

int process_vs(Server* serv,V_s* vs,V_r* vr){
    printf("getlock\n");
        vr->h = isvr;
    vr->term = serv->currentterm;
    vr->votegranted = vote_false;
    serv->lastactivetime = get_ms();
    if(vs->term < serv->currentterm){
        //printf("term <\n");
        return;
    }    
    printf("my term %d votefor %d",serv->currentterm,serv->votedfor);
    if(vs->term > serv->currentterm){
        serv->votedfor = none;
    }
    if(serv->votedfor == none || serv->votedfor == vs->id){
        if(vs->lastlogindex >= serv->lastincludeindex){
            //printf("ok\n");
            serv->votedfor = vs->id;
            serv->lastactivetime = get_ms();
            serv->currentterm = vs->term; 
            vr->term = serv->currentterm;
            vr->votegranted = vote_ok;
            printf("now %ld vote ok %d lastactive%ld votefor %d\n",get_ms()%simple_time,vr->votegranted,serv->lastactivetime,serv->votedfor);
            return;
        }
        //printf("index <\n");
    }
    printf("have vote\n");
    return;
}

int process_cs(Server* serv,C_s* cs,C_r* cr){
    cr->h = iscr;
    cr->term = serv->currentterm;
    cr->success = copy_false;
    /*if(cs->id = serv->leaderid){
        serv->lastactivetime = get_ms();
    }*/
    if(cs->term < serv->currentterm){
        //cr->success = copy_false;
        return;
    }
    if(serv->s_log[cs->prevlogindex].term != cs->prevlogterm){
        return;
    }
    //chongtu add log != behand prev load log
    return;
}

/*int election(void* args){
        pthread_mutex_lock(&mutex);
        Server* s;
        memcpy(s,args,sizeof(s));
        
        if(isovertime(s) && s->role == follow){
            //pthread_mutex_lock(&mutex);
            printf("%d %d is overtime at %ld",port,s->role,get_ms);
            s->currentterm ++;
            s->votedfot = s->id;
            int i = 0;
            for(i = 0;i < server_num;i++){
                if(node[i] == s->id){
                    continue;
                }
                else{
                    V_s vs;
                    vs.h = isvs;
                    vs.id = s->id;
                    vs.lastlogindex = s->lastincludeindex;
                    vs.lastlogterm = s->lastincludeterm;
                    vs.term = s->currentterm;
                    char s_buf[32];
                    memcpy(s_buf,&vs,sizeof(s_buf));
                    char r_buf[128];
                    send_for_reply(node[i],s_buf,r_buf);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
        else{
            pthread_mutex_unlock(&mutex); 
            printf("in activetime\n");
            usleep(s->lastactivetime + overtime - get_ms());
        }
    return 0;
}*/

int election(Server* s){
    if(1/*s->role == follow*/){
        printf("%ld %dstatt election\n",get_ms()%simple_time,port);
        s->role = candidate;
        s->lastactivetime = get_ms();
        //s->role = candidate;
        s->votedfor = s->id;
        //printf("%d\n",s->currentterm);
        s->currentterm += 1;
        V_s vs;
        V_r vr;
        result re;
        vote_init(&re);
        int i = 0;
        char s_buf[128];
        get_vs(s,&vs);
        //printf("get %d now vs\n",vs.id);
        //s_buf[sizeof(vs)] = '\0';
        //printf("**size s_buf %d\n",sizeof(s_buf));//now size 128
        /*int k = 0;
        memcpy(&k,&s_buf[sizeof(int)*2],sizeof(int));
        printf("**send voterequest%d**\n",k);*/
        for(i = 0;i < server_num;i++){
            if(node[i] == port){
                continue;
            }
            send_vs(node[i],&vs,&vr);//in size 8
            if(vr.votegranted == vote_ok){
                vote_get(&re);
                printf("get %d vote\n",node[i]);
            }
            else{
                vote_lose(&re);
                printf("lose %d vote\n",node[i]);
                //s->votedfor = none;
            }
        }
        if(!(re.ok+1) < majority){
            //pthread_mutex_lock(&server_state);
            s->role = leader;
            s->lastactivetime = get_ms();
            printf("%d become leader!\n",s->id);
            for(i = 0;i < server_num;i++){
                if(node[i] == port){
                    continue;
                }
                C_s cs;
                get_cs(s,&cs,node[i]);
                C_r cr;
                send_cs(node[i],&cs,&cr);
            }
            //pthread_mutex_unlock(&server_state);
            return;
        }
    }
}

int vote_init(result* re){
    pthread_mutex_lock(&vote_count);
    re->finish = 0;
    re->ok = 0;
    pthread_mutex_unlock(&vote_count);
    return 0;
}

int vote_get(result* re){
    pthread_mutex_lock(&vote_count);
    re->finish++;
    re->ok++;
    pthread_mutex_unlock(&vote_count);
    return 0;
}

int vote_lose(result* re){
    pthread_mutex_lock(&vote_count);
    re->finish++;
    pthread_mutex_unlock(&vote_count);
    return 0;
}

int get_vs(Server* s,V_s* vs){
    vs->h = isvs;
    vs->id = s->id;
    /*vs->lastlogindex = sizeof(s->s_log)-1;
    if(sizeof(s->s_log) == 0){
        vs->lastlogterm = 0;
    }
    else{
        vs->lastlogterm = s->s_log[sizeof(s->s_log)-1].term;
    }*/
    vs->lastlogindex = s->lastincludeindex;
    vs->lastlogterm = s->lastincludeterm;
    vs->term = s->currentterm;
    printf("get %d now vs lastlogindex: %d lastlogterm: %d term: %d\n"
    ,vs->id,vs->lastlogindex,vs->lastlogterm,vs->term);
}

int get_cs(Server* s,C_s* cs,int goal_port){
    cs->h = iscs;
    cs->id = s->id;
    cs->prevlogindex = s->nextindex[port-port_start] - 1;
    cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    printf("get %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d\n"
    ,cs->id,cs->prevlogindex,cs->prevlogterm,cs->term,cs->commit);
}

int get_hreat(Server* s,C_s* cs,int goal_port){
    cs->h = iscs;
    cs->id = s->id;
    cs->prevlogindex = s->nextindex[port-port_start] - 1;
    cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    return;
}

int send_vs(int recv_port,V_s* vs,V_r* vr){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    /*int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        printf("**i:%d k:%d\n",i,k);
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/
    //printf("size %d\n",sizeof(request));
    char request[128];
    char reply[16];
    memcpy(request,vs,sizeof(*vs));
    write(sock,request,sizeof(request));
    //printf("send something at %d",)
    read(sock,reply,sizeof(reply));
    /*
    int i,k;
        for(i = 0;i < sizeof(reply);){
            memcpy(&k,&reply[i],sizeof(int));
            printf("**i:%d k:%d\n",i,k);
            i += sizeof(int);
            if(i > 12){
                break;
            }
        }
    */
    memcpy(vr,reply,sizeof(*vr));
    //printf("recv %d result %d\n",recv_port,vr->votegranted);
    return 0;
}

int send_cs(int recv_port,C_s* cs,C_r* cr){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    /*int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        printf("**i:%d k:%d\n",i,k);
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/
    //printf("size %d\n",sizeof(request));
    char request[128];
    char reply[16];
    memcpy(request,cs,sizeof(*cs));
    write(sock,request,sizeof(request));
    //printf("send something at %d",)
    read(sock,reply,sizeof(reply));
    memcpy(cr,reply,sizeof(*cr));
    return 0;
}

int load_log_append(Server* s){

}

/* //lock problem  in thread lock agaiin
int clock(Server* s){
    while(1){
        pthread_mutex_lock(&server_state);
        long s_active = s->lastactivetime;
        if(s->role == leader){
            if(s->lastactivetime + leader_copy_time < get_ms()){
                log_copy(s);//open thread?
            }
            else{
                pthread_mutex_unlock(&server_state);
                usleep((s_active + leader_copy_time -get_ms())*1000);
            }
        }
        if(s->role == follow){
            if(s->lastactivetime + overtime < get_ms()){
                election(s);//openthread?
            }
            else{
                pthread_mutex_unlock(&server_state);
                usleep((s_active + overtime -get_ms())*1000);
            }
        }
        pthread_mutex_unlock(&server_state);
    }
}
*/

int log_copy(Server* s){
    int i = 0;
    for(i = 0;i < server_num;i++){

    }    
}

int leader_loop(void* argv){
    while(1){
        pthread_mutex_lock(&server_state);
        Server s = *(Server*)argv;
        if(s.role == leader){
            if(s.lastactivetime + leader_copy_time < get_ms()){
                C_s cs;
                C_r cr;
                //get_cs(&s,&cs); //goal_port
                //send_cs(&cs,&cr); //every port
            }
        }
        pthread_mutex_unlock(&server_state);
    }
    return;
}

int self_updata(void* argv){
    pthread_mutex_lock(&server_state);


    pthread_mutex_unlock(&server_state);
    return;
}

/*
long get_rand_time(){
    srand((unsigned)time(NULL));
    return rand()%300+150;
}
*/

long get_us(){
    long time_us;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    time_us = tv.tv_usec;
    return time_us;
}

long get_rand_time(){
    srand(get_us());
    return rand()%300+150;
}