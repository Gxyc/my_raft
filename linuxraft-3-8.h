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
#define leader_copy_time 5000


#define sock_max 1024


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

#define error_connect -1

#define cr_error_term -2
#define cr_error_dismatch -3

#define client_read 1
#define client_write 2
#define client_delete 3
#define client_updata 4

//#define client_recv 11
#define client_redirect 12
//#define client_tryagain 2

#define client_ok 21
#define client_notfind 22
#define client_unrecognized 23
#define client_wrong 24
#define server_role_wrong 31
#define server_append_wrong 32

int node[server_num] = {12341,12342,12343};

typedef struct log{
    int op;
    int key;
    int val;
    int term;
}Log;

typedef struct server{
    //pthread_mutex_t mutex;
    int id;
    int role;

    int currentterm;
    int votedfor;

    int log_num;
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
    int log_num;
    Log send_log[log_send_max];
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

typedef struct client_request{
    int h;
    Log log;
}Request_C;

typedef struct client_reply{
    int h;
    int result;
}Reply_C;

int server_init(Server* s);
long get_ms();
//int isovertime(Server* s);
//int send_for_reply(int recv_port,char* request,char* reply);
//int send_heart();
int listen_loop();
void process(void* argv);
int process_vs(Server* serv,V_s* vs,V_r* vr);
int process_cs(Server* serv,C_s* cs,C_r* cr);
int copy_log(C_s* cs, Server* s);
int election(Server* s);
int vote_init(result* re);
int vote_get(result* re);
int vote_lose(result* re);
int get_vs(Server* s,V_s* vs);
int get_cs(Server* s,C_s* cs,int goal_port);
int get_hreat(Server* s,C_s* cs,int goal_port);
int send_vs(int recv_port,V_s* vs,V_r* vr);
int send_cs(int recv_port,C_s* cs,C_r* cr);
int load_log_append(Server* s,Log* log,int num);
//int log_copy(Server* s);
int leader_loop(void* argv);
int self_updata(void* argv);
int log_apply(Server* s,int index);
long get_us();
long get_rand_time();
int find(Server* s,int key,int* val);

int min(int a,int b);
int max(int a,int b);



#endif