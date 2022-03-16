#include"raft.h"


struct mytype{
    int h;
    char content[10];
};


int main(){


    Request_C c;
    Reply_C r;
    c.h = isclient;
    c.log.op = op_write;
    c.log.key = 3;
    c.log.val = 5;

    //创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    //向服务器（特定的IP和端口）发起请求
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
    serv_addr.sin_port = htons(12341);  //端口
    int con = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("1 %d\n",con);
    char buffer[128];
    //V_s m1;  
    //m1.h = 3;
    //m1.term = 1;
    //memcpy(m1.content,buffer,sizeof(m1.content));
    //printf("send %d %s\n",m1.h,m1.content);
    memcpy(buffer,&c,sizeof(c));
    write(sock, buffer, sizeof(buffer));
    //printf("after write \n");
    read(sock,buffer,sizeof(buffer));
    memcpy(&r,buffer,sizeof(r));
    printf("Message form server:%d  %d\n",r.h,r.result);
    //if(send(sock,buffer,sizeof(buffer),MSG_DONTWAIT) == -1 && errno == EPIPE){
       //printf("serv close\n");
    //}
    printf("%ld\n",get_ms());
    //关闭套接字
    close(sock);

    return 0;
}


long get_ms(){
    long time_ms;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    time_ms = tv.tv_sec*1000+tv.tv_usec/1000;
    return time_ms;
}
