#ifndef _SW6208_H
#define _SW6208_H
#include <iic.h>
#define SW6208_address 0x3C // 设备地址
// define MAX_AES_BUFFER_SIZ 1
//  ntc相关参数
#define BX 3435
#define T25 298.15
#define R25 10000
#ifndef uint16_t
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
#endif

float batteryV();                                              // 电池电压的读取和计算
float battery_outinV();                                        // 输入输出电压
float battery_ictemp();                                        // ic温度
float battery_ntcV();                                          // ntc电压
float SYS_inA();                                               // 系统输入电流
float SYS_outA();                                              // 系统输出电流的获取
float battery_volume();                                        // 电池容量获取
uint16_t battery_per();                                        // 电池百分比获取
void sysstate(uint8_t *sys_state, uint8_t *A_C, float *sys_a); // 系统状态 2充电 1 放电          1A  4C  5AC         充放电流

uint8_t Sink_Protocol();   // 充电协议
uint8_t Source_Protocol(); // 放电协议

uint8_t xdlzt(); // 小电流状态
void kqxdl();    // 开启或关闭小电流

void AC_OFF();     // 关闭所有输出口  bit 自动清零  子线程持续关闭

void SW6208init(); // 1.按键作用      2.轻载时间设置为8s     3.小电流使能    4.NTC门限改为 60℃    5.打开12V输入

#endif
