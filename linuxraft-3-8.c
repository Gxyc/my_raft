#include"raft.h"

#define port 12341
#define debug 1
//#include<stdio.h>

pthread_mutex_t server_state;
pthread_mutex_t vote_count;

int main(){
    if(debug){printf("--start at %ld--\n",get_ms()%simple_time); }
    Server s;
    server_init(&s);
    //if(debug){printf("%d\n",sizeof(s.s_log));}
    pthread_mutex_init(&server_state,NULL);
    pthread_mutex_init(&vote_count,NULL);
    pthread_t pid_listen,pid_election,pid_leader;   
    pthread_create(&pid_listen,NULL,(void*)listen_loop,&s);
    pthread_create(&pid_leader,NULL,(void*)leader_loop,&s);
    //pthread_create(&pid_election,NULL,(void*)election,(void*)&s);
    //pthread_join(pid_election,NULL);
    //char try[128];
    //memcpy(try,&s,sizeof(s));
    //election(try);
    //int num = 0;
    while(1){
        //if(debug){printf("while %d term: %d\n",num++,s.currentterm);}
        //if(debug){printf("my time %ld\n",get_ms()%simple_time);}
        int time_rand = get_rand_time();
        if(debug){printf("my rand time %ld\n",time_rand);}
        usleep(time_rand*1000);
        //if(debug){printf("my time %ld\n",get_ms()%simple_time);}
        pthread_mutex_lock(&server_state);
        long s_active = s.lastactivetime;
        if((get_ms() - s_active) > overtime){
            if(debug){printf("%ld %d overtime start election\n",get_ms()%simple_time,port);}
            election(&s); 
        }
        else{
            pthread_mutex_unlock(&server_state);
            //if(debug){printf("s active %ld  %ld election sleep\n",s_active,get_ms());}
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
    s->log_num = 0;
    int i = 0;
    for(i = 0;i < server_num;i++){
        s->nextindex[i] = 1;
        s->matchindex[i] = 0;
    }
    s->s_log[0].term = 0;
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
        if(debug){printf("now %ld %d is overtime\n",get_ms(),port);}
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
    
    //if(debug){printf("size %d\n",sizeof(request));}
    write(sock,request,sizeof(request));
    //if(debug){printf("send something at %d",)}
    read(sock,reply,sizeof(reply)-1);
    return 0;
}*/

/*int send_heart(){

}*/

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
                if(debug){printf("thread_create %d error\n",ntohs(clnt_addr.sin_port));}
            }
            //pthread_join(pid[n],NULL);
            pid_n++;
            if(pid_n > sock_max){
                pid_n = 0;
            }
            //if(debug){printf("thread %d\n",pid_n);}
            /*char request[128];
            read(clnt_sock,request,sizeof(request));
            if(debug){printf("accept port %d\n",ntohs(clnt_addr.sin_port));}
            process(request);*/
        }
    }
}

void process(void* argv){
    char request[128];
    S_T st = *(S_T*)argv;
    if(debug){printf("\nstart process sock %d thread\n",ntohs(st.sock_addr.sin_port));}
    //if(debug){printf("server message%d %d\n",st.serv.id,st.serv.votedfor);}
    read(st.sock,request,sizeof(request));
    //int k = 0;
    //memcpy(&k,&request[sizeof(int)*2],sizeof(int));
    //if(debug){printf("**recv %d**\n",k);}
    //if(debug){printf("**recv request %s**\n",request);}
    int h = 0;
    //int temp_id = 4;
    memcpy((void*)&h,request,sizeof(h));
    /*
    int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        if(debug){printf("**i:%d k:%d\n",i,k);}
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/

    //memcpy(&temp_id,request+sizeof(int),sizeof(temp_id));
    //if(debug){printf("h = %d id = %d",h,temp_id);}
    if(h == isvs){
        V_s vs;
        memcpy(&vs,request,sizeof(vs));
        if(debug){printf("recv voterequest from %d term %d ",vs.id,vs.term);}
        if(debug){printf("lastlogindex %d lastlogterm %d",vs.lastlogindex,vs.lastlogterm);}
        if(debug){printf("at %d\n",ntohs(st.sock_addr.sin_port));}
        V_r vr;

        //if(debug){printf("votefor:%d\n",st.serv.votedfor);}
        pthread_mutex_lock(&server_state);
        process_vs(st.serv,&vs,&vr);
        pthread_mutex_unlock(&server_state);
        
        char reply[sizeof(vr)+1];
        memcpy(reply,&vr,sizeof(vr));
        /*
        int i,k;
        for(i = 0;i < sizeof(reply);){
            memcpy(&k,&reply[i],sizeof(int));
            if(debug){printf("**i:%d k:%d\n",i,k);}
            i += sizeof(int);
            if(i > 12){
                break;
            }
        }
        */
        write(st.sock,reply,sizeof(reply));
        if(debug){printf("send %d vs result %d\n",vs.id,vr.votegranted);}
        //if(debug){printf("%d\n",ntohs(st.sock_addr.sin_port));}
        close(st.sock);
        if(debug){printf("time %ld close\n\n",get_ms()%simple_time);}
        return;
    }
    else if(h == iscs){
        C_s cs;
        memcpy(&cs,request,sizeof(cs));
        //if(debug){printf("recv copyrequest from %d term %d ",cs.id,cs.term);}
        //if(debug){printf("prevlogindex %d prevlogterm %d",cs.prevlogindex,cs.prevlogterm);}
        //if(debug){printf("at %d\n",ntohs(st.sock_addr.sin_port));}
        C_r cr;
        
        pthread_mutex_lock(&server_state);
        process_cs(st.serv,&cs,&cr);
        pthread_mutex_unlock(&server_state);

        char reply[sizeof(cr)+1];
        memcpy(reply,&cr,sizeof(cr));
        write(st.sock,reply,sizeof(reply));
        if(debug){printf("send %d cs result %d\n",cs.id,cr.success);}
        //if(debug){printf("%d\n",ntohs(st.sock_addr.sin_port));}
        close(st.sock);
        if(debug){printf("%ld close\n",get_ms()%simple_time);}
        return;
    }
    else if(h == isclient){
        pthread_mutex_lock(&server_state);

        if(debug){printf("client?\n");}
        Reply_C reply;
        if(st.serv->role == leader){
            Request_C c;
            memcpy(&c,request,sizeof(c));
            if(c.log.op == client_read){
                int val = 0;
                if(find(st.serv,c.log.key,&val)){
                    reply.h = client_ok;
                    reply.result = val;
                }
                else{
                    reply.h = client_notfind;
                }
            }
            else if(c.log.op == client_write){
                int re_ap_c = load_log_append(st.serv,&c.log,1);
                if(re_ap_c > 0){//??if log be delete in leader change??what should do??
                    pthread_mutex_unlock(&server_state);
                    while(1){
                        pthread_mutex_lock(&server_state);
                        if(st.serv->role != leader){
                            reply.h = server_role_wrong;
                            pthread_mutex_unlock(&server_state);
                            break;
                        }
                        if(st.serv->lastapplied > re_ap_c){
                            reply.h = client_ok;
                            pthread_mutex_unlock(&server_state);
                            break;
                        }
                        pthread_mutex_unlock(&server_state);
                    }
                }
                else{
                    reply.h = server_append_wrong;
                }
            }
            else if(c.log.op == client_delete){

            }
            else if(c.log.op == client_updata){

            }
            else{
                reply.h = client_unrecognized;
            }
            char buf[128];
            memcpy(buf,&reply,sizeof(reply));
            write(st.sock,buf,sizeof(buf));
            close(st.sock);
            return;
        }
        else{
            if(debug){printf("not leader,redirect\n");}
            reply.h = client_redirect;
            reply.result = st.serv->leaderid;
            char buf[128];
            memcpy(buf,&reply,sizeof(reply));
            write(st.sock,buf,sizeof(buf));
            close(st.sock);
            return;
        }

        pthread_mutex_unlock(&server_state);
    }
    else{
        if(debug){printf("unknow request\n");}
    }
    return;
}

int process_vs(Server* serv,V_s* vs,V_r* vr){
    //if(debug){printf("getlock\n");}
        vr->h = isvr;
    vr->term = serv->currentterm;
    vr->votegranted = vote_false;
    serv->lastactivetime = get_ms();
    if(vs->term < serv->currentterm){
        //if(debug){printf("term <\n");}
        return;
    }    
    if(debug){printf("my term %d votefor %d",serv->currentterm,serv->votedfor);}
    if(vs->term > serv->currentterm){
        serv->role = follow;
        serv->votedfor = none;
        serv->lastactivetime = get_ms();
    }
    if(serv->votedfor == none || serv->votedfor == vs->id){
        if(vs->lastlogindex >= serv->lastincludeindex){
            //if(debug){printf("ok\n");}
            serv->votedfor = vs->id;
            serv->lastactivetime = get_ms();
            serv->currentterm = vs->term; 
            vr->term = serv->currentterm;
            vr->votegranted = vote_ok;
            if(debug){printf("now %ld vote ok %d lastactive%ld votefor %d\n",get_ms()%simple_time,vr->votegranted,serv->lastactivetime,serv->votedfor);}
            return;
        }
        //if(debug){printf("index <\n");}
    }
    if(debug){printf("have vote\n");}
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
        cr->success = cr_error_term;
        return;
    }

    if(cs->term == serv->currentterm && cs->id == serv->leaderid){
        serv->lastactivetime = get_ms();
    }
    
    if(cs->term > serv->currentterm){
        serv->role = follow;
        serv->leaderid = cs->id;
        serv->currentterm = cs->term;
        serv->lastactivetime = get_ms();
    }

    if(serv->s_log[cs->prevlogindex].term != cs->prevlogterm){
        //serv->currentterm = cs->term;
        cr->success = cr_error_dismatch;
        if(debug){printf("log mismatch my log index %d term is %d\n",cs->prevlogindex,serv->s_log[cs->prevlogindex].term);}
        return;
    }
    //if(debug){printf("logsize %d\n",sizeof(cs->send_log));}
    if(cs->log_num == 0){
        //serv->currentterm = cs->term;
        cr->success = copy_ok;
        serv->lastactivetime = get_ms();
        if(cs->commit > serv->commitindex){
            serv->commitindex = min(cs->commit,cs->prevlogindex);
        }
        return;
    }
    else{
        if(debug){printf("recv copyrequest from %d term %d ",cs->id,cs->term);}
        if(debug){printf("prevlogindex %d prevlogterm %d ",cs->prevlogindex,cs->prevlogterm);}
        if(debug){printf("log_num %d \n",cs->log_num);}
        copy_log(cs,serv);
    }
    /*if(cs->commit > serv->commitindex){
        serv->commitindex = min(cs->commit,serv->lastincludeindex);
    }*/
    //chongtu add log != behand prev load log
    return;
}

int copy_log(C_s* cs, Server* s){  
    int i = 0;
    for(i = 0;i < cs->log_num;i++){
        s->s_log[cs->prevlogindex + i + 1] = cs->send_log[i];
    }
    s->log_num += (i-1);
    s->lastincludeindex += (i-1);
    s->lastincludeterm = cs->send_log[i-1].term;
    s->lastactivetime = get_ms();
}

/*int election(void* args){
        pthread_mutex_lock(&mutex);
        Server* s;
        memcpy(s,args,sizeof(s));
        
        if(isovertime(s) && s->role == follow){
            //pthread_mutex_lock(&mutex);
            if(debug){printf("%d %d is overtime at %ld",port,s->role,get_ms);}
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
            if(debug){printf("in activetime\n");}
            usleep(s->lastactivetime + overtime - get_ms());
        }
    return 0;
}*/

int election(Server* s){
    if(1/*s->role == follow*/){
        if(debug){printf("%ld %dstatt election\n",get_ms()%simple_time,port);}
        s->role = candidate;
        s->lastactivetime = get_ms();
        //s->role = candidate;
        s->votedfor = s->id;
        //if(debug){printf("%d\n",s->currentterm);}
        s->currentterm += 1;
        V_s vs;
        V_r vr;
        result re;
        vote_init(&re);
        int i = 0;
        char s_buf[128];
        get_vs(s,&vs);
        //if(debug){printf("get %d now vs\n",vs.id);}
        //s_buf[sizeof(vs)] = '\0';
        //if(debug){printf("**size s_buf %d\n",sizeof(s_buf));//now size 128}
        /*int k = 0;
        memcpy(&k,&s_buf[sizeof(int)*2],sizeof(int));
        if(debug){printf("**send voterequest%d**\n",k);}*/
        for(i = 0;i < server_num;i++){
            if(node[i] == port){
                continue;
            }
            int v = send_vs(node[i],&vs,&vr);//in size 8
            if(v){
                if(debug){printf("error connect %d when send vs\n",node[i]);}
            }
            else{
                if(vr.term > s->currentterm){
                    s->role = follow;
                    s->currentterm = vr.term;
                    s->votedfor = none;
                    s->leaderid = node[i];
                    s->lastactivetime = get_ms();
                    return;
                }
                if(vr.votegranted == vote_ok){
                    vote_get(&re);
                    if(debug){printf("get %d vote\n",node[i]);}
                }
                else{
                    vote_lose(&re);
                    if(debug){printf("lose %d vote\n",node[i]);}
                    //s->votedfor = none;
                }
            }
        }
        if((re.ok+1) > majority){
            if(debug){printf("my vote %d majority %d \n",re.ok,majority);}
            //pthread_mutex_lock(&server_state);
            s->role = leader;
            s->lastactivetime = get_ms();
            if(debug){printf("%d become leader!\n",s->id);}
            for(i = 0;i < server_num;i++){
                if(node[i] == port){
                    continue;
                }
                C_s cs;
                //if(debug){printf("error 1\n");}
                get_hreat(s,&cs,node[i]);//error
                C_r cr;
                //if(debug){printf("error 2\n");}
                int c = send_cs(node[i],&cs,&cr);
                //cr.term > myterm ? beleader : ?
                if(c){
                    if(debug){printf("error connect %d when  send cs\n",node[i]);}
                }   
            }
            //pthread_mutex_unlock(&server_state);
            return;
        }
        else{
            if(debug){printf("%d lose election\n",port);}
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
    if(debug){printf("get %d now vs lastlogindex: %d lastlogterm: %d term: %d\n"
    ,vs->id,vs->lastlogindex,vs->lastlogterm,vs->term);}
}

int get_cs(Server* s,C_s* cs,int goal_port){
    cs->h = iscs;
    cs->id = s->id;
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    /*if((s->nextindex[port-port_start] - 1) < 0){
        //if(debug){printf("?1\n");}
        cs->prevlogindex = 0;
        cs->prevlogterm = 0;
    }
    else{
        //if(debug){printf("?2\n");}
        cs->prevlogindex = s->nextindex[port-port_start] - 1;
        cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    }
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    */
   if(s->matchindex[goal_port - port_start] == s->log_num){
       get_hreat(s,cs,goal_port);
       return;
   }
   else{
       if(debug){printf("copying log...\n");}
       if(s->log_num - s->matchindex[goal_port - port_start] > 5){
           cs->log_num = 5;
           int i = 0;
            for(i = 0;i < 5;i++){
                memcpy(&cs->send_log[i],&s->s_log[s->nextindex[goal_port - port_start]+ i - 1],sizeof(Log));
            }
       }
       else{
           cs->log_num = s->log_num - s->matchindex[goal_port - port_start];
            int n = s->log_num - s->matchindex[goal_port - port_start];
            int i = 0;
            for(i = 0;i < n;i++){
                memcpy(&cs->send_log[i],&s->s_log[s->nextindex[goal_port - port_start]+ i - 1],sizeof(Log));
            }
        }
   }
    if(debug){printf("get %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d log_num %d\n"
    ,cs->id,cs->prevlogindex,cs->prevlogterm,cs->term,cs->commit,cs->log_num);}
}

int get_hreat(Server* s,C_s* cs,int goal_port){
    //if(debug){printf("?\n");}
    cs->h = iscs;
    cs->id = s->id;
    //if(debug){printf("?\n");}
    cs->log_num = 0;
    if(debug){printf("%d nextindex is %d \n",goal_port,s->nextindex[goal_port - port_start]);}
    if((s->nextindex[goal_port-port_start] - 1) == 0){
        //if(debug){printf("?1\n");}
        cs->prevlogindex = 0;
        cs->prevlogterm = 0;
    }
    else{
        //if(debug){printf("?2\n");}
        cs->prevlogindex = s->nextindex[goal_port-port_start] - 1;
        cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    }
    //if(debug){printf("?\n");}
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    if(debug){printf("get %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d log_num %d\n"
    ,cs->id,cs->prevlogindex,cs->prevlogterm,cs->term,cs->commit,cs->log_num);}
    return;
}

int send_vs(int recv_port,V_s* vs,V_r* vr){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))){
        return error_connect;
    }
    /*int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        if(debug){printf("**i:%d k:%d\n",i,k);}
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/
    //if(debug){printf("size %d\n",sizeof(request));}
    char request[128];
    char reply[16];
    memcpy(request,vs,sizeof(*vs));
    write(sock,request,sizeof(request));
    //if(debug){printf("send something at %d",)}
    read(sock,reply,sizeof(reply));
    /*
    int i,k;
        for(i = 0;i < sizeof(reply);){
            memcpy(&k,&reply[i],sizeof(int));
            if(debug){printf("**i:%d k:%d\n",i,k);}
            i += sizeof(int);
            if(i > 12){
                break;
            }
        }
    */
    memcpy(vr,reply,sizeof(*vr));
    //if(debug){printf("recv %d result %d\n",recv_port,vr->votegranted);}
    return 0;
}

int send_cs(int recv_port,C_s* cs,C_r* cr){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(recv_port);
    if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))){
        return error_connect;
    }
    /*int i,k;
    for(i = 0;i < sizeof(request);){
        memcpy(&k,&request[i],sizeof(int));
        if(debug){printf("**i:%d k:%d\n",i,k);}
        i += sizeof(int);
        if(i > 20){
            break;
        }
    }*/
    //if(debug){printf("size %d\n",sizeof(request));}
    char request[128];
    char reply[16];
    memcpy(request,cs,sizeof(*cs));
    write(sock,request,sizeof(request));
    //if(debug){printf("send something at %d",)}
    read(sock,reply,sizeof(reply));
    memcpy(cr,reply,sizeof(*cr));
    return 0;
}

int load_log_append(Server* s,Log* log,int num){
    int i = 0;
    for(i = 0;i < num;i++){
        if(s->log_num < load_log_maxsize){
            s->s_log[s->log_num] = log[i];
            s->log_num++;
        }
        else{
            break;
        }
    }
    return s->log_num;
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

/*int log_copy(Server* s){
    int i = 0;
    for(i = 0;i < server_num;i++){

    }    
}*/

int leader_loop(void* argv){
    while(1){
        pthread_mutex_lock(&server_state);
        Server* s = (Server*)argv;
        if(s->role == leader){
            if(s->lastactivetime + leader_copy_time < get_ms()){
                C_s cs;
                C_r cr;
                int i = 0;
                for(i = 0;i < server_num;i++){
                    if(node[i] == port){
                        continue;
                    }
                    get_cs(s,&cs,node[i]);
                    int re = send_cs(node[i],&cs,&cr);
                    if(re = error_connect){
                        if(debug){printf("error connect when copy log %d\n",node[i]);}
                        continue;
                    }
                    if(cr.success == cr_error_term){
                        if(s->currentterm < cr.term){
                            s->role = follow;
                            s->currentterm = cr.term;
                            break;
                        }
                    }
                    else if(cr.success == cr_error_dismatch){
                        s->nextindex[i] --;
                        s->matchindex[i] --;
                    }
                    else if(cr.success == copy_ok){
                        s->nextindex[i] += cs.log_num;
                        s->matchindex[i] += cs.log_num;
                    }
                }
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
    Server* s = (Server*)argv;
    if(s->commitindex > s->lastapplied){
        if(log_apply(s,s->lastapplied)){
            s->lastapplied ++;
        }
    }

    pthread_mutex_unlock(&server_state);
    return;
}

int log_apply(Server* s,int index){
    //ok return 1
    //wrong return 0
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
    return rand()%150+150;
}

int find(Server* s,int key,int* val){
    
    return 0;
}

int min(int a,int b){
    return a < b ?  a : b;
}

int max(int a,int b){
    return a > b ? a : b;
}
