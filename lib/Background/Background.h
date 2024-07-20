#ifndef _Background_H
#define _Background_H

void DisplayInit(); // 屏幕初始化函数
void lcdRotation(); // 连接蓝牙实时刷新屏幕方向

void RefreshDisplay();           // 刷新画面
void sys_init(uint16_t timenum); // 系统初始化页面   没用   用LOGO页面取代
void PowerLOGO();                // 开机LOGO
//---------------主题-----------
void lcdlayout01(uint16_t xunhuan, uint8_t bat_per, float battery_V, float ic_temp, float sys_outinv, float battery_A, float bat_ntc,
                 uint8_t sys, uint8_t smalla, uint8_t A_C, uint8_t bt_icon, uint8_t sinkProtocol, uint8_t sourceProtocol);
void BackgroundTime2(uint8_t A_C, uint8_t bt_icon, float sys_outinv, float battery_A, float sys_w, float ic_temp, float bat_ntc,
                     uint8_t bat_per, uint16_t xunhuan, uint8_t sys, uint8_t sinkProtocol, uint8_t sourceProtocol);
void BackgroundTime3(uint8_t week, float batv, float sysv, uint16_t workp, uint16_t ACstate,
                     float bata, float sysw, float batvolume, int xnum, uint16_t batper, uint8_t bt_icon);
void BackgroundTime3_2(uint8_t month, uint8_t day, float battemp, float temp, uint8_t hour, uint8_t minute, uint8_t sec, uint8_t xdl, uint16_t cycle);

void BackgroundTime4(float battery_V, float sys_outinv, uint8_t sys, float battery_A, float sys_w, uint8_t bat_per,
                     uint8_t bt_icon, float ic_temp, float bat_ntc, uint8_t smalla, uint8_t A_C);
void BackgroundTime5(uint8_t smalla, float battery_V, float sys_outinv, uint8_t sys, uint8_t A_C, float battery_A, float sys_w, uint8_t bat_per, float bat_m, uint8_t bt_icon,
                     float ic_temp, float bat_ntc, uint8_t sinkProtocol, uint8_t sourceProtocol, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t sec, uint8_t week);
void BackgroundTime6(uint8_t sys, uint8_t A_C, float sys_outinv, float sys_a, float sys_w, float bat_v, uint8_t smalla, uint8_t cycle, uint8_t bat_per, float bat_m,
                     float bat_ntc, uint8_t bt_icon, uint8_t sinkProtocol, uint8_t sourceProtocol);
void BackgroundTime7(uint8_t sys, uint8_t cycle, uint8_t smalla, float sys_outinv, float sys_a, float sys_w, uint8_t bat_per, uint8_t bt_icon,
                     float bat_ntc, uint8_t sinkProtocol, uint8_t sourceProtocol);
//------------------
void Backgroundyan(uint16_t pngindex); // 动画
void offscreen();                      // 息屏
void onscreen();                       // 亮屏

void lost_Page(); // 丢失模式  关闭所有输出口

void WiFi_Page();            // 等待连接WiFi页面
void ota_AP();               // 配网信息提示
void ota_Page(int a, int b); // 更新页面
#endif
