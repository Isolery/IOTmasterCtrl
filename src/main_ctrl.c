#include <stdio.h> 
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>  
#include <net/if_arp.h>  
#include <netinet/in.h>  
#include <pthread.h>

#include "sqlite3.h"

#include "dev_info.h"
#include "my_sqlite3.h"
#include "node_info.h"

#include "mr_protocol.h"

#include "zlog.h"

#define ARTNET_PORT       6454

#define MAX_INTERFACE     1

int interface_num = MAX_INTERFACE;

dev_info_t dev_info[MAX_INTERFACE];

uint8_t send_poll[] = {0x41, 0x72, 0x74, 0x2d, 0x4e, 0x65, 0x74, 0x00, 0x00, 0x20, 0x00, 0x0e, 0x00, 0x00};

char sendbuf[1024];
char recvbuf[1024];

socklen_t len;
int send_length;

int sock;    // 本机sock
struct sockaddr_in host;
struct sockaddr_in remote;

 sqlite3 *db;
 char *errmsg;
 char sql[128] = {0};

// 获取节点的ArtPollReply
void *thread_get_artpollreply(void *arg)
{
    char ip_addr[15] = {0};
    char mac_addr[20] = {0};
    
    struct sockaddr_in node;
    bzero(&node, sizeof(node));

    send_length = sendto(sock, send_poll, sizeof(send_poll), 0, (struct sockaddr *)&remote, len);
    if(send_length < 0){
        perror("sendto");
    }else{
        printf("send_length = %d\n", send_length);
    }
    
    while(1) 
    {
        ssize_t s = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0, (struct sockaddr*)&node, &len);
        if(s > 0){
            recvbuf[s] = 0;
            printf("host %s: port %d: say# %s \n", inet_ntoa(node.sin_addr), ntohs(node.sin_port), recvbuf);

            sendto(sock, recvbuf, s, 0, (struct sockaddr *)&host, len);   // 发送给主机，可通过抓包工具查看
        }

        struct replyPollPacket *reply = (struct replyPollPacket *)recvbuf;
        snprintf(ip_addr, sizeof(ip_addr), "%d.%d.%d.%d", reply->IPAddr[0], reply->IPAddr[1], reply->IPAddr[2], reply->IPAddr[3]);
        snprintf(mac_addr, sizeof(mac_addr), "%02X:%02X:%02X:%02X:%02X:%02X", reply->Mac[0], reply->Mac[1], reply->Mac[2], reply->Mac[3], reply->Mac[4], reply->Mac[5]);

        // 存入数据库                  
        sprintf(sql, "insert into stu values('%s', '%s', %d, '%s');", reply->ShortName, ip_addr, reply->Port, mac_addr);

        printf("%s\n", sql);

        if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
            printf("%s\n", errmsg);
        }else{
            printf("Insert success.\n");
        }

        //sleep(1);
    }
}

int main(void)
{
    // int cmd;

    // int rc_zlog;

	// rc_zlog = dzlog_init("zlog.conf", "myrule_class");
	// if (rc_zlog)
	// {
	// 	printf("init failed\n");
	// 	zlog_fini();
	// }

    // dzlog_info("helo, zlog info");
    // dzlog_error("helo, zlog error");
    // dzlog_warn("helo, zlog warn");
    // dzlog_debug("helo, zlog debug");

    // get_device_info((dev_info_t *)&dev_info, interface_num);    

    // for(int i=0; i<interface_num; i++)
    // {
    //     printf("IP Address:%s\n", dev_info[i].ip_addr);
    //     printf("MAC Address:%02x:%02x:%02x:%02x:%02x:%02x\n\n", dev_info[i].mac_addr[0], dev_info[i].mac_addr[1], dev_info[i].mac_addr[2], dev_info[i].mac_addr[3], dev_info[i].mac_addr[4], dev_info[i].mac_addr[5]);  
    // }     

    // // /* 建立IPv4的UDP套接字sock */  
    // sock = socket(AF_INET, SOCK_DGRAM, 0);

    // if(sock < 0){
    //     perror("socket");
    //     return -1;
    // }      

    // int iOptval = 1;
    // if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int)) < 0){
    //     printf("setsockopt failed!");
    // }     

    // /* bind */ 
    // struct sockaddr_in local;

    // bzero(&local, sizeof(local));
    // local.sin_family = AF_INET;
    // local.sin_addr.s_addr = inet_addr("192.168.0.183");
    // local.sin_port = htons(ARTNET_PORT);

    // len = sizeof(local); 

    // if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
    //     perror("bind");
    //     return -1;
    // }    

    // // 广播地址
    // bzero(&remote, sizeof(remote));
    // remote.sin_family = AF_INET;
    // remote.sin_addr.s_addr = inet_addr("255.255.255.255");
    // remote.sin_port = htons(ARTNET_PORT);

    // // 主机地址
    // bzero(&host, sizeof(host));
    // host.sin_family = AF_INET;
    // host.sin_addr.s_addr = inet_addr("192.168.0.224");
    // host.sin_port = htons(ARTNET_PORT);

    // if(sqlite3_open(DATABASE, &db) != SQLITE_OK){
    //     printf("%s\n", sqlite3_errmsg(db));
    //     return -1;
    // }else{
    //     printf("Open DATABASE success.\n");
    // }

    // // 创建一张数据库表格
    // if(sqlite3_exec(db, "create table stu(NODE_NAME char UNIQUE, IP_ADDR char, OUT_PORT Integer, MAC_ADDR char);", NULL, NULL, &errmsg) != SQLITE_OK){
    //     printf("%s\n", errmsg);
    // }else{
    //     printf("create table or open success.\n");
    // }

    // pthread_t tid0;
    // pthread_create(&tid0, NULL, (void *)thread_get_artpollreply, NULL);

    subctrl_configuration();

    /* 发送ArtPoll */
    while(0) 
    {
        // send_length = sendto(sock, send_poll, sizeof(send_poll), 0, (struct sockaddr *)&remote, len);
        // if( send_length < 0){
        //     perror("sendto");
        // }else{
        //     printf("send_length = %d\n", send_length);
        // }

        sleep(3);
    }

    return 0;
}
