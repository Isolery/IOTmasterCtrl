#ifndef _DEV_INFO_H_
#define _DEV_INFO_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef struct  
{   
    uint16_t host_port;      // 端口地址
    char ip_addr[15];        // ip地址("192.168.0.123")
    uint8_t mac_addr[6];     // mac地址
}dev_info_t;

int get_device_info(dev_info_t *dev_info, int num);

#if defined(__cplusplus)
}
#endif

#endif