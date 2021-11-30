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
#include <sys/shm.h> 
#include <signal.h>
#include <sys/msg.h>
#include <sys/ipc.h>

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

int pid;

#define SHARED_BUFFER_SIZE 16200*4

typedef struct
{
    int pid;                             // 0表示可写 ，非0表示不可写
    uint8_t rgb_data[SHARED_BUFFER_SIZE];    
} shared_user_st;

extern uint8_t (*mr_framebuffer)[1472];
extern uint8_t (*led_location)[18];
extern uint16_t framebuffer_len;

pthread_cond_t condv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; 


shared_user_st *shared_user = NULL;

struct amsgbuf{
	long int type;
	char text[128];
};

struct amsgbuf arecvmsg;
struct amsgbuf asendmsg;

key_t shm_key = 0;
key_t msg_key = 0;
int msg_id = 0;
int shmid = 0;

static void atexit_handler(void)
{
	shmdt(shared_user);
	shmctl(shmid, IPC_RMID, NULL);
	system("ipcs -m"); 

    if(msgctl(msg_id, IPC_RMID, NULL) < 0){
		printf("del msg error \n");
	}

	printf("atexit_handler\n");
}

void handler(int signum)
{
	exit(0);
}

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
    uint8_t channel_num;

    uint32_t (*rgb_buffer)[180] = (uint32_t (*)[180])shared_user->rgb_data;
    uint32_t rgbw = 0;
    uint32_t loop_cnt = 0;

    uint32_t cnt = 0;
    uint16_t real_frame_len = 0;

    uint32_t index = 0;

    while(1)
    {
        do
		{
			memset(&arecvmsg, 0, sizeof(arecvmsg));
			arecvmsg.type = 100; 

			if(msgrcv(msg_id, (void *)&arecvmsg, 128, arecvmsg.type, 0) < 0){
				printf("receive msg error\n");
				break;
			}
		}while(!(strncmp(arecvmsg.text, "start", 5) == 0));

        // send rgb data...
        // printf("rgb_buffer = %d\n", (int)rgb_buffer[0][0]);

        // memset(&asendmsg, 0, sizeof(asendmsg));
		// asendmsg.type = 200;
		// memcpy(asendmsg.text, "OK", 2);

		// if(msgsnd(msg_id, (void *)&asendmsg, 128, 0) < 0){
		// 	printf("send msg error \n");
		// 	return 0;
		// }

        // continue;
        
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

                            p_framebuffer[i] = rgbw >> 24;    // R
                        }else if(loop_cnt%3 == 1){
                            p_framebuffer[i] = rgbw >> 16;    // G
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

        real_frame_len = 0;
        rgb_index = 0;
        loop_cnt = 0;
        index = 0;

        printf("cnt = %d\n", cnt);

        //usleep(5000);

        // send ok to msg queue
        memset(&asendmsg, 0, sizeof(asendmsg));
		asendmsg.type = 200;
		memcpy(asendmsg.text, "OK", 2);

		if(msgsnd(msg_id, (void *)&asendmsg, 128, 0) < 0){
			printf("send msg error \n");
			return 0;
		}
    }
}

int main(void)
{
    subctrl_configuration();

    parse_configuration_file();

    // 共享内存
    shm_key = ftok("/tmp/myshm",'a');

    if(shm_key < 0){
        printf("creat key failure\n");
        return -1;
    }

    printf("creat key sucess\n");

    shmid = shmget((key_t)shm_key, sizeof(shared_user_st), IPC_CREAT | 0777);
    if(shmid < 0){
        printf("creat share memory failure\n");
        return -1;
    }
    printf("creat share memory sucess shmid=%d\n",shmid);

    shared_user = (shared_user_st *)shmat(shmid,NULL,0);
    if(shared_user == NULL){
	   printf("parent process:shmat function failure\n");
	   return -3;
    }

    // 消息队列
    msg_key = ftok("/tmp/mymsg",'a');
    msg_id = msgget(msg_key, 0666 | IPC_CREAT);

	if(msg_id == -1){
		printf("open msg error \n");
		return 0;
	}

    printf("create msg success, msg_id=%d\n",msg_id);

    signal(SIGINT, handler);
    atexit(atexit_handler);
    
    pthread_t tid1;
    pthread_create(&tid1, NULL, (void *)thread_send_rgbdata, NULL);

    /* 发送ArtPoll */
    while(1) 
    {
        //printf("main running...\n");
        // pause();
        // //printf("pause after...\n");
        // pthread_cond_signal(&condv);
        // kill(pid, SIGUSR2);//server may write share memory

        sleep(3);   
    }

    return 0;
}
