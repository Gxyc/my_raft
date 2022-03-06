#ifndef RAFT
#define RAFT

#include<stdio.h>
#include<sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include<pthread.h>

#include<sys/wait.h>

#include<errno.h>
//#include<stdio.h>

#define logsize 16
#define load_log_maxsize 10
#define log_send_max 5

#define leader 1
#define follow 2
#define candidate 3

#define server_num 3
#define majority  (server_num / 2 + 1)
#define overtime 10000
#define election_time 1000
#define leader_copy_time 500


#define isvs 1
#define isvr 2
#define iscs 3
#define iscr 4
#define isclient 5

#define copy_false  -1
#define copy_ok  1
#define vote_false -1
#define vote_ok 1

#define little_wait 5 
#define port_start 12341

#define empty 0
#define none -1

#define false 0
#define true 1

#define simple_time 100000


int node[server_num] = {12341,12342,12343};

typedef struct log{
    char _log[logsize];
    int term;
}Log;

typedef struct server{
    //pthread_mutex_t mutex;
    int id;
    int role;

    int currentterm;
    int votedfor;

    Log s_log[load_log_maxsize];

    int commitindex;
    int lastapplied;

    int nextindex[server_num];
    int matchindex[server_num];

    int lastincludeindex;
    int lastincludeterm;
    int leaderid;

    long lastactivetime;

}Server;

typedef struct vote_s{
    int h;
    int term;
    int id;
    int lastlogindex;
    int lastlogterm;
}V_s;

typedef struct vote_r{
    int h;
    int term;
    int votegranted;
}V_r;

typedef struct copy_s{
    int h;
    int term;
    int id;
    int prevlogindex;
    int prevlogterm;
    int commit;
    char send_log[log_send_max][logsize];
}C_s;

typedef struct copy_r{
    int h;
    int term;
    int success;
}C_r;

typedef struct sock_thread{
    Server* serv;
    int sock;
    struct sockaddr_in sock_addr;
}S_T;

typedef struct vote_result{
    int finish;
    int ok;
}result;

int server_init(Server* s);
long get_ms();
//int isovertime(Server* s);
int send_for_reply(int recv_port,char* request,char* reply);
int send_heart();
int listen_loop();
void process(void* argv);
int process_vs(Server* serv,V_s* vs,V_r* vr);
int process_cs(Server* serv,C_s* cs,C_r* cr);
int election(Server* s);
int vote_init(result* re);
int vote_get(result* re);
int vote_lose(result* re);
int get_vs(Server* s,V_s* vs);
int get_cs(Server* s,C_s* cs,int goal_port);
int send_vs(int recv_port,V_s* vs,V_r* vr);
int send_cs(int recv_port,C_s* cs,C_r* cr);
int load_log_append(Server* s);
int log_copy(Server* s);
int leader_loop(void* argv);
int self_updata(void* argv);
long get_rand_time();



#endif