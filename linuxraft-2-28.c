#include"raft.h"

#define port 12341
#define sock_max 1024

//#include<stdio.h>

pthread_mutex_t mutex;
pthread_mutex_t vote;

int main(){
    printf("start at %ld\n",get_ms); 
    Server s;
    server_init(&s);
    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_init(&vote,NULL);
    pthread_t pid_listen,pid_election;   
    pthread_create(&pid_listen,NULL,(void*)listen_loop,NULL);
    //pthread_create(&pid_election,NULL,(void*)election,(void*)&s);
    //pthread_join(pid_election,NULL);
    //char try[128];
    //memcpy(try,&s,sizeof(s));
    //election(try);
    //int num = 0;
    while(1){
        //printf("while %d term: %d\n",num++,s.currentterm);
        pthread_mutex_lock(&mutex);
        long s_active = s.lastactivetime;
        if((get_ms() - s_active) > overtime){
            printf("%d overtime start election\n",port);
            election(&s); 
        }
        else{
            pthread_mutex_unlock(&mutex);
            printf("s active %ld  %ld sleep\n",s_active,get_ms());
            usleep((s_active + overtime -get_ms())*1000+little_wait);
            continue;
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_join(pid_listen,NULL);
    return 0;
}

int server_init(Server* s){
    s->id = port;
    s->role = follow;
    s->currentterm = 1;
    s->votedfor = empty;
    s->commitindex = 0;
    s->lastapplied = 0;
    s->leaderid = empty;
    s->lastactivetime = get_ms();
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

int send_for_reply(int recv_port,char* request,char* reply){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    write(sock,request,sizeof(request));
    //printf("send something at %d",)
    read(sock,reply,sizeof(reply)-1);
    return 0;
}

int send_heart(){

}

int listen_loop(/*char* request*/){
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
    printf("start process sock %d\n",ntohs(st.sock_addr.sin_port));
    read(st.sock,request,sizeof(request));
    /*while(1){
        printf("%d ",ntohs(st.sock_addr.sin_port));
        printf("%s\n",request);
        sleep(1);
    }*/
    int h = 0;
    memcpy((void*)&h,request,sizeof(h));
    if(h == isvs){
        V_s vs;
        memcpy(&vs,request,sizeof(vs));
        printf("recv voterequest from %d term %d ",vs.id,vs.term);
        printf("lastlogindex %d lastlogterm %d",vs.lastlogindex,vs.lastlogterm);
        printf("at %d\n",ntohs(st.sock_addr.sin_port));
        V_r vr;
        process_vs(&st.serv,&vs,&vr);
        char reply[sizeof(vr)+1];
        memcpy(reply,&vr,sizeof(vr));
        write(st.sock,reply,sizeof(reply));
        printf("send %d result %d\n",vs.id,vr.votegranted);
        //printf("%d\n",ntohs(st.sock_addr.sin_port));
        close(st.sock);
        printf("%ld close\n",get_ms());
        return;
    }
    else if(h == iscs){
        C_s cs;
        memcpy(&cs,request,sizeof(cs));
        printf("recv copyrequest from %d term %d ",cs.id,cs.term);
        printf("prevlogindex %d prevlogterm %d",cs.prevlogindex,cs.prevlogterm);
        printf("at %d\n",ntohs(st.sock_addr.sin_port));
        C_r cr;
        process_cs(&st.serv,&cs,&cr);
        char reply[sizeof(cr)+1];
        memcpy(reply,&cr,sizeof(cr));
        write(st.sock,reply,sizeof(reply));
        printf("send %d result %d\n",cs.id,cr.success);
        //printf("%d\n",ntohs(st.sock_addr.sin_port));
        close(st.sock);
        printf("%ld close\n",get_ms());
        return;
    }
    else if(h == isclient){
        printf("client?\n");
    }
    else{
        printf("unkmow request\n");
    }
    return;
}

int process_vs(Server* serv,V_s* vs,V_r* vr){
    vr->h = isvr;
    vr->term = serv->currentterm;
    vr->votegranted = vote_false;
    if(vs->term < serv->currentterm){
        return;
    }    
    if(serv->votedfor == empty || serv->votedfor == vs->id){
        if(vs->lastlogindex > sizeof(serv->s_log)-2){
            serv->currentterm = vs->term; 
            vr->term = serv->currentterm;
            vr->votegranted = vote_ok;
            return;
        }
    }
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
    if(s->role == follow){
        //printf("ready election\n");
        s->lastactivetime = get_ms();
        //s->role = candidate;
        s->votedfor = s->id;
        //printf("%d\n",s->currentterm);
        s->currentterm += 1;
        V_s vs;
        result re;
        vote_init(&re);
        int i = 0;
        char s_buf[128];
        char r_buf[32];
        get_vs(s,&vs);
        memcpy(s_buf,&vs,sizeof(vs));
        for(i = 0;i < server_num;i++){
            if(node[i] == port){
                continue;
            }
            send_for_reply(node[i],s_buf,r_buf);
        }
    }
}

int vote_init(result* re){
    pthread_mutex_lock(&vote);
    re->finish = 0;
    re->ok = 0;
    pthread_mutex_unlock(&vote);
    return 0;
}

int vote_get(result* re){
    pthread_mutex_lock(&vote);
    re->finish++;
    re->ok++;
    pthread_mutex_unlock(&vote);
    return 0;
}

int vote_lose(result* re){
    pthread_mutex_lock(&vote);
    re->finish++;
    pthread_mutex_unlock(&vote);
    return 0;
}

int get_vs(Server* s,V_s* vs){
    vs->h = isvs;
    vs->id = s->id;
    vs->lastlogindex = s->lastincludeindex;
    vs->lastlogterm = s->lastincludeterm;
    vs->term = s->currentterm;
    printf("get %d now vs lastlogindex: %d lastlogterm: %d term: %d\n"
    ,vs->id,vs->lastlogindex,vs->lastlogterm,vs->term);
}

int get_cs(Server* s,C_s* cs){
    cs->h = iscs;
    cs->id = s->id;
    cs->prevlogindex = s->nextindex[port-port_start] - 1;
    cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    printf("get %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d\n"
    ,cs->id,cs->prevlogindex,cs->prevlogterm,cs->term,cs->commit);
}









