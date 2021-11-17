#ifndef _MR_PROTOCOL_H_
#define _MR_PROTOCOL_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef struct{
    char name[16];               // 芯片名字
    uint16_t channel_num;        // 通道数
    uint16_t gray_scale;          // 灰度等级
    uint16_t clock_frequency;     // 时钟频率
    uint8_t clock_duty;          // 时钟占空比
    uint8_t color_num;           // 颜色数
    uint8_t bright_adjust;       // 亮度调节
    uint8_t bright_channel_A;    // 通道亮度A
    uint8_t bright_channel_B;    // 通道亮度B
    uint8_t bright_channel_C;    // 通道亮度C
    uint8_t bright_channel_D;    // 通道亮度D
    uint8_t GAMA;                // GAMA
    uint16_t disconnect_state;    // 信号断开状态

}hardware_param_t;
void subctrl_configuration(void);

#if defined(__cplusplus)
}
#endif

#endif