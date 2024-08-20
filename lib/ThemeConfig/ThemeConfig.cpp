#include <Arduino.h>
#include "ThemeConfig.h"
#include "Background.h"
#include "SW6208.h"
#include "eeprom32.h"
#include "espDS1302.h"
#include <ESP32bt.h>
#include <ArduinoJson.h>
#include "AgentConfig.h"

// #include <NimBLE-Arduino.h>

BleKeyboard bleKeyboard("AC225", "OCRC", 50); // 蓝牙

float bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m;                // 电池电压   系统电压  系统电流   ic温度    电池温度  电池实时容量
uint8_t bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol; // 电池百分比   系统充放电状态   系统输出口状态   快充协议  快放协议
uint8_t smalla;                                                     // 小电流状态 及 蓝牙设置

// DS1302
uint16_t year;                               // 年份
uint8_t month, day, hour, minute, sec, week; //  月  日  时  分  秒  星期

// main 当前文件用
uint16_t cycle = 0;    // uint8_t 循环次数     暂用uint16_t预留
uint8_t ble_state = 0; // 蓝牙开关状态

void ThemeSelect()
{
    // 多线程检测按键时候使用，获取蓝牙状态，后续此一个函数即可，，，，需注意 双击小电流与蓝牙小电流判断冲突
    // if (ESP32bt.status() == BLE_STATUS_CONNECTED)
    //     Serial.println("低功耗蓝牙已经开启且有设备已连接");
    // else if (BLE.status() == BLE_STATUS_ADVERTISING)
    //     Serial.println("低功耗蓝牙已开启，正在广播");
    // else if (BLE.status() == BLE_STATUS_OFF)
    //     Serial.println("低功耗蓝牙未开启");
    Serial.println("-----------------------------Loop----------------------");

    bat_v = Battery_V();                      // 电池电压
    sys_v = SYS_V();                          // 输入输出电压
    ic_temp = IC_Temp();                      // ic温度
    ntc_temp = NTC_Temp();                    // 电池温度
    bat_per = Battery_Per();                  // 电池百分比
    bat_m = Battery_Volume() * bat_per / 100; // 电池实时容量  =  总容量x电池百分比
    EE_CycleCount(bat_per);                   // 电池循环次数的判断
    cycle = EEPROM.read(2) / 2;               // 判断之后读取  电池循环次数
    SYS_STATE(&sys_state, &ac_state, &sys_a); // 冲放电状态及输出电流的判断   1A  4C  5AC
    vTaskDelay(10);
    PrintTime(&year, &month, &day, &hour, &minute, &sec, &week); // 从DS1302获取时间数据     年 月 日 时 分 秒 周
    vTaskDelay(10);
    sinkProtocol = Sink_Protocol();     // 充电协议
    sourceProtocol = Source_Protocol(); // 放电协议
    smalla = Small_A_State();           // 小电流状态

    Serial.print("EfuseMac: ");             // chipID  //MAC
    Serial.println(ESP.getEfuseMac(), HEX); // chipID  //MAC

    switch (EEPROM.read(4)) // 读取主题号
    {
    case 1:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle); // 14个
        break;
    case 2:
        Theme2(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 3:
        Theme3(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 4:
        Theme4(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 5:
        Theme5(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 6:
        Theme6(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 7:
        Theme7(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    default:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    }
}

void ThemeSelect_BLEing()
{

    bat_v = Battery_V();                      // 电池电压
    sys_v = SYS_V();                          // 输入输出电压
    ic_temp = IC_Temp();                      // ic温度
    ntc_temp = NTC_Temp();                    // 电池温度
    bat_per = Battery_Per();                  // 电池百分比
    bat_m = Battery_Volume() * bat_per / 100; // 电池实时容量  =  总容量x电池百分比
    EE_CycleCount(bat_per);                   // 电池循环次数的判断
    cycle = EEPROM.read(2) / 2;               // 判断之后读取  电池循环次数
    SYS_STATE(&sys_state, &ac_state, &sys_a); // 冲放电状态及输出电流的判断   1A  4C  5AC
    vTaskDelay(10);
    PrintTime(&year, &month, &day, &hour, &minute, &sec, &week); // 从DS1302获取时间数据     年 月 日 时 分 秒 周
    vTaskDelay(10);
    sinkProtocol = Sink_Protocol();     // 充电协议
    sourceProtocol = Source_Protocol(); // 放电协议
    smalla = Small_A_State();           // 小电流状态

    ble_state = 1;

    switch (EEPROM.read(4))
    {
    case 1:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle); // 14个
        break;
    case 2:
        Theme2(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 3:
        Theme3(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 4:
        Theme4(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 5:
        Theme5(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 6:
        Theme6(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 7:
        Theme7(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    default:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    }
    bleKeyboard.begin(); // 打开蓝牙
    delay(1000);         // 给蓝牙启动缓冲
}

void ThemeSelect_BLE()
{
    bat_v = Battery_V();                      // 电池电压
    sys_v = SYS_V();                          // 输入输出电压
    ic_temp = IC_Temp();                      // ic温度
    ntc_temp = NTC_Temp();                    // 电池温度
    bat_per = Battery_Per();                  // 电池百分比
    bat_m = Battery_Volume() * bat_per / 100; // 电池实时容量  =  总容量x电池百分比
    EE_CycleCount(bat_per);                   // 电池循环次数的判断
    cycle = EEPROM.read(2) / 2;               // 判断之后读取  电池循环次数
    SYS_STATE(&sys_state, &ac_state, &sys_a); // 冲放电状态及输出电流的判断   1A  4C  5AC
    vTaskDelay(10);
    PrintTime(&year, &month, &day, &hour, &minute, &sec, &week); // 从DS1302获取时间数据     年 月 日 时 分 秒 周
    vTaskDelay(10);
    sinkProtocol = Sink_Protocol();     // 充电协议
    sourceProtocol = Source_Protocol(); // 放电协议
    smalla = Small_A_State();           // 小电流状态

    // 打开或关闭小电流
    if ((smalla == 0 && EEPROM.read(8) == 1) || (smalla == 1 && EEPROM.read(8) == 0)) // smalla状态和蓝牙给的设置不一样     注意：eeprom默认255,故不能用 != 判断
        Small_A_ON_or_OFF();                                                          // 写1打开或关闭小电流

    delay(5);
    switch (EEPROM.read(4))
    {
    case 1:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle); // 14个
        break;
    case 2:
        Theme2(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 3:
        Theme3(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 4:
        Theme4(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 5:
        Theme5(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle, year, month, day, hour, minute, sec, week);
        break;
    case 6:
        Theme6(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    case 7:
        Theme7(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    default:
        Theme1(bat_v, sys_v, sys_a, ic_temp, ntc_temp, bat_m, bat_per, sys_state, ac_state, sinkProtocol, sourceProtocol, smalla, ble_state, cycle);
        break;
    }
    DynamicJsonDocument jsonBuffer1(512);
    DynamicJsonDocument jsonBuffer2(512);
    // 整理第 1 次发送数据
    jsonBuffer1["agent"] = agent; // 代理人
    // jsonBuffer1["efuseMac"] = String(ESP.getEfuseMac(), HEX);
    jsonBuffer1["name"] = "AC225";                // 设备名称
    jsonBuffer1["software"] = software;           // 固件版本
    jsonBuffer1["hardware"] = hardware;           // 硬件版本
    jsonBuffer1["ic_temp"] = String(ic_temp, 2);  // ic温度
    jsonBuffer1["bat_ntc"] = String(ntc_temp, 2); // 电池温度
    jsonBuffer1["bat_V"] = String(bat_v, 2);      // 电池电压
    jsonBuffer1["sys_outinv"] = String(sys_v, 2); // 系统电压
    jsonBuffer1["bat_A"] = String(sys_a, 2);      // 充放电流
    // jsonBuffer1["sys_w"] = String(sys_v * sys_a, 2); // 功率         小程序计算，或分段传输保留
    jsonBuffer1["A_C"] = ac_state;           // AC口状态
    jsonBuffer1["sys"] = sys_state;          // 充放电状态
    jsonBuffer1["bat_m"] = String(bat_m, 2); // 电池实时容量
    jsonBuffer1["bat_per"] = bat_per;        // 百分比bat_per
    jsonBuffer1["bat_cir"] = cycle;          // 循环次数
    jsonBuffer1["smalla"] = smalla;          // 小电流状态
    // jsonBuffer1["sinkProtocol"] = sinkProtocol;      // 充电协议       后续小程序更新分段传值
    // jsonBuffer1["sourceProtocol"] = sourceProtocol;  // 放电协议       后续小程序更新分段传值

    String output1;
    serializeJson(jsonBuffer1, output1);
    jsonBuffer1.clear();
    Serial.println("----------------------SendTx: 1");
    Serial.println(output1);
    Serial.println("----------------------");
    bleKeyboard.sendTx(output1); // 第一次发送数据
    output1 = "";
    delay(50);

    // 整理第 2 次发送数据
    // jsonBuffer1["topic_dir"] = EEPROM.read(3);  // 屏幕方向  1  3
    // jsonBuffer1["topic_Num"] = EEPROM.read(4);  // 主题编号
    // jsonBuffer1["led-sleep"] = EEPROM.read(5);  // 亮屏时间  最大存储255   小程序设置值  30  60  90  120  0常亮
    // jsonBuffer1["blt_Time"] = EE_BLETimeRead(); // 蓝牙时间 eeprom: 6  7

    // serializeJson(jsonBuffer1, output1);
    // jsonBuffer1.clear();
    // Serial.println("----------------------SendTx: 2");
    // Serial.println(output1);
    // Serial.println("----------------------");
    // bleKeyboard.sendTx1(output1); // 第二次发送数据
    // output1 = "";
    // delay(50);

    if (Rxdata.length() > 0) // 蓝牙接收数据
    {
        Serial.println("----------------------Rxdata: ");
        Serial.println(Rxdata);
        DeserializationError error = deserializeJson(jsonBuffer2, Rxdata);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: ")); // F: Flash    内容存储到flash  节省RAM空间
            Serial.println(error.f_str());
            return;
        }
        // ble set   蓝牙设置
        uint16_t bletime;                           // 蓝牙时间
        uint8_t lcdtime, theme, topic, idlock, ota; // 亮屏时间    主题    屏幕方向1上3下     OTA    ID锁
        // 解析JSON
        bletime = jsonBuffer2["bttime"];      // 读蓝牙开启状态的时间
        lcdtime = jsonBuffer2["btsleeptime"]; // 亮屏时间
        theme = jsonBuffer2["btthem"];        // 主题
        topic = jsonBuffer2["bttopic"];       // 屏幕方向（1上3下）	屏幕方向
        idlock = jsonBuffer2["btidlock"];     // ID锁
        ota = jsonBuffer2["btota"];           // OTA更新
        // 以下数据类型共用uint定义
        year = jsonBuffer2["btyear"];     // 年
        month = jsonBuffer2["btmon"];     // 月
        day = jsonBuffer2["btday"];       // 日
        hour = jsonBuffer2["bthour"];     // 时
        minute = jsonBuffer2["btmin"];    // 分
        sec = jsonBuffer2["btsec"];       // 秒
        week = jsonBuffer2["btweek"];     // 周
        smalla = jsonBuffer2["btsmalla"]; // 读小电流开关设置
        cycle = jsonBuffer2["btcycle"];   // 改写循环次数

        // 开始写入数据
        if (bletime != 0)
            EE_BLETimeWrite(bletime); // 写入蓝牙时间
        EEPROM.write(5, lcdtime);     // 亮屏时间    (0 / 1)
        if (theme != 0)
            EEPROM.write(4, theme); // 写入主题编号
        if (topic != 0)
            EEPROM.write(3, topic); // 写入屏幕显示方向
        if (idlock != 0)
            EEPROM.write(12, idlock); // 写1 关闭所有输出口(丢失模式)
        if (ota != 0)
            EEPROM.write(11, ota); // OTA更新  写1更新  更新处自动置零

        if (sec != 0 || minute != 0 || hour != 0 || day != 0 || month != 0 || year || week != 0)
            SetRTCtime(year, month, day, hour, minute, sec + 1, week); // 更新彩屏时间
        EEPROM.write(8, smalla);                                       // 写入小电流设置    (0 / 1)
        if (cycle == 1)
        {
            EEPROM.write(0, 0); // 循环判断一并清零
            EEPROM.write(1, 0); //
            EEPROM.write(2, 0); // 改写循环次数  写1清零
        }
        else if (cycle != 0)
            EEPROM.write(2, cycle); // 改写循环次数   (实际值 = 写入的值 / 2)
        EEPROM.commit();            // 保存
        vTaskDelay(10);
        Rxdata = ""; // 清空
        Serial.println("----------------------");
    }
}

void Button_DOWN()
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