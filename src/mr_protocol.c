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
#include "mr_protocol.h"

#define MAX_SUBCTRL_NUM    3

struct sockaddr_in mr_local;
struct sockaddr_in mr_dev;

int mr_socket;
socklen_t len;
int send_length;

int cmd;

uint8_t recvbuf[1024] = {0};

uint8_t mr_send_poll[] = {
    0x4d, 0x52, 0x4b, 0x4a, 
    0xff, 0xff, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x82, 0x16, 0xe2, 0x22, 0x6c, 0x54, 
    0x00, 0x1b, 0x24, 0xcd, 0xd0, 0xe8, 0x08, 0x00, 0x45, 0x00, 0x01, 0x20, 0x01, 0x00, 0x00, 0x00, 
    0x20, 0x11, 0x10, 0xaa, 0xc0, 0xa8, 0xc7, 0x7b, 0xff, 0xff, 0xff, 0xff, 0x11, 0x11, 0x11, 0x11, 
    0x01, 0x0c, 0x00, 0x00 
};

// 查询硬件参数
uint8_t mr_ack_hardware_param[] = {
    0x4d, 0x52, 0x4b, 0x4a, 
    0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x01, 0x82, 0x16, 0xe2, 0x22, 0x6c, 0x54, 
    0x00, 0x1b, 0x24, 0xcd, 0xd0, 0xe8, 0x08, 0x00, 0x45, 0x00, 0x01, 0x20, 0x01, 0x00, 0x00, 0x00, 
    0x20, 0x11, 0x10, 0xaa, 0xc0, 0xa8, 0xc7, 0x7b, 0xff, 0xff, 0xff, 0xff, 0x11, 0x11, 0x11, 0x11, 
    0x01, 0x0c, 0x00, 0x00
};

// 清除固定ID
uint8_t clear_fixid0[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

uint8_t clear_fixid1[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x20, 0x08, 0x00, 0x00, 0x01
};

uint8_t clear_fixid2[] = {
    0x4d, 0x52, 0x4b, 0x4a, 
    0x01, 0x00, 0x00, 0x05, 0x00, 0x20, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t clear_fixid3[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x0f, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

uint8_t clear_fixid4[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

// 写入固定ID
uint8_t set_fixid0[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

uint8_t set_fixid1[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x20, 0x08, 0x00, 0x00, 0x01
};

uint8_t set_fixid2[] = {
    0x4d, 0x52, 0x4b, 0x4a,
    0x01, 0x00, 0x00, 0x05, 0x00, 0x20, 0x08, 0x00, 0x00, 0x01, 0x01, 0x00, 0xfe, 0xff, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t set_fixid3[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x0f, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

uint8_t set_fixid4[] = {
    0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

// 写入硬件参数
uint8_t mr_set_hardware_param0[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t mr_set_hardware_param1[] = {0x4d, 0x52, 0x4b, 0x4a, 0xff, 0xff, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
uint8_t mr_set_hardware_param2[] = {0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x08, 0x00, 0x00, 0x01};   
uint8_t mr_set_hardware_param3[] = {0x4d, 0x52, 0x4b, 0x4a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x01};
uint8_t mr_set_hardware_param4[270] = {0};
// uint8_t mr_set_hardware_param4[] = {
//     0x4d, 0x52, 0x4b, 0x4a,
//     0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x01, 0x4d, 0x52, 0x4b, 0x4a, 0xc8, 0x06,  
//     0x54, 0x4d, 0x31, 0x38, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00, 0x35, 0x2a, 0x29, 0x44, 0x46, 0xab,  
//     0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x90, 0x46, 0x16, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };  // 硬件参数数组--------------------------------------------------------------------------------

uint8_t mr_set_hardware_param5[] = {
    0x4d, 0x52, 0x4b, 0x4a, 
    0x01, 0x00, 0x00, 0x05, 0x00, 0x01, 0x0f, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t mr_set_hardware_param6[] = {
    0x4d, 0x52, 0x4b, 0x4a, 
    0x01, 0x00, 0x00, 0x05, 0x00, 0x02, 0x0f, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


//查询硬件参数
void query_hardware_param(void)
{
    printf("query_hardware_param...\n");

    printf("Please enter the sub-control number to be queried: ");
    scanf("%d", &cmd);
    getchar();
    printf("\n");

    if((cmd > 0) && (cmd <= MAX_SUBCTRL_NUM)){
        mr_ack_hardware_param[4] = cmd;
    }else{
        printf("error number!\n");
        exit(0);
    }

    send_length = sendto(mr_socket, mr_ack_hardware_param, sizeof(mr_ack_hardware_param), 0, (struct sockaddr *)&mr_dev, len);
    if(send_length < 0){
        perror("sendto");
    }else{
        printf("send_length = %d\n", send_length);
    }

    struct sockaddr_in node;
    bzero(&node, sizeof(node));

    struct sockaddr_in mr_dev_ack;

    int mr_ack = socket(AF_INET, SOCK_DGRAM, 0);

    mr_dev_ack.sin_family = AF_INET;
    mr_dev_ack.sin_addr.s_addr = inet_addr("255.255.255.255");
    mr_dev_ack.sin_port = htons(4369);

    int iOptval = 1;
    if (setsockopt(mr_ack, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int)) < 0){
            printf("setsockopt failed!");
        }

    if(bind(mr_ack, (struct sockaddr*)&mr_dev_ack, sizeof(mr_dev_ack)) < 0){
        perror("bind");
        exit(0);
    }

    ssize_t s = recvfrom(mr_ack, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&node, &len);
    if(s > 0){
        for(int i=0; i<s; i++)
        {
            if((i%8 == 0) && (i != 0)) 
                printf(" ");
            if(i%16 == 0)
                printf("\n");
            printf("%02x ", recvbuf[i]);
        }
    }

    printf("\n");

    // 解析硬件参数
    hardware_param_t hardware_param;

    while(1)
    {
        if(recvbuf[0] == 0x4d && recvbuf[1] == 0x52 && recvbuf[2] == 0x4b && recvbuf[3] == 0x4a){
            char *pc = (char *)(recvbuf+6);
            int i = 0;

            do
            {
                hardware_param.name[i] = pc[i];
                i++;
            } while ((pc[i] != 0x00) && (pc[i] != 0x01));

            hardware_param.name[i] = 0;
            hardware_param.color_num = pc[i];
            
            uint16_t *ps = (uint16_t *)(pc+i+3);
            hardware_param.bright_adjust = (uint8_t)ntohs(ps[0]);

            hardware_param.bright_channel_A = (uint8_t)(ntohs(ps[1]) >> 8);
            hardware_param.bright_channel_B = (uint8_t)ntohs(ps[1]);

            hardware_param.bright_channel_C = (uint8_t)(ntohs(ps[2]) >> 8);
            hardware_param.bright_channel_D = (uint8_t)ntohs(ps[2]);

            hardware_param.gray_scale =  ntohs(ps[5]);
            hardware_param.channel_num = ntohs(ps[7]);

            hardware_param.clock_frequency = ntohs(ps[8]);
            
            hardware_param.clock_duty = (uint8_t)(ntohs(ps[9]) >> 8);
            hardware_param.GAMA = (uint8_t)(ntohs(ps[9]));

            hardware_param.disconnect_state = (ntohs(ps[37]));

            // 打印参数
            printf("成功: %d\n", cmd);
            printf("芯片名字: %s\n", hardware_param.name);
            printf("通道数: %ld\n", hardware_param.channel_num);
            printf("灰度等级: %d\n", hardware_param.gray_scale);
            printf("时钟频率: %.2fMHz\n", (float)hardware_param.clock_frequency/1000.0);
            printf("时钟占空比: %d%%\n", hardware_param.clock_duty);
            printf("颜色数: %d\n", (hardware_param.color_num == 0 ? 3 : 4));
            printf("亮度调节: %d\n", hardware_param.bright_adjust);
            printf("通道亮度A: %d\n", hardware_param.bright_channel_A);
            printf("通道亮度B: %d\n", hardware_param.bright_channel_B);
            printf("通道亮度C: %d\n", hardware_param.bright_channel_C);
            printf("通道亮度D: %d\n", hardware_param.bright_channel_D);
            printf("GAMA: %.1f\n", (float)hardware_param.GAMA/10.0);
            if(hardware_param.disconnect_state == 0){
                printf("信号断开后: 黑屏\n");
            }else{
                printf("信号断开后: 保留最后一帧\n");
            }

            break;
        }else{
            printf("失败 %d\n", cmd);
            break;
        }
    }

    close(mr_ack);
}

//写入硬件参数
void revise_hardware_param(void)
{
    printf("revise_hardware_param...\n");

    // 定义数据参数
    hardware_param_t hardware_param = {0};
    memcpy(&hardware_param.name, "WS2812B", strlen("WS2812B"));   // 芯片名称
    hardware_param.channel_num = 3072;   // 通道数
    hardware_param.gray_scale = 256;   // 灰度等级
    hardware_param.clock_frequency = 1800;   // 时钟频率
    hardware_param.clock_duty = 50;      // 时钟占空比
    hardware_param.color_num = 1;        // 颜色数
    hardware_param.bright_adjust = 88;   // 亮度调节
    hardware_param.bright_channel_A = 12;   // 通道亮度A
    hardware_param.bright_channel_B = 34;   // 通道亮度B
    hardware_param.bright_channel_C = 56;   // 通道亮度C
    hardware_param.bright_channel_D = 78;   // 通道亮度D
    hardware_param.GAMA = 22;             // GAMA 
    hardware_param.disconnect_state = 1;    // 信号断开状态

    // 填充硬件参数到数组中
    mr_set_hardware_param4[0] = 0x4d; mr_set_hardware_param4[1] = 0x52; mr_set_hardware_param4[2] = 0x4b; mr_set_hardware_param4[3] = 0x4a; 
    mr_set_hardware_param4[4] = 0x01; mr_set_hardware_param4[5] = 0x00; mr_set_hardware_param4[6] = 0x00; mr_set_hardware_param4[7] = 0x05; 
    mr_set_hardware_param4[8] = 0x00; mr_set_hardware_param4[9] = 0x00; mr_set_hardware_param4[10] = 0x0f; mr_set_hardware_param4[11] = 0x00; 
    mr_set_hardware_param4[12] = 0x00; mr_set_hardware_param4[13] = 0x01;

    mr_set_hardware_param4[14] = 0x4d;
    mr_set_hardware_param4[15] = 0x52;
    mr_set_hardware_param4[16] = 0x4b;
    mr_set_hardware_param4[17] = 0x4a;

    mr_set_hardware_param4[18] = 0xc8;
    mr_set_hardware_param4[19] = strlen(hardware_param.name);
    memcpy(mr_set_hardware_param4+20, &hardware_param.name, strlen(hardware_param.name));

    uint16_t index = 19+mr_set_hardware_param4[19]+1;

    mr_set_hardware_param4[index] = hardware_param.color_num;
    mr_set_hardware_param4[index+1] = 0x00;
    mr_set_hardware_param4[index+2] = 0x00;
    mr_set_hardware_param4[index+3] = 0x00;

    mr_set_hardware_param4[index+4] = hardware_param.bright_adjust;
    mr_set_hardware_param4[index+5] = hardware_param.bright_channel_A;
    mr_set_hardware_param4[index+6] = hardware_param.bright_channel_B;
    mr_set_hardware_param4[index+7] = hardware_param.bright_channel_C;
    mr_set_hardware_param4[index+8] = hardware_param.bright_channel_D;

    mr_set_hardware_param4[index+9] = 0xab;
    mr_set_hardware_param4[index+10] = 0x00;
    mr_set_hardware_param4[index+11] = 0x00;
    mr_set_hardware_param4[index+12] = 0x00;

    mr_set_hardware_param4[index+13] = hardware_param.gray_scale >> 8;
    mr_set_hardware_param4[index+14] = hardware_param.gray_scale;

    mr_set_hardware_param4[index+15] = 0x00;
    mr_set_hardware_param4[index+16] = 0x00;

    mr_set_hardware_param4[index+17] = hardware_param.channel_num >> 8;
    mr_set_hardware_param4[index+18] = hardware_param.channel_num;

    mr_set_hardware_param4[index+19] = hardware_param.clock_frequency >> 8;
    mr_set_hardware_param4[index+20] = hardware_param.clock_frequency;

    mr_set_hardware_param4[index+21] = hardware_param.clock_duty;
    mr_set_hardware_param4[index+22] = hardware_param.GAMA;

    mr_set_hardware_param4[index+77] = hardware_param.disconnect_state;

    for(int i=0; i<270; i++)
    {
        if((i%8 == 0) && (i != 0)) 
            printf(" ");
        if(i%16 == 0)
            printf("\n");
        printf("%02x ", mr_set_hardware_param4[i]);
    }

    printf("\n");

    sendto(mr_socket, mr_set_hardware_param1, sizeof(mr_set_hardware_param1), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param2[4] = i;
        sendto(mr_socket, mr_set_hardware_param2, sizeof(mr_set_hardware_param2), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param2[4] = i;
        sendto(mr_socket, mr_set_hardware_param2, sizeof(mr_set_hardware_param2), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param3[4] = i;
        sendto(mr_socket, mr_set_hardware_param3, sizeof(mr_set_hardware_param3), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }
    
    sendto(mr_socket, mr_set_hardware_param0, sizeof(mr_set_hardware_param0), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param4[4] = i;
        sendto(mr_socket, mr_set_hardware_param4, sizeof(mr_set_hardware_param4), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param5[4] = i;
        sendto(mr_socket, mr_set_hardware_param5, sizeof(mr_set_hardware_param5), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        mr_set_hardware_param6[4] = i;
        sendto(mr_socket, mr_set_hardware_param6, sizeof(mr_set_hardware_param6), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }
}

//写入固定ID
void revise_fixid(void)
{
    printf("revise_fixid...\n");

    sendto(mr_socket, set_fixid0, sizeof(set_fixid0), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        set_fixid1[4] = i; 
        sendto(mr_socket, set_fixid1, sizeof(set_fixid1), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=1; i<=MAX_SUBCTRL_NUM; i++)
    {
        set_fixid2[4] = i; 
        set_fixid2[14] = i; 
        set_fixid2[16] = 0xff-i;
        sendto(mr_socket, set_fixid2, sizeof(set_fixid2), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }
    
    sendto(mr_socket, set_fixid3, sizeof(set_fixid3), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    sendto(mr_socket, set_fixid4, sizeof(set_fixid4), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    // sendto(mr_socket, set_fixid0, sizeof(set_fixid0), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    // set_fixid1[4] = 2; 
    // sendto(mr_socket, set_fixid1, sizeof(set_fixid1), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    // set_fixid2[4] = 2; 
    // set_fixid2[16] = 0xfd;
    // sendto(mr_socket, set_fixid2, sizeof(set_fixid2), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    // sendto(mr_socket, set_fixid3, sizeof(set_fixid3), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    // sendto(mr_socket, set_fixid3, sizeof(set_fixid3), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
}

//清除固定ID
void clear_fixid(void)
{
    printf("clear_fixid...\n");

    sendto(mr_socket, clear_fixid0, sizeof(clear_fixid0), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);

    for(int i=0; i<MAX_SUBCTRL_NUM; i++)
    {
        clear_fixid1[4] = i+1;
        sendto(mr_socket, clear_fixid1, sizeof(clear_fixid1), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    for(int i=0; i<MAX_SUBCTRL_NUM; i++)
    {
        clear_fixid2[4] = i+1;
        sendto(mr_socket, clear_fixid2, sizeof(clear_fixid2), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    }

    sendto(mr_socket, clear_fixid3, sizeof(clear_fixid3), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
    sendto(mr_socket, clear_fixid4, sizeof(clear_fixid4), 0, (struct sockaddr *)&mr_dev, len); usleep(100000);
}

void subctrl_configuration(void)
{
    int iOptval = 1;
    len = sizeof(mr_local);

    // 明瑞分控接口
    mr_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (setsockopt(mr_socket, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &iOptval, sizeof(int)) < 0){
        printf("setsockopt failed!");
    }

    mr_local.sin_family = AF_INET;
    mr_local.sin_addr.s_addr = inet_addr("199.199.199.199");
    mr_local.sin_port = htons(19794);

    if(bind(mr_socket, (struct sockaddr*)&mr_local, sizeof(mr_local)) < 0){
        perror("bind");
        exit(0);
    }

    mr_dev.sin_family = AF_INET;
    inet_aton("255.255.255.255", &mr_dev.sin_addr);
    mr_dev.sin_port = htons(19274);

    for(int i=0; i<10; i++)
    {
        send_length = sendto(mr_socket, mr_send_poll, sizeof(mr_send_poll), 0, (struct sockaddr *)&mr_dev, len);
        if( send_length < 0){
            perror("sendto");
        }else{
            printf("send_length = %d\n", send_length);
        }

        usleep(100000);
    }
    
    while(1)
    {
        printf("***************************************************************\n");
        printf("1:返回硬件参数 2:写入硬件参数 3:写入固定ID 4:清除固定ID 5:退出.\n");
        printf("***************************************************************\n");

        printf("input num: ");
        scanf("%d", &cmd);
        getchar();
        printf("\n");

        switch(cmd)
        {
            case 1:
                query_hardware_param();
                break;
            case 2:
                revise_hardware_param();
                break;
            case 3:
                revise_fixid();
                break;
            case 4:
                clear_fixid();
                break;
            case 5:
                return;

            default: break;
        }
    }    

    
}