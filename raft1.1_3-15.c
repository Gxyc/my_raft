#include"raft.h"
#include"data1.h"

#define port 12341
#define debug1 0
#define debug2 1
#define debug_lock 0
//#include<stdio.h>

pthread_mutex_t server_state;
pthread_mutex_t vote_count;


Hash hash_h;  //need free

int main(){
    hash_init(&hash_h);
    if(debug1){printf("--start at %ld--\n",get_ms()%simple_time); }
    Server s;
    server_init(&s);
    pthread_mutex_init(&server_state,NULL);
    pthread_mutex_init(&vote_count,NULL);
    pthread_t pid_listen,pid_election,pid_leader,pid_updata;
    pthread_t pid_check,pid_heart;   
    pthread_create(&pid_listen,NULL,(void*)listen_loop,&s);
    //pthread_create(&pid_leader,NULL,(void*)leader_loop,&s);
    pthread_create(&pid_updata,NULL,(void*)self_updata,&s);
    pthread_create(&pid_check,NULL,(void*)self_check,&s);
    pthread_create(&pid_heart,NULL,(void*)leader_heart,&s);
    //pthread_t pid_debug;
    //pthread_create(&pid_debug,NULL,(void*)func_debug,&s);

    while(1){
        int time_rand = get_rand_time();
        if(debug1){printf("my rand time %ld\n",time_rand);}
        usleep(time_rand*1000);
        pthread_mutex_lock(&server_state);
        if(debug_lock)printf("election get lock\n");
        long s_active = s.lastactivetime;
        if((get_ms() - s_active) > overtime){
            if(debug1){printf("%ld %d overtime start election\n",get_ms()%simple_time,port);}
            election(&s); 
        }
        else{
            pthread_mutex_unlock(&server_state);
            if(debug_lock)printf("election back lock\n");
            usleep((s_active + overtime -get_ms())*1000+little_wait);
            continue;
        }
        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("election back lock\n");
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

    s->commitindex = -1;
    s->lastapplied = -1;

    int i = 0;
    for(i = 0;i < server_num;i++){
        s->nextindex[i] = 0;
        s->matchindex[i] = -1;
    }
    s->s_log[0].term = 0;
    
    return 0;
}

long get_ms(){
    long time_ms;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    time_ms = tv.tv_sec*1000+tv.tv_usec/1000;
    return time_ms;
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
                if(debug1){printf("thread_create %d error\n",ntohs(clnt_addr.sin_port));}
            }
            //pthread_join(pid[n],NULL);
            pid_n++;
            if(pid_n > sock_max){
                pid_n = 0;
            }
            
        }
    }
}

void process(void* argv){
    char request[128];
    S_T st = *(S_T*)argv;
    if(debug1){printf("\nstart process sock %d thread\n",ntohs(st.sock_addr.sin_port));}
    read(st.sock,request,sizeof(request));
    int h = 0;
    memcpy((void*)&h,request,sizeof(h));

    if(h == isvs){
        V_s vs;
        memcpy(&vs,request,sizeof(vs));
        if(debug1){printf("recv voterequest from %d term %d ",vs.id,vs.term);}
        if(debug1){printf("lastlogindex %d lastlogterm %d",vs.lastlogindex,vs.lastlogterm);}
        if(debug1){printf("at %d\n",ntohs(st.sock_addr.sin_port));}
        V_r vr;

        pthread_mutex_lock(&server_state);
        if(debug_lock)printf("p_vs get lock\n");
        process_vs(st.serv,&vs,&vr);
        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("p_vs back lock\n");

        char reply[sizeof(vr)+1];
        memcpy(reply,&vr,sizeof(vr));

        write(st.sock,reply,sizeof(reply));
        if(debug1){printf("send %d vs result %d\n",vs.id,vr.votegranted);}
        close(st.sock);
        if(debug1){printf("time %ld close\n\n",get_ms()%simple_time);}
        return;
    }
    else if(h == iscs){
        C_s cs;
        memcpy(&cs,request,sizeof(cs));
        C_r cr;
        
        pthread_mutex_lock(&server_state);
        if(debug_lock)printf("p_cs get lock\n");
        process_cs(st.serv,&cs,&cr);
        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("p_cs back lock\n");

        char reply[sizeof(cr)+1];
        memcpy(reply,&cr,sizeof(cr));
        write(st.sock,reply,sizeof(reply));
        if(debug1){printf("send %d cs result %d\n",cs.id,cr.success);}
        close(st.sock);
        if(debug1){printf("%ld close\n",get_ms()%simple_time);}
        return;
    }
    else if(h == isclient){
        pthread_mutex_lock(&server_state);
        if(debug_lock)printf("p_c get lock\n");
        if(debug2)printf("recv client\n");
        if(debug1){printf("client?\n");}
        Reply_C reply;
        if(st.serv->role == leader){
            if(debug2)printf("im leader process client request\n");
            Request_C c;
            memcpy(&c,request,sizeof(c));
            if(c.log.op == op_read){
                if(debug2)printf("client just read\n");
                int val = 0;
                if(find(st.serv,c.log.key,&val)){
                    if(debug2)printf("find ok result %d\n",val);
                    reply.h = client_ok;
                    reply.result = val;
                }
                else{
                    if(debug2)printf("find wrong\n");
                    reply.h = client_notfind;
                }
            }
            else if(c.log.op == op_write || c.log.op == op_delete || c.log.op == op_updata){
                if(debug2)printf("client change data save log\n");
                int re_ap_c = load_log_append(st.serv,&c.log,1);
                if(re_ap_c > 0){//??if log be delete in leader change??what should do??
                    pthread_mutex_unlock(&server_state);
                    if(debug_lock)printf("p_c back lock\n");
                    if(debug2)printf("wait log apply...\n");
                    while(1){
                        pthread_mutex_lock(&server_state);
                        if(debug_lock)printf("p_c check get lock\n");
                        if(st.serv->role != leader){
                            if(debug2)printf("lose leader role log apply wrong\n");
                            reply.h = server_role_wrong;
                            pthread_mutex_unlock(&server_state);
                            if(debug_lock)printf("p_c check back lock\n");
                            break;
                        }
                        if(st.serv->lastapplied > re_ap_c-1){
                            if(debug2)printf("log apply ok\n");
                            reply.h = client_ok;
                            pthread_mutex_unlock(&server_state);
                            if(debug_lock)printf("p_c check back lock\n");
                            break;
                        }
                        pthread_mutex_unlock(&server_state);
                        if(debug_lock)printf("p_c check back lock\n");
                    }

                }
                else{
                    reply.h = server_append_wrong;
                }
            }
            /*else if(c.log.op == op_delete){
                int re_ap_c = load_log_append(st.serv,&c.log,1);
                if(re_ap_c > 0){//
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
            else if(c.log.op == op_updata){

            }*/
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
            if(debug2){printf("not leader,redirect\n");}
            reply.h = client_redirect;
            reply.result = st.serv->leaderid;
            char buf[128];
            memcpy(buf,&reply,sizeof(reply));
            write(st.sock,buf,sizeof(buf));
            close(st.sock);
            return;
        }

        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("p_c check back lock\n");
    }
    else{
        if(debug2){printf("unknow request\n");}
    }
    return;
}

int process_vs(Server* serv,V_s* vs,V_r* vr){
    //if(debug1){printf("getlock\n");}
        vr->h = isvr;
    vr->term = serv->currentterm;
    vr->votegranted = vote_false;
    serv->lastactivetime = get_ms();
    if(vs->term < serv->currentterm){
        //if(debug1){printf("term <\n");}
        return;
    }    
    if(debug1){printf("my term %d votefor %d",serv->currentterm,serv->votedfor);}
    if(vs->term > serv->currentterm){
        serv->role = follow;
        serv->votedfor = none;
        serv->lastactivetime = get_ms();
    }
    if(serv->votedfor == none || serv->votedfor == vs->id){
        if(vs->lastlogindex >= serv->lastincludeindex){
            //if(debug1){printf("ok\n");}
            serv->votedfor = vs->id;
            serv->lastactivetime = get_ms();
            serv->currentterm = vs->term; 
            vr->term = serv->currentterm;
            vr->votegranted = vote_ok;
            if(debug1){printf("now %ld vote ok %d lastactive%ld votefor %d\n",get_ms()%simple_time,vr->votegranted,serv->lastactivetime,serv->votedfor);}
            return;
        }
        //if(debug1){printf("index <\n");}
    }
    if(debug1){printf("have vote\n");}
    return;
}

int process_cs(Server* serv,C_s* cs,C_r* cr){
    cr->h = iscr;
    cr->term = serv->currentterm;
    cr->success = copy_false;

    if(cs->term < serv->currentterm){
        cr->success = cr_error_term;
        if(debug1)printf("refuse because term\n");
        return;
    }

    if(cs->term == serv->currentterm && cs->id == serv->leaderid){
        serv->lastactivetime = get_ms();
    }
    
    if(cs->term > serv->currentterm && (serv->role != follow || serv->leaderid != cs->id)){
        serv->role = follow;
        serv->leaderid = cs->id;
        serv->currentterm = cs->term;
        serv->lastactivetime = get_ms();
    }
    if(cs->prevlogindex == -1){

    }
    else if(serv->s_log[cs->prevlogindex].term != cs->prevlogterm){
        cr->success = cr_error_dismatch;
        if(debug1){printf("log mismatch my log index %d term is %d\n",cs->prevlogindex,serv->s_log[cs->prevlogindex].term);}
        return;
    }
    
    if(cs->log_num == 0){
        cr->success = copy_ok;
        serv->lastactivetime = get_ms();
        if(cs->commit > serv->commitindex){
            serv->commitindex = min(cs->commit,cs->prevlogindex);
        }
        return;
    }
    else{

        if(debug1){printf("recv copyrequest from %d term %d ",cs->id,cs->term);}
        if(debug1){printf("prevlogindex %d prevlogterm %d ",cs->prevlogindex,cs->prevlogterm);}
        if(debug1){printf("log_num %d \n",cs->log_num);}
        if(debug1){printf("1 log op%d k%d v%d t%d\n",
        cs->send_log[0].op,cs->send_log[0].key,
        cs->send_log[0].val,cs->send_log[0].term);}
        //int k = serv->log_num;
        if(load_log_append(serv,cs->send_log,cs->log_num)){
            if(debug2)printf("log append ok copy %d \n",serv->log_num);
            cr->success = copy_ok;
            serv->lastactivetime = get_ms();
            if(debug2)printf("after copy s.l_n %d s.c %d cs.c %d\n",serv->log_num,serv->commitindex,cs->commit);
            serv->commitindex = min(cs->commit,serv->log_num-1);
        }
        else{
            if(debug1)printf("log copy error of num copy %d\n",serv->log_num);
        }
        
    }
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

int election(Server* s){
    if(1/*s->role == follow*/){
        if(debug1){printf("%ld %dstatt election\n",get_ms()%simple_time,port);}
        s->role = candidate;
        s->lastactivetime = get_ms();
        s->votedfor = s->id;
        s->currentterm += 1;
        V_s vs;
        V_r vr;
        result re;
        vote_init(&re);
        int i = 0;
        char s_buf[128];
        get_vs(s,&vs);

        for(i = 0;i < server_num;i++){
            if(node[i] == port){
                continue;
            }
            int v = send_vs(node[i],&vs,&vr);//in size 8
            if(v){
                if(debug1){printf("error connect %d when send vs\n",node[i]);}
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
                    if(debug1){printf("get %d vote\n",node[i]);}
                }
                else{
                    vote_lose(&re);
                    if(debug1){printf("lose %d vote\n",node[i]);}
                }
            }
        }
        if((re.ok+1) > majority){
            if(debug1){printf("my vote %d majority %d \n",re.ok,majority);}
            s->role = leader;
            s->lastactivetime = get_ms();
            if(debug2){printf("%d become leader!\n",s->id);}
            for(i = 0;i < server_num;i++){
                if(node[i] == port){
                    continue;
                }
                C_s cs;
                get_heart(s,&cs,node[i]);//error
                C_r cr;
                int c = send_cs(node[i],&cs,&cr);
                if(c){
                    if(debug1){printf("error connect %d when  send cs\n",node[i]);}
                }   
            }
            Log l;
            l.op = op_write;
            l.key = 1;
            l.val = 3;
            l.term = s->currentterm;
            load_log_append(s,&l,1);
            return;
        }
        else{
            if(debug1){printf("%d lose election\n",port);}
        }
    }
}

int vote_init(result* re){
    pthread_mutex_lock(&vote_count);
    re->finish = 1;
    re->ok = 1;
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
    
    vs->lastlogindex = s->lastincludeindex;
    vs->lastlogterm = s->lastincludeterm;
    vs->term = s->currentterm;
    if(debug1){printf("get %d now vs lastlogindex: %d lastlogterm: %d term: %d\n"
    ,vs->id,vs->lastlogindex,vs->lastlogterm,vs->term);}
}

int get_cs(Server* s,C_s* cs,int goal_port){
    cs->h = iscs;
    cs->id = s->id;
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    cs->prevlogindex = 0;
    cs->prevlogterm = 0;

   //if(debug1)printf("func get cs\t%d matcchindex %d s.log_num %d\n",
   //goal_port,s->matchindex[goal_port-port_start],s->log_num);
   if(s->matchindex[goal_port - port_start] == s->log_num-1){
       get_heart(s,cs,goal_port);
       return;
   }
    else{
        if(s->matchindex[goal_port-port_start] < 0){
            cs->prevlogindex = 0;
            cs->prevlogterm = 0;
        }
        else{
            cs->prevlogindex = s->matchindex[goal_port-port_start];
            cs->prevlogterm = s->s_log[cs->prevlogindex].term;
        }
       if(debug1){printf("func get cs\tcopying load log...\n");}
       if(s->log_num - 1 - s->matchindex[goal_port - port_start] > 5){
            if(debug1)printf("func get cs\tlog - %d matchindex %d log_n %d  >5\n",
            goal_port,s->matchindex[goal_port - port_start],s->log_num);
            cs->log_num = 5;
            int i = 0;
            for(i = 0;i < 5;i++){
                memcpy(&cs->send_log[i],&s->s_log[s->nextindex[goal_port - port_start]+ i - 1],sizeof(Log));
            }
       }
       else{
            if(debug1)printf("func get cs\tlog - %d matchindex %d log_n %d  <5\n",
            goal_port,s->matchindex[goal_port - port_start],s->log_num);
            cs->log_num = s->log_num - 1 - s->matchindex[goal_port - port_start];
            int n = s->log_num -1 - s->matchindex[goal_port - port_start];
            int i = 0;
            for(i = 0;i < n;i++){
                memcpy(&cs->send_log[i],&s->s_log[s->nextindex[goal_port - port_start]+ i],sizeof(Log));
            }
        }
   }
    if(debug2 && cs->log_num != 0){printf("func get cs\tget %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d log_num %d\n"
    ,cs->id,cs->prevlogindex,cs->prevlogterm,cs->term,cs->commit,cs->log_num);}
}

int get_heart(Server* s,C_s* cs,int goal_port){
    
    cs->h = iscs;
    cs->id = s->id;
    cs->log_num = 0;
    
    if(debug1){printf("func get hreat\t%d nextindex is %d \n",goal_port,s->nextindex[goal_port - port_start]);}
    if(s->matchindex[goal_port-port_start] < 0){
        cs->prevlogindex = -1;
        cs->prevlogterm = 0;
    }
    else{
        cs->prevlogindex = s->matchindex[goal_port-port_start];
        cs->prevlogterm = s->s_log[cs->prevlogindex].term;
    }
    cs->term = s->currentterm;
    cs->commit = s->commitindex;
    if(debug1){printf("func get hreat\tget %d now cs prevlogindex: %d prevlogterm: %d term: %d commit: %d log_num %d\n"
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
    
    char request[128];
    char reply[16];
    memcpy(request,vs,sizeof(*vs));
    write(sock,request,sizeof(request));
    
    read(sock,reply,sizeof(reply));
    
    memcpy(vr,reply,sizeof(*vr));
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

    char request[128];
    char reply[16];
    memcpy(request,cs,sizeof(*cs));
    write(sock,request,sizeof(request));
    read(sock,reply,sizeof(reply));
    memcpy(cr,reply,sizeof(*cr));
    return 1;
}

int load_log_append(Server* s,Log* log,int num){
    int i = 0;
    for(i = 0;i < num;i++){
        if(s->log_num < load_log_maxsize){
            if(debug2)printf("append log op %d key %d value %d term %d at time%d\n",
            log[i].op,log[i].key,log[i].val,log[i].term,get_ms()%1000000);
            s->s_log[s->log_num] = log[i];
            //s->s_log[s->log_num].term = s->currentterm;
            s->lastincludeterm = s->s_log[s->log_num].term;
            s->lastincludeindex = s->log_num;
            s->log_num++;
        }
        else{
            return 0;
        }
    }
    return 1;
}

/*int leader_loop(void* argv){
    while(1){
        pthread_mutex_lock(&server_state);
        Server* s = (Server*)argv;
        if(s->role == leader){
            if(s->lastactivetime + leader_copy_time < get_ms()){
                printf("heart.\n")
                s->lastactivetime = get_ms();
                C_s cs;
                C_r cr;
                int i = 0;
                for(i = 0;i < server_num;i++){
                    if(node[i] == port){
                        continue;
                    }
                    get_cs(s,&cs,node[i]);
                    int re = send_cs(node[i],&cs,&cr);
                    if(re == error_connect){
                        if(debug1){printf("func leaderloop\terror connect when copy log to %d\n",node[i]);}
                        continue;
                    }
                    if(debug1)printf("func leaderloop\trecv %d cr is %d\n",node[i],cr.success);
                    if(cr.success == cr_error_term){
                        if(s->currentterm < cr.term){
                            if(debug1)printf("func leaderloop\tcr tell leader change\n");
                            s->role = follow;
                            s->currentterm = cr.term;
                            s->leaderid = node[i];
                            break;
                        }
                    }
                    else if(cr.success == cr_error_dismatch){
                        if(debug1)printf("func leaderloop\tcr tell dismatch\n");
                        if(s->nextindex[i] > 1 && s->matchindex[i] > 1){
                            s->matchindex[i] --;
                            s->nextindex[i] = s->matchindex[i] + 1;
                        }
                    }
                    else if(cr.success == copy_ok){
                        if(debug1)printf("func leaderloop\t%d copy ok now matchindex %d\n",
                        node[i],s->matchindex[i]);
                        if(debug1)printf("func leaderloop\t cs.l_n%d\n",cs.log_num);
                        if(debug2 && cs.log_num > 0){
                            printf("func_leaderloop\t copy ok befor %d match %d\n",node[i],s->matchindex[i]);
                        }
                        s->matchindex[i] += cs.log_num;
                        if(debug2 && cs.log_num > 0){
                            printf("func_leaderloop\t copy ok after %d match %d\n",node[i],s->matchindex[i]);
                        }
                        s->nextindex[i] = s->matchindex[i] + 1;
                        if(debug1)printf("func leaderloop\t%d copy ok now matchindex %d\n",
                        node[i],s->matchindex[i]);
                    }
                }
            }
        }
        pthread_mutex_unlock(&server_state);
        //if(debug2)usleep(debug2_leaderloop_uodataloop_);
    }
    return;
}*/

int self_updata(void* argv){
    while(1){
        pthread_mutex_lock(&server_state);
        if(debug_lock)printf("self_updata get lock\n");
        Server* s = (Server*)argv;
        if(s->commitindex > s->lastapplied){
            if(log_apply(s)){
                //s->lastapplied++;
                if(debug2)printf("log apply ok at index %d\n",s->lastapplied);
            }
            else{
                if(debug2)printf("apply error at index %d\n",s->lastapplied);
            }
        }
        if(s->role == leader){
            //if(debug2)printf("start_findk.../t");
            s->matchindex[s->id - port_start] = s->log_num - 1;
            int k = newarray_for_find(s->matchindex,server_num);
            //if(debug2)printf("\tk %d",k);
            if(k > s->commitindex){
                if(debug2)printf("k %d\n",k);
                int i = 0;
                for(i = 0;i < server_num;i++){
                    if(debug2)printf("%d matchindex %d \t",node[i],s->matchindex[i]);
                }
                printf("\nk %d\n",k);
                s->commitindex = k;
                C_s cs;
                C_r cr;
                for(i = 0;i < server_num;i++){
                    if(node[i] == s->id)continue;
                    get_heart(s,&cs,node[i]);
                    send_cs(node[i],&cs,&cr);
                }
            }
            //if(debug2)printf("end_findk\n");
        }
        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("self_updata back lock\n");
        usleep(updata_time);
        //if(debug2)usleep(debug2_leaderloop_uodataloop_);
    }
    return;
}

int leader_heart(void* argv){
    while(1){
        Server* s = (Server*)argv;
        if(s->role == leader){
            if(debug2)printf("heart..\t");
            pthread_mutex_lock(&server_state);
            if(debug_lock)printf("leader_heart get lock\n");
            if(debug_lock)printf("heart get lock\n");
            s->lastactivetime = get_ms();
            C_s cs;
            C_r cr;
            int i = 0;
            for(i = 0;i < server_num;i++){
                if(node[i] == s->id)continue;
                get_heart(s,&cs,node[i]);
                send_cs(node[i],&cs,&cr);
            }
            pthread_mutex_unlock(&server_state);
            if(debug_lock)printf("leader_heart back lock\n");
        }
        usleep(heart_time);
    }
}

int self_check(void* argv){
    while(1){
        Server* s = (Server*)argv;
        pthread_mutex_lock(&server_state);
        if(debug2)printf("self_check..\t");
        if(debug_lock)printf("self_check get lock\n");
        if(s->role == leader){
            int i = 0;
            for(i = 0;i < server_num;i++){
                if(node[i] == s->id)continue;
                if(s->matchindex[i] < s->log_num - 1){
                    if(debug2)printf("time %d %d need append log\n",
                    get_ms()%simple_time,node[i]);
                    C_s cs;
                    C_r cr;
                    get_cs(s,&cs,node[i]);
                    send_cs(node[i],&cs,&cr);
                    process_cr(s,&cr,&cs,node[i]);
                }
            }
        }
        pthread_mutex_unlock(&server_state);
        if(debug_lock)printf("self_check get lock\n");
        usleep(check_time);
    }
}

int process_cr(Server* s,C_r* cr,C_s* cs,int from_port){
    if(cr->success == cr_error_term){
        s->role = follow;
        s->leaderid = from_port;
        s->currentterm = cr->term;
        s->votedfor = from_port;
        return;
    }
    if(cr->success == cr_error_dismatch){
        s->matchindex[from_port - port_start]--;
        s->nextindex[from_port - port_start]--;
        return;
    }
    if(cr->success == copy_ok){
        s->matchindex[from_port - port_start] += cs->log_num;
        s->nextindex[from_port - port_start] += cs->log_num;
        return;
    }
    return;
}

int log_apply(Server* s){
    if(debug2)printf("start apply log index %d at time%d\n",
    s->lastapplied+1,get_ms()%1000000);
    if(s->s_log[s->lastapplied + 1].op == op_write){
        int re = hash_insert(&hash_h,s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val);
        if(debug2)printf("apply log write key %d val %d term %d result %d\n",
        s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val,s->s_log[s->lastapplied + 1].term,re);
        hash_print(&hash_h);
        if(re){
            s->lastapplied++;
            return 1;
        }
        return 0;
    }
    else if(s->s_log[s->lastapplied + 1].op == op_read){
        //hash_print(&h);
        return 1;
    }
    else if(s->s_log[s->lastapplied + 1].op == op_delete){
        int re = hash_delete(&hash_h,s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val);
        if(debug2)printf("apply log delete key %d val %d term %d result %d\n",
        s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val,s->s_log[s->lastapplied + 1].term,re);
        hash_print(&hash_h);
        if(re){
            s->lastapplied++;
            return 1;
        }
        return 0;
    }
    else if(s->s_log[s->lastapplied + 1].op == op_updata){
        int re = hash_updata(&hash_h,s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val);
        if(debug2)printf("apply log updata key %d val %d term %d result %d\n",
        s->s_log[s->lastapplied + 1].key,s->s_log[s->lastapplied + 1].val,s->s_log[s->lastapplied + 1].term,re);
        hash_print(&hash_h);
        if(re){
            s->lastapplied++;
            return 1;
        }
        return 0;
    }
    return 0;
}

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

int newarray_for_find(int *array,int size){
    int* a = (int*)malloc(sizeof(int)*size);
    memcpy(a,array,sizeof(array));
    int re = find_k(a,0,size-1,majority-1);
    free(a);
    return re;
}

int find_k(int* array,int start,int end,int k){
    if(start >= end){return array[start];}
    int flag = array[start];
    int i = start;
    int j = end;
    int temp = 0;
    while(i < j){
        while(i < j && array[j] >= flag){
            j--;
        }
        if(i < j){
            array[i] = array[j];
        }
        while(i < j && array[i] <= flag){
            i++;
        }
        if(i < j){
            array[j] = array[i];
        }

    }
    array[i] = flag;
    if(i - start +1 == k){
        return array[i];
    }
    else if(i - start + 1 > k){
        return find_k(array,start,i-1,k);
    }
    else{
        return find_k(array,i+1,end,k - i + start -1);
    }
}

int func_debug(void* argv){
    while(1){
        pthread_mutex_lock(&server_state);
        Server* s  = (Server*)argv;
        if(debug2)printf("server state: s.log_n %d s.commit %d\n",
        s->log_num,s->commitindex);
        pthread_mutex_unlock(&server_state);
        sleep(debug_time);
    }
}
// 3/8 7 1 6 9 5 2 4 3
//0  8
// 3  j 6         2 7 1 6 9 5 2 4 3 
// 3  i 1         271695743
//3  j 2         211695743
//3 i 2         213695743