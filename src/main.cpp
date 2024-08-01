#include <SW6208.h>
#include <Background.h>
#include <Ticker.h>
#include <ESP32bt.h>
#include <ck22.h>
#include <eeprom32.h>
#include <ArduinoJson.h>
#include <espds1302.h>
#include <OneButton.h>
#include "ota.h"
#include <WiFi.h>
// #include "soc/rtc_wdt.h" // 设置rtc看门狗用

#define BUTTON_PIN_BITMASK 0x0010 // GPIOs 4    io4 按钮

BleKeyboard bleKeyboard("AC225", "OCRC", 50); // 蓝牙

uint8_t cycleHigh = 0, cycleLow = 0, bt_icon = 0, sleeptime = 30, espthem = 1, yan = 0;
uint16_t cycle = 0;
Ticker ticker1;
OneButton button(4, true);

void Task_OTA(void *pvParameters);    // OTA更新 子线程
void Task_AC_OFF(void *pvParameters); // 关闭所有输出口 子线程
void Task_RTC(void *pvParameters);    // 看门狗

void setup()
{
    Serial.begin(115200);

    pinMode(4, INPUT_PULLUP);
    pinMode(27, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);                              // 唤醒引脚配置 低电平唤醒
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ALL_LOW); // 唤醒引脚配置 低电平唤醒
    IICinit();                                                                 // 初始化 IIC 通讯
    EEPROMinit();                                                              // 初始化 EEPROM 寄存器
    DisplayInit();                                                             // 显示初始化
    // PowerLOGO(); // 开机LOGO
    // delay(1000);

    // 配置倒计时  蓝牙/屏幕
    if (EEPROM.read(5) < 30 && EEPROM.read(5) != 0 || EEPROM.read(5) > 120) // 亮屏时间30-120s
        EEPROM.write(5, 30);                                                // 不在范围  设置为 30
    if (EE_BLETimeRead() < 150 || EE_BLETimeRead() > 3600)                  // 蓝牙休眠时间150-3600s
        EE_BLETimeWrite(150);                                               // 不在范围  设置为 150
    EEPROM.commit();                                                        // 保存

    // 丢失模式  关闭所有输出口
    if (EEPROM.read(12) == 1) // 小程序挂失  关闭所有输出口
    {
        xTaskCreatePinnedToCore(Task_AC_OFF,   // 具体实现的函数
                                "Task_AC_OFF", // 任务名称
                                1024,          // 堆栈大小
                                NULL,          // 输入参数
                                1,             // 任务优先级
                                NULL,          //
                                0              // 核心  0/1  不指定
        );
        lost_Page();      // 丢失设备提示页
        vTaskDelay(5000); // 提示5s
    }

    // OTA更新
    if (EEPROM.read(11) == 1) // 小程序给更新确认
    {
        EEPROM.write(11, 0);                // 写非1  更新结束
        EEPROM.commit();                    // 保存
        xTaskCreatePinnedToCore(Task_OTA,   // 具体实现的函数
                                "Task_OTA", // 任务名称
                                10240,      // 堆栈大小
                                NULL,       // 输入参数
                                1,          // 任务优先级
                                NULL,       //
                                0           // 核心  0/1  不指定
        );
        updateBin(); // OTA  含联网
        vTaskDelay(1000);
        esp_restart(); // 重启 （失败情况，防止乱跳）
    }

    // CK22AT  2314
    if (keros_main() != 1)
        esp_deep_sleep_start();

    // 看门狗
    xTaskCreatePinnedToCore(Task_RTC,   // 具体实现的函数
                            "Task_RTC", // 任务名称
                            1024,       // 堆栈大小
                            NULL,       // 输入参数
                            1,          // 任务优先级
                            NULL,       //
                            0           // 核心  0/1  自动选择
    );

    // sw6208开机配置
    SW6208init(); // 1.按键作用取消      2.轻载时间设置为8s     3.小电流使能    4.NTC门限改为 60℃    5.打开12V输入
    Serial.println("ESP32_setup_OK");
}

void loop()
{
    delay(200);
    float battery_V, sys_outinv, ic_temp, ntc_v, sys_a, sys_w, bat_m, bat_ntc;                    // 变量电池电压   系统输入输出电压   ic温度   ntc电压   输入/输出电流   系统功率   电池容量   电池温度
    uint16_t year, time, pass;                                                                    // 年份   读蓝牙链接时间    四位密码
    uint8_t bat_per, sys, A_C, sinkProtocol, sourceProtocol, month, day, hour, minute, sec, week; // 电池百分比   系统充放电状态   系统输出口状态   快充协议  快放协议    月  日  时  分  秒  星期
    uint8_t smalla, a1, c1, topic, ota, idlock;                                                   // 小电流   A1口状态1开0关     C1口状态1开0关      屏幕方向1上3下     OTA    ID锁

    uint8_t currentTime = EEPROM.read(5); // 睡眠时间
    unsigned long currentTime1;           // 程序执行计时
    while (currentTime >= 0)              // 屏幕睡眠时间 转次数
    {
        battery_V = batteryV(); // 电池电压
        delay(5);
        sysstate(&sys, &A_C, &sys_a); // 冲放电状态及输出电流的判断   1A  4C  5AC
        delay(5);
        sys_outinv = battery_outinV(); // 输入输出电压
        ic_temp = battery_ictemp();    // ic温度
        delay(5);
        sys_w = sys_outinv * sys_a; // 系统功率大小
        bat_per = battery_per();    // 电池百分比
        delay(5);
        bat_m = battery_volume() * bat_per / 100; // 电池容量判断  实时容量    总容量x电池百分比
        bat_ntc = battery_ntcV();                 // 电池温度
        delay(5);
        EE_CycleCount(bat_per);     // 电池循环次数的判断
        cycle = EEPROM.read(2) / 2; // 判断之后读取  电池循环次数
        delay(5);
        printTime(&year, &month, &day, &hour, &minute, &sec, &week); // 从DS1302获取时间数据     年 月 日 时 分 秒 周
        delay(5);
        smalla = xdlzt();                   // 小电流状态
        sinkProtocol = Sink_Protocol();     // 充电协议
        sourceProtocol = Source_Protocol(); // 放电协议

        Serial.println(ESP.getEfuseMac() & 0X0000FFFFFFFFFFFF, HEX); // chipID  //MAC

    beijing0:
        switch (EEPROM.read(4)) // 读取主题号
        // switch (5)
        {
        case 1:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing1;
                        break;
                    }
                }
            }
            else
            beijing1:
                lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
            break;
        case 2:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing2;
                        break;
                    }
                }
            }
            else
            beijing2:
                BackgroundTime2(battery_V, sys_outinv, sys, sys_a, sys_w, bat_per, bt_icon, ic_temp, bat_ntc, smalla, A_C);
            break;
        case 3:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing3;
                        break;
                    }
                }
            }
            else
            {
            beijing3:
                BackgroundTime3(week, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_m, cycle, bat_per, bt_icon);
                BackgroundTime3_2(month, day, bat_ntc, ic_temp, hour, minute, sec, smalla, cycle);
            }
            break;
        case 4:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing4;
                        break;
                    }
                }
            }
            else
            {
            beijing4:
                BackgroundTime4(A_C, bt_icon, sys_outinv, sys_a, sys_w, ic_temp, bat_ntc, bat_per, cycle, sys, sinkProtocol, sourceProtocol);
            }
            break;
        case 5:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing5;
                        break;
                    }
                }
            }
            else
            {
            beijing5:
                BackgroundTime5(smalla, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_per, bat_m, bt_icon, ic_temp, bat_ntc, sinkProtocol, sourceProtocol, month, day, hour, minute, sec, week);
            }
            break;
        case 6:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing6;
                        break;
                    }
                }
            }
            else
            {
            beijing6:
                BackgroundTime6(sys, cycle, smalla, sys_outinv, sys_a, sys_w, bat_per, bt_icon, bat_ntc, sinkProtocol, sourceProtocol);
            }
            break;
        case 7:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(50);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing7;
                        break;
                    }
                }
            }
            else
            {
            beijing7:
                BackgroundTime7(sys, A_C, sys_outinv, sys_a, sys_w, battery_V, smalla, cycle, bat_per, bat_m, bat_ntc, bt_icon, sinkProtocol, sourceProtocol);
            }
            break;
        default:
            if (yan == 1)
            {
                for (uint16_t i = 1; i < 20; i++)
                {
                    Backgroundyan(i);
                    delay(100);
                    if (digitalRead(4) == 0)
                    {
                        vTaskDelay(300);
                        yan ^= 1;
                        goto beijing10;
                        break;
                    }
                }
            }
            else
            {
            beijing10:
                lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
            }
            break;
        }
        if (currentTime != 0)
            currentTime--; // 睡眠时间倒计时(循环次数，大概1s/fps)   到0退出循环
        if (currentTime == 1)
            break; // 跳出大循环  睡眠
        Serial.print("lcdSleepTime: ");
        Serial.println(currentTime);
        currentTime1 = millis();                     // 板运行当前程序的时间
        while (millis() - currentTime1 < 1000 - 165) // 延时 大概 1s 刷新 一次   以上执行时间165ms↔
        {
            if (digitalRead(4) == 0)
            {
                delay(200);              // 消抖
                if (digitalRead(4) == 0) // 此处进入眼睛动画
                {
                    delay(200);                // 延时
                    if (digitalRead(4) == LOW) // 拉低准备进入蓝牙
                    {
                        delay(1000);               // 延时
                        if (digitalRead(4) == LOW) // 进入蓝牙
                        {
                            // bleKeyboard.begin();                           // 打开蓝牙
                            // delay(1000);                                   // 给蓝牙启动缓冲
                            ticker1.once(EE_BLETimeRead(), esp_deep_sleep_start); // 小程序改的蓝牙打开时间  (150-3600,执行函数)
                            DynamicJsonDocument jsonBuffer1(512);
                            DynamicJsonDocument jsonBuffer2(512);
                            bt_icon = 1;
                            switch (EEPROM.read(4))
                            {
                            case 1:
                                lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
                                break;
                            case 2:
                                BackgroundTime2(battery_V, sys_outinv, sys, sys_a, sys_w, bat_per, bt_icon, ic_temp, bat_ntc, smalla, A_C);
                                break;
                            case 3:
                                BackgroundTime3(week, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_m, cycle, bat_per, bt_icon);
                                BackgroundTime3_2(month, day, bat_ntc, ic_temp, hour, minute, sec, smalla, cycle);
                                break;
                            case 4:
                                BackgroundTime4(A_C, bt_icon, sys_outinv, sys_a, sys_w, ic_temp, bat_ntc, bat_per, cycle, sys, sinkProtocol, sourceProtocol);
                                break;
                            case 5:
                                BackgroundTime5(smalla, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_per, bat_m, bt_icon, ic_temp, bat_ntc, sinkProtocol, sourceProtocol, month, day, hour, minute, sec, week);
                                break;
                            case 6:
                                BackgroundTime6(sys, cycle, smalla, sys_outinv, sys_a, sys_w, bat_per, bt_icon, bat_ntc, sinkProtocol, sourceProtocol);
                                break;
                            case 7:
                                BackgroundTime7(sys, A_C, sys_outinv, sys_a, sys_w, battery_V, smalla, cycle, bat_per, bat_m, bat_ntc, bt_icon, sinkProtocol, sourceProtocol);
                                break;
                            default:
                                lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
                                break;
                            }
                            // AC_OFF();
                            bleKeyboard.begin(); // 打开蓝牙
                            delay(1000);         // 给蓝牙启动缓冲
                            while (1)
                            {
                                lcdRotation();                                               // 实时屏幕上下
                                cycle = EEPROM.read(2) / 2;                                  // 判断之后读取  电池循环次数
                                printTime(&year, &month, &day, &hour, &minute, &sec, &week); // 从DS1302获取时间数据     年 月 日 时 分 秒 周
                                battery_V = batteryV();                                      // 电池电压
                                delay(5);
                                sysstate(&sys, &A_C, &sys_a); // 冲放电状态及输出电流的判断
                                delay(5);
                                sys_outinv = battery_outinV(); // 输入输出电压
                                ic_temp = battery_ictemp();    // ic温度
                                delay(5);
                                sys_w = sys_outinv * sys_a;               // 系统功率大小
                                bat_per = battery_per();                  // 电池百分比
                                bat_m = battery_volume() * bat_per / 100; // 电池容量判断
                                bat_ntc = battery_ntcV();                 // 电池温度

                                topic = EEPROM.read(3);  // 读用户设置的值（1上3下）屏幕方向
                                time = EE_BLETimeRead(); // 读蓝牙链接时间

                                sinkProtocol = Sink_Protocol();     // 充电协议
                                sourceProtocol = Source_Protocol(); // 放电协议
                                smalla = xdlzt();                   // 读取小电流状态
                                Serial.print("smalla: ");
                                Serial.println(smalla);
                                // 打开或关闭小电流
                                if ((smalla == 0 && EEPROM.read(8) == 1) || (smalla == 1 && EEPROM.read(8) == 0)) // smalla状态和蓝牙给的设置不一样     注意：eeprom默认255,故不能用 != 判断
                                    kqxdl();                                                                      // 写1打开或关闭小电流

                                delay(5);
                                switch (EEPROM.read(4))
                                {
                                case 1:
                                    lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
                                    break;
                                case 2:
                                    BackgroundTime2(battery_V, sys_outinv, sys, sys_a, sys_w, bat_per, bt_icon, ic_temp, bat_ntc, smalla, A_C);
                                    break;
                                case 3:
                                    BackgroundTime3(week, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_m, cycle, bat_per, bt_icon);
                                    BackgroundTime3_2(month, day, bat_ntc, ic_temp, hour, minute, sec, smalla, cycle);
                                    break;
                                case 4:
                                    BackgroundTime4(A_C, bt_icon, sys_outinv, sys_a, sys_w, ic_temp, bat_ntc, bat_per, cycle, sys, sinkProtocol, sourceProtocol);
                                    break;
                                case 5:
                                    BackgroundTime5(smalla, battery_V, sys_outinv, sys, A_C, sys_a, sys_w, bat_per, bat_m, bt_icon, ic_temp, bat_ntc, sinkProtocol, sourceProtocol, month, day, hour, minute, sec, week);
                                    break;
                                case 6:
                                    BackgroundTime6(sys, cycle, smalla, sys_outinv, sys_a, sys_w, bat_per, bt_icon, bat_ntc, sinkProtocol, sourceProtocol);
                                    break;
                                case 7:
                                    BackgroundTime7(sys, A_C, sys_outinv, sys_a, sys_w, battery_V, smalla, cycle, bat_per, bat_m, bat_ntc, bt_icon, sinkProtocol, sourceProtocol);
                                    break;
                                default:
                                    lcdlayout01(cycle, bat_per, battery_V, ic_temp, sys_outinv, sys_a, bat_ntc, sys, smalla, A_C, bt_icon, sinkProtocol, sourceProtocol);
                                    break;
                                }
                                jsonBuffer1["chipid"] = ESP.getEfuseMac() & 0X0000FFFFFFFFFFFF;
                                jsonBuffer1["name"] = "AC225";                     // 设备名称
                                jsonBuffer1["software"] = "v4.1";                  // 固件版本
                                jsonBuffer1["hardware"] = "v3.0";                  // 硬件版本
                                jsonBuffer1["bat_cir"] = cycle;                    // 循环次数
                                jsonBuffer1["bat_V"] = String(battery_V, 3);       // 电池电压
                                jsonBuffer1["bat_A"] = String(sys_a, 3);           // 电流
                                jsonBuffer1["A_C"] = A_C;                          // AC口状态
                                jsonBuffer1["ic_temp"] = String(ic_temp, 3);       // ic温度
                                jsonBuffer1["sys_outinv"] = String(sys_outinv, 3); // 系统充放电压
                                jsonBuffer1["sys_w"] = String(sys_w, 3);           // 系统功率
                                jsonBuffer1["sys"] = sys;                          // 充放电状态
                                jsonBuffer1["bat_m"] = String(bat_m, 3);           // 电池容量
                                jsonBuffer1["bat_per"] = bat_per;                  // 百分比bat_per
                                jsonBuffer1["bat_ntc"] = String(bat_ntc, 3);       // 电池温度
                                String output1;
                                serializeJson(jsonBuffer1, output1);
                                jsonBuffer1.clear();
                                Serial.println("----------------sendTx------1111---------------------");
                                Serial.println(output1);
                                Serial.println("-----------------------------------------------------");
                                bleKeyboard.sendTx(output1); // 第一次发送数据
                                output1 = "";
                                delay(50);

                                // jsonBuffer1["topic_Num"] = EEPROM.read(4); // 主题编号
                                // jsonBuffer1["led-sleep"] = EEPROM.read(5); // 睡眠时间  最大存储255   小程序设置值  30  60  90  120  常亮
                                // jsonBuffer1["topic_dir"] = EEPROM.read(3);     // 屏幕方向  1  3
                                // jsonBuffer1["blt_Time"] = eepromread();        // 蓝牙打开及连接时间
                                // jsonBuffer1["small_A"] = xdlzt();              // 读取最新的小电流状态  发送给小程序

                                // serializeJson(jsonBuffer1, output1);
                                // jsonBuffer1.clear();
                                // Serial.println("---------------sendTx1------2222------");
                                // Serial.println(output1);
                                // Serial.println("--------------------------------------");
                                // bleKeyboard.sendTx1(output1); // 第二次发送数据
                                // output1 = "";
                                // delay(50);
                                if (Rxdata.length() > 0) // 蓝牙接收数据
                                {
                                    Serial.println("-------------------------------Rxdata:-----------");
                                    Serial.println(Rxdata);
                                    DeserializationError error = deserializeJson(jsonBuffer2, Rxdata);
                                    if (error)
                                    {
                                        Serial.print(F("deserializeJson() failed: ")); // F: Flash    内容存储到flash  节省RAM空间
                                        Serial.println(error.f_str());
                                        return;
                                    }
                                    // 解析JSON
                                    idlock = jsonBuffer2["btidlock"];       // ID锁
                                    year = jsonBuffer2["btyear"];           // 读取年
                                    month = jsonBuffer2["btmon"];           // 读取月
                                    day = jsonBuffer2["btday"];             // 读取天
                                    hour = jsonBuffer2["bthour"];           // 读取小时
                                    minute = jsonBuffer2["btmin"];          // 读取分
                                    sec = jsonBuffer2["btsec"];             // 读取秒
                                    week = jsonBuffer2["btweek"];           // 读取周几
                                    espthem = jsonBuffer2["btthem"];        // 读取主题
                                    sleeptime = jsonBuffer2["btsleeptime"]; // 读取睡眠时间
                                    topic = jsonBuffer2["bttopic"];         // 读用户设置的值（1上3下）	屏幕方向
                                    time = jsonBuffer2["bttime"];           // 读蓝牙开启状态的时间

                                    smalla = jsonBuffer2["btsmalla"]; // 读小电流开关设置
                                    ota = jsonBuffer2["btota"];       // OTA更新
                                    cycle = jsonBuffer2["btcycle"];   // 改写循环次数

                                    // 开始写入数据
                                    EEPROM.write(12, idlock);                                   // 写1 关闭所有输出口(丢失模式)
                                    initRTCtime(year, month, day, hour, minute, sec + 1, week); // 更新彩屏时间
                                    EEPROM.write(5, sleeptime);                                 // 写入屏幕睡眠倒计时
                                    EEPROM.write(8, smalla);                                    // 写入小电流设置
                                    EEPROM.write(11, ota);                                      // OTA更新  写1更新  更新处自动置零
                                    if (espthem != 0)
                                        EEPROM.write(4, espthem); // 写入主题编号
                                    if (topic != 0)
                                        EEPROM.write(3, topic); // 写入屏幕显示方向
                                    if (time != 0)
                                        EE_BLETimeWrite(time); // 写入蓝牙连接时间
                                    if (cycle == 1)
                                    {
                                        EEPROM.write(0, 0); // 循环判断一并清零
                                        EEPROM.write(1, 0); //
                                        EEPROM.write(2, 0); // 改写循环次数  写1清零
                                    }
                                    else if (cycle != 0)
                                        EEPROM.write(2, cycle); // 改写循环次数
                                    delay(5);
                                    EEPROM.commit(); // 保存
                                    delay(5);
                                    Rxdata = ""; // 清空
                                    Serial.println("-------------------------------RxEnd ! ! !-----------");
                                }
                                Serial.print("topic:\t");
                                Serial.println(EEPROM.read(3));
                                Serial.print("them:\t");
                                Serial.println(EEPROM.read(4));
                                Serial.print("sleepTime:\t");
                                Serial.println(EEPROM.read(5));
                                Serial.print("BleTime:\t");
                                Serial.println(EE_BLETimeRead());
                                Serial.print("smalla:\t");
                                Serial.println(EEPROM.read(8));
                                Serial.print("OTA:\t");
                                Serial.println(EEPROM.read(11));
                                Serial.print("IDLock_AC_OFF:\t");
                                Serial.println(EEPROM.read(12));
                                Serial.print("Cycle:\t");
                                Serial.print(EEPROM.read(2));
                                Serial.println("/2");

                                unsigned long currentTime2;
                                currentTime2 = millis(); // 程序执行到此时间
                                delay(10);
                                while (millis() - currentTime2 < 1000) // while延时
                                {
                                    if (digitalRead(4) == 0)
                                    {
                                        delay(300);
                                        if (digitalRead(4) == LOW)
                                        {
                                            delay(1000);
                                            if (digitalRead(4) == LOW)
                                            {
                                                offscreen(); // 息屏    给个假提示
                                                delay(1000); // 防止关闭蓝牙后  没及时松开 再次点亮屏幕
                                                esp_deep_sleep_start();
                                            }
                                        }
                                        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN); // 音量减  用于拍照功能
                                        Serial.println("KEY_MEDIA_VOLUME_DOWN");
                                        delay(500);
                                    }
                                }
                            }
                        }
                    }
                    yan ^= 1;
                    vTaskDelay(100);
                    goto beijing0;
                }
            }
        }
    }
    // 屏幕进入睡眠
    if (digitalRead(27) == 1) // 6208 工作状态
    {
        esp_deep_sleep_start();
    }
    offscreen();              // 息屏
    if (digitalRead(27) == 0) // 6208 未工作
    {
        while (1)
        {
            if (digitalRead(4) == 0) // 按io4
            {
                yan == 1;
                onscreen(); // 点亮屏幕
                break;
            }
            if (digitalRead(27) == 1) // 6208 在工作
            {
                onscreen(); // 点亮屏幕
                break;
            }
            vTaskDelay(300);
        }
    }
}
// OTA
void Task_OTA(void *pvParameters)
{
    // esp_task_wdt_add(NULL); // 给本任务添加看门口  NULL代表本任务
    Serial.print("Task_OTA on core: ");
    Serial.println(xPortGetCoreID()); // 所在核心
    while (1)
    {
        if (WiFi.status() == WL_CONNECTED) // WiFi连接成功
            ota_Page(a, b);                // lcd 显示进度
        else
            WiFi_Page(); // 正在连接WiFi...
        // ota_AP();     // lcd配网ip提示页面
        if (digitalRead(4) == LOW) // 按键单击退出更新
        {
            esp_deep_sleep_start();
            break;
        }
        vTaskDelay(600); // 慢一点循环，让OTA跑流畅点   //延时 退让资源同时喂狗
    }
}
// 丢失，锁输出
void Task_AC_OFF(void *pvParameters)
{
    while (1)
    {
        Serial.print("Task_AC_OFF on core: ");
        Serial.println(xPortGetCoreID()); // 所在核心
        AC_OFF();                         // 关闭AC口输出
        vTaskDelay(1000);                 // 延时  及喂狗
    }
}
// 看门狗，防死机
void Task_RTC(void *pvParameters)
{
    while (1)
    {
        Serial.print("Task_RTC on core: ");
        Serial.println(xPortGetCoreID()); // 所在核心
        vTaskDelay(2000);                 // 喂狗
    }
}
