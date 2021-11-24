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
#define MR_SRC_PORT       19794
#define MR_DST_PORT       19274

#define MAX_INTERFACE     1

int interface_num = MAX_INTERFACE;

dev_info_t dev_info[MAX_INTERFACE];

uint8_t send_poll1[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t send_poll2[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x0e, 0x00, 0x10, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00};

char sendbuf[1024];
char recvbuf[1024];

extern int mr_socket;
extern socklen_t slen;
int send_length;

int sock;    // 本机sock
struct sockaddr_in host   = {0};
struct sockaddr_in remote = {0};
extern struct sockaddr_in mr_dev;

sqlite3 *db;
char *errmsg;
char sql[128] = {0};

extern uint8_t (*mr_framebuffer)[1472];
extern uint8_t (*led_location)[18];
extern uint16_t framebuffer_len;

uint8_t rgb_data[16200 * 4] = {0};

// 获取节点的ArtPollReply
void *thread_get_artpollreply(void *arg)
{
    // char ip_addr[15] = {0};
    // char mac_addr[20] = {0};
    
    // struct sockaddr_in node;
    // bzero(&node, sizeof(node));

    // send_length = sendto(sock, send_poll, sizeof(send_poll), 0, (struct sockaddr *)&remote, len);
    // if(send_length < 0){
    //     perror("sendto");
    // }else{
    //     printf("send_length = %d\n", send_length);
    // }
    
    // while(1) 
    // {
    //     ssize_t s = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0, (struct sockaddr*)&node, &len);
    //     if(s > 0){
    //         recvbuf[s] = 0;
    //         printf("host %s: port %d: say# %s \n", inet_ntoa(node.sin_addr), ntohs(node.sin_port), recvbuf);

    //         sendto(sock, recvbuf, s, 0, (struct sockaddr *)&host, len);   // 发送给主机，可通过抓包工具查看
    //     }

    //     struct replyPollPacket *reply = (struct replyPollPacket *)recvbuf;
    //     snprintf(ip_addr, sizeof(ip_addr), "%d.%d.%d.%d", reply->IPAddr[0], reply->IPAddr[1], reply->IPAddr[2], reply->IPAddr[3]);
    //     snprintf(mac_addr, sizeof(mac_addr), "%02X:%02X:%02X:%02X:%02X:%02X", reply->Mac[0], reply->Mac[1], reply->Mac[2], reply->Mac[3], reply->Mac[4], reply->Mac[5]);

    //     // 存入数据库                  
    //     sprintf(sql, "insert into stu values('%s', '%s', %d, '%s');", reply->ShortName, ip_addr, reply->Port, mac_addr);

    //     printf("%s\n", sql);

    //     if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
    //         printf("%s\n", errmsg);
    //     }else{
    //         printf("Insert success.\n");
    //     }

    //     //sleep(1);
    // }
}


// 发送rgb数据到明瑞分控
void *thread_send_rgbdata(void *arg)
{
    uint16_t len = 0;
    uint16_t axis_x = 0;
    uint16_t axis_y = 0;
    int32_t rgb_index = -1;
    uint32_t buf_index = 0;
    uint8_t channel_num;
    uint32_t (*rgb_buffer)[180] = (uint32_t (*)[180])rgb_data;
    uint32_t rgbw = 0;
    uint32_t loop_cnt = 0;

    ssize_t size;
    uint32_t cnt = 0;
    uint16_t real_frame_len = 0;

    int fd_src = 0;
    int fd_dst = 0;
    char rgb_buf[16200 * 3] = {0};
    char lighting = 0xff;

    uint32_t index = 0;

    //fd_src = open("/doc/thoseyears.rgb", O_RDONLY);
    fd_src = open("/doc/rgbfile180x90.rgb", O_RDONLY);
    if(fd_src < 0){
        printf("open error!\n");
        exit(1);
    }

    while((size = read(fd_src, rgb_buf, sizeof(rgb_buf))) > 0)
    {
        printf("size: %ld\n", size);

        // for(int i=0; i<size; i++)
        // {
        //     if((i%8 == 0) && (i != 0)) 
        //         printf(" ");
        //     if(i%16 == 0)
        //         printf("\n");
        //     printf("%02x ", (uint8_t)rgb_buf[i]);
        // }

        cnt++;

        // printf("\n");

        for(int i=0; i<16200*3; i++)
        {
            if(i%3 == 0){
                rgb_data[index++]   = rgb_buf[i];
                rgb_data[index++] = rgb_buf[i+1];
                rgb_data[index++] = rgb_buf[i+2];
                rgb_data[index++] = 0x00;
            }
        }

        // for(int i=0; i<16200*2; i++)
        // {
        //     if(i%4 == 0){
        //         rgb_data[i]   = 0x12;
        //         rgb_data[i+1] = 0x34;
        //         rgb_data[i+2] = 0x56;
        //         rgb_data[i+3] = 0x00;
        //     }
        // }

        // for(int i=16200*2; i<16200*4; i++)
        // {
        //     if(i%4 == 0){
        //         rgb_data[i]   = 0x78;
        //         rgb_data[i+1] = 0x9a;
        //         rgb_data[i+2] = 0xbc;
        //         rgb_data[i+3] = 0x00;
        //     }
        // }

        // 准备一帧数据
        for(int framebuffer_index=0; framebuffer_index<framebuffer_len; framebuffer_index++)
        {
            if(mr_framebuffer[framebuffer_index][0] == 0x4d && mr_framebuffer[framebuffer_index][1] == 0x52 && mr_framebuffer[framebuffer_index][2] == 0x4b && mr_framebuffer[framebuffer_index][3] == 0x4a){
                
                channel_num = mr_framebuffer[framebuffer_index][6];   // 当前帧中包含多少通道的数据

                real_frame_len++;

                uint8_t *p_framebuffer = (uint8_t *)mr_framebuffer[framebuffer_index];
                uint8_t channel_id = 0;
            
                p_framebuffer+=8;

                while(channel_num--)
                {
                    p_framebuffer+=2;  
                    channel_id = *p_framebuffer;
                    p_framebuffer+=2;    
                    len = *(p_framebuffer+1) << 8 | *p_framebuffer;   // 该通道包含的字节数
                    p_framebuffer+=2;

                    for(int i=0; i<len; i++)
                    {   
                        rgb_index += (loop_cnt%3 == 0 ? 1: 0);  

                        if(loop_cnt%3 == 0){
                            axis_x = led_location[rgb_index][0] << 8 | led_location[rgb_index][1];
                            axis_y = led_location[rgb_index][2] << 8 | led_location[rgb_index][3];

                            rgbw = htonl(rgb_buffer[axis_y][axis_x]);

                            // printf("(%d, ", axis_y);
                            // printf(" %d)", axis_x);
                            // printf("\n");

                            p_framebuffer[i] = rgbw >> 16;    // G
                        }else if(loop_cnt%3 == 1){
                            p_framebuffer[i] = rgbw >> 24;    // R
                        }else if(loop_cnt%3 == 2){
                            p_framebuffer[i] = rgbw >> 8;     // B
                        }

                        loop_cnt++;
                    }

                    p_framebuffer+=len;
                }
            }else{
                break;
            }

            // for(int i=0; i<1472; i++)
            // {
            //     if((i%8 == 0) && (i != 0)) 
            //         printf(" ");
            //     if(i%16 == 0)
            //         printf("\n");
            //     printf("%02x ", mr_framebuffer[framebuffer_index][i]);
            // }

            // printf("\n");
        }

        // 发送一帧数据
        for(int i=0; i<real_frame_len; i++)
        {
            // for(int j=0; j<14; j++)
            // {
            //     printf("%02x ", (uint8_t)mr_framebuffer[i][j]);
            // }
            // printf("\n");
            
            send_length = sendto(mr_socket, (char *)mr_framebuffer[i], 1472, 0, (struct sockaddr *)&mr_dev, slen);
            if(send_length < 0){
                perror("sendto");
            }else{
                printf("send_length = %d\n", send_length);
            }
            usleep(100);
        }

        sendto(mr_socket, send_poll1, sizeof(send_poll1), 0, (struct sockaddr *) &mr_dev, slen);
        usleep(1);
        sendto(mr_socket, send_poll2, sizeof(send_poll2), 0, (struct sockaddr *) &mr_dev, slen);

        usleep(5000);

        real_frame_len = 0;
        rgb_index = 0;
        loop_cnt = 0;
        index = 0;
    }

    printf("cnt = %d\n", cnt);

    while(1);
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

    /* 建立IPv4的UDP套接字sock */  
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
    // local.sin_addr.s_addr = inet_addr("199.199.199.199");
    // local.sin_port = htons(MR_SRC_PORT);

    // slen = sizeof(local); 

    // if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
    //     perror("bind");
    //     return -1;
    // }    

    // mr_dev.sin_family = AF_INET;
    // inet_aton("255.255.255.255", &mr_dev.sin_addr);
    // mr_dev.sin_port = htons(MR_DST_PORT);

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

    parse_configuration_file();

    pthread_t tid1;
    pthread_create(&tid1, NULL, (void *)thread_send_rgbdata, NULL);

    /* 发送ArtPoll */
    while(1) 
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
