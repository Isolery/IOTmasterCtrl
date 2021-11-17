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

#include "dev_info.h"

//#define MAX_INTERFACE     3

int get_device_info(dev_info_t *dev_info, int num)
{
    struct ifreq buf[num+1];              /* ifreq结构体  */  
    struct ifconf ifc;                    /* ifconf结构体 */  
 
    int if_len;     /* 接口数量 */  
    
    /* 建立IPv4的UDP套接字sock */  
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock < 0){
        perror("socket");
        return -1;
    }

    //获取本机ip地址和mac地址
    ifc.ifc_len = sizeof(buf);  
    ifc.ifc_buf = (char *) buf;

    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */  

    if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) == -1){  
        perror("SIOCGIFCONF ioctl");  
        return -1;  
    }  

    // buf[1] ==> 192.168.0.73
    // buf[2] ==> 192.168.0.74
    while(if_len-- > 1)
    {
        // 获取ip地址
        if (!(ioctl(sock, SIOCGIFADDR, (char *) &buf[if_len]))){  
        char *ip_adr = inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr);
        //printf("IP Address:%s\n", ip_adr);  
        memcpy(dev_info[if_len-1].ip_addr, ip_adr, strlen(ip_adr)+1);
        }else{  
            char str[256];  
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);  
            perror(str);  
        }  

        // 获取mac地址
        if (!(ioctl(sock, SIOCGIFHWADDR, (char *) &buf[if_len]))){  
            dev_info[if_len-1].mac_addr[0] = buf[if_len].ifr_hwaddr.sa_data[0];
            dev_info[if_len-1].mac_addr[1] = buf[if_len].ifr_hwaddr.sa_data[1];
            dev_info[if_len-1].mac_addr[2] = buf[if_len].ifr_hwaddr.sa_data[2];
            dev_info[if_len-1].mac_addr[3] = buf[if_len].ifr_hwaddr.sa_data[3];
            dev_info[if_len-1].mac_addr[4] = buf[if_len].ifr_hwaddr.sa_data[4];
            dev_info[if_len-1].mac_addr[5] = buf[if_len].ifr_hwaddr.sa_data[5];
        }else{  
            char str[256];  
            sprintf(str, "SIOCGIFHWADDR ioctl %s", buf[if_len].ifr_name);  
            perror(str);  
        }  
    }
}

