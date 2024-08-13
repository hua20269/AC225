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
#include "AgentConfig.h"

#include "ThemeConfig.h"

// #include "soc/rtc_wdt.h" // 设置rtc看门狗用

#define BUTTON_PIN_BITMASK 0x0010 // GPIOs 4    io4 按钮

// BleKeyboard bleKeyboard("AC225", "OCRC", 50); // 蓝牙

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

    IICinit();          // 初始化 IIC 通讯
    EEPROMinit();       // 初始化 EEPROM 寄存器
    DisplayInit();      // 显示初始化
    PowerLOGO(imgName); // 开机LOGO 

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
        EEPROM.write(5, 30);                // 点亮时间改为30s
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
    Serial.printf("setup on core: ");
    Serial.println(xPortGetCoreID());
}

void loop()
{
    Serial.printf("loop on core: ");
    Serial.println(xPortGetCoreID());
    uint8_t currentTime = EEPROM.read(5); // LCDTime时间
    unsigned long currentTime1;           // 程序执行计时
    while (currentTime >= 0)              // 亮屏时间 转次数
    {

    beijing0:
        ThemeSelect(); // 未开启蓝牙的  主题选择  // ThemeConfig.h

        if (currentTime != 0)
            currentTime--; // 睡眠时间倒计时(循环次数，大概1s/fps)   到0退出循环
        if (currentTime == 1)
            break; // 跳出大循环  睡眠
        Serial.print("LCDTime: ");
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
                            ticker1.once(EE_BLETimeRead(), esp_deep_sleep_start); // 小程序改的蓝牙打开时间  (150-3600,执行函数)

                            ThemeSelect_BLEing(); // 开启蓝牙时  主题选择  // ThemeConfig.h
                            // AC_OFF();

                            while (1)
                            {
                                LcdRotation(); // 实时屏幕上下

                                ThemeSelect_BLE(); // 蓝牙循环

                                Serial.print("Topic: ");
                                Serial.println(EEPROM.read(3));
                                Serial.print("Theme: ");
                                Serial.println(EEPROM.read(4));
                                Serial.print("LCDTime: ");
                                Serial.println(EEPROM.read(5));
                                Serial.print("BleTime: ");
                                Serial.println(EE_BLETimeRead());
                                Serial.print("Smalla: ");
                                Serial.println(EEPROM.read(8));
                                Serial.print("OTA: ");
                                Serial.println(EEPROM.read(11));
                                Serial.print("IDLock: ");
                                Serial.println(EEPROM.read(12));
                                Serial.print("Cycle: ");
                                Serial.print(EEPROM.read(2));
                                Serial.println("/2");

                                unsigned long currentTime2;
                                currentTime2 = millis(); // 程序执行到此时间
                                delay(10);
                                while (millis() - currentTime2 < 1000) // while延时
                                {
                                    Button_DOWN(); // 按键拍照
                                }
                            }
                        }
                    }
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
        vTaskDelay(600); // 慢一点循环，让OTA跑流畅点   //延时 退让资源
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
        vTaskDelay(1000);                 // 延时 退让资源
    }
}
// 看门狗，防死机
void Task_RTC(void *pvParameters)
{
    while (1)
    {
        Serial.print("Task_RTC on core: ");
        Serial.println(xPortGetCoreID()); // 所在核心
        vTaskDelay(2000);                 // 延时 退让资源
    }
}
