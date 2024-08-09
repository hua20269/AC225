#include <SW6208.h>

/**
 * @brief sw的adc数据
 * 0x13是adc高八位数据
 * 0x14是adc低四位数据
 * @return uint16_t 返回的adc值  在其他函数中进行处理
 */
uint16_t ADC_Data()
{
    // uint8_t r_buffer[2];
    // int ADCvalue;
    // r_buffer[0] = I2C_Read(SW6208_address, 0x13);
    // Serial.println("ADC: ");
    // Serial.println(r_buffer[0], HEX);
    // delay(10);
    // r_buffer[1] = I2C_Read(SW6208_address, 0x14);
    // Serial.println(r_buffer[1], HEX);
    // ADCvalue = (r_buffer[0] << 4) + r_buffer[1];
    // Serial.println(ADCvalue);
    // return ADCvalue;

    uint16_t adc_value = (I2C_Read(SW6208_address, 0x13) << 4) + I2C_Read(SW6208_address, 0x14); // <<  >>  此移位符号干扰运算符，移位要加括号

    // Serial.print("ADC_Data: ");
    // Serial.println(adc_value);
    return adc_value;
}

/**
 * @brief 电池电压的读取和计算  电压=adc*0.0012v
 */
float Battery_V()
{
    I2C_Write(SW6208_address, 0x12, 0x0);
    Serial.print("Battery_V: ");
    Serial.println(ADC_Data() * 0.0012);
    return ADC_Data() * 0.0012;
}

/**
 * @brief 输入输出电压获取
 *
 * @return float 电压
 */
float SYS_V()
{
    I2C_Write(SW6208_address, 0x12, 0x1);
    Serial.print("SYS_V: ");
    Serial.println(ADC_Data() * 0.004);
    return ADC_Data() * 0.004;
}

/**
 * @brief ic温度获取
 *
 * @return float ic温度
 */
float IC_Temp()
{
    I2C_Write(SW6208_address, 0x12, 0x2);
    Serial.print("IC_Temp: ");
    Serial.println((ADC_Data() - 1839) / 6.82);
    return (ADC_Data() - 1839) / 6.82;
}

/**
 * @brief ntc电阻转换温度
 *
 * @return float ntc温度
 */
float NTC_Temp()
{
    uint8_t r_buffer;
    float temp;
    r_buffer = I2C_Read(SW6208_address, 0x48);
    r_buffer = r_buffer & 0x1;
    I2C_Write(SW6208_address, 0x12, 0x3);

    if (r_buffer == 0)
        temp = ADC_Data() * 0.0022 / 0.00008;
    if (r_buffer == 1)
        temp = ADC_Data() * 0.0011 / 0.00004;

    temp = log(temp / R25); // ln(Rt/Rp)：Rt:当前温度下的阻值 R25:25℃下的NTC阻值(K)
    temp /= BX;             // ln(Rt/Rp)/BX 注：BX值一般为NTC的规格，本次用的NTC型号是3950
    temp += (1 / T25);
    temp = 1 / temp;
    temp -= 273.15;
    temp += 0.5;
    Serial.print("NTC_Temp: ");
    Serial.println(temp, 4);
    return temp;
}

/**
 * @brief 充电电流获取
 *
 * @return float 返回充电电流值
 */

float SYS_inA()
{
    I2C_Write(SW6208_address, 0x12, 0x4);

    float a_value;
    a_value = ADC_Data() * 0.002273;

    Serial.print("SYS_inA: ");
    Serial.println(a_value);
    // return a_value;
    return a_value * 1.1;
}

/**
 * @brief 输出电流的获取
 *
 * @return float 返回输出电流  小数形式
 */
float SYS_outA()
{
    I2C_Write(SW6208_address, 0x12, 0x5);

    float a_value;
    a_value = ADC_Data() * 0.002273;

    Serial.print("SYS_outA: ");
    Serial.println(a_value);
    // return a_value;
    return a_value * 1.1;
}

/**
 * @brief 电量获取
 *
 * @return float 返回电量值  小数形式
 */
float Battery_Volume()
{
    float a_value;

    // uint8_t r_buffer[2];
    // int ADCvalue;
    // r_buffer[0] = I2C_Read(SW6208_address, 0x73);
    // r_buffer[1] = I2C_Read(SW6208_address, 0x74);
    // ADCvalue = (r_buffer[1] << 8) + r_buffer[0];
    // a_value = ADCvalue * 0.1695;

    a_value = ((I2C_Read(SW6208_address, 0x74) << 8) + I2C_Read(SW6208_address, 0x73)) * 0.1695; // <<  >>  此移位符号干扰运算符，移位要加括号

    Serial.print("Battery_Volume: ");
    Serial.println(a_value);
    return a_value;
}

/**
 * @brief 电量百分比获取
 *
 * @return uint16_t 返回电量百分比数值
 */
uint8_t Battery_Per()
{
    Serial.print("Battery_Per: ");
    Serial.println(I2C_Read(SW6208_address, 0x7E));

    return I2C_Read(SW6208_address, 0x7E);

    // return 100;
}

/**
 * @brief 系统充放电状态 AC口状态
 *
 * @param sys_state  系统状态 2充电 1 放电
 * @param A_C   1 A口状态  4C口状态    5 AC
 * @param sys_a 充电或者输出电流
 *
 */
void SYS_STATE(uint8_t *sys_state, uint8_t *ac_state, float *sys_a)
{
    uint8_t svalue;
    svalue = I2C_Read(SW6208_address, 0x0C);
    *sys_state = svalue >> 6;
    Serial.print("*sys_state: ");
    Serial.println(*sys_state);

    *ac_state = svalue & 0xF;
    Serial.print("*ac_state: ");
    Serial.println(*ac_state);

    if (*sys_state == 1) // 放电
        *sys_a = SYS_outA();
    else if (*sys_state == 2) // 充电
        *sys_a = SYS_inA();
    else
        *sys_a = SYS_outA();
}

/**
 * @brief REG 0x0F: 快充协议指示
 * bit:7 PD版本指示 0: PD2.0 1: PD3.0   注意此指示只在PD沟通后有效
 * bit:6-4 充电sink快充协议指示 0: 非快充  * 1: PD sink 2: / 3: HV sink 4: AFC sink 5: FCP sink 6: SCP sink 7: PE1.1 sink
 * bit:3-0 放电source快充协议指示 0: 非快充 1: PD source 2: PPS source 3: QC2.0 source 4: QC3.0 source 5: FCP source 6: PE2.0 /1.1 source 7: SFCP source 8: AFC source 9: SCP source 10-15: reserved
 * @return uint8_t
 */
uint8_t Sink_Protocol() // 充电协议
{
    // Serial.print("Sink_Protocol------------------------------------");
    // Serial.println(I2C_Read(SW6208_address, 0x0F) >> 4 & 0x07);
    if (I2C_Read(SW6208_address, 0x0F) >> 4 & 0x07 == 1 && I2C_Read(SW6208_address, 0x0F) >> 7 & 0x1 == 1) // PD3.0
        return 2;
    else
        return I2C_Read(SW6208_address, 0x0F) >> 4 & 0x07;
}
uint8_t Source_Protocol() // 放电协议
{
    // Serial.print("Source_Protocol------------------------------------");
    // Serial.println(I2C_Read(SW6208_address, 0x0F) & 0x0F);
    if (I2C_Read(SW6208_address, 0x0F) & 0x0F == 1 && I2C_Read(SW6208_address, 0x0F) >> 7 & 0x1 == 1) // PD3.0
        return 10;
    else
        return I2C_Read(SW6208_address, 0x0F) & 0x0F;
}

/**
 * @brief 小电流状态
 * @return uint8_t   1/0
 */
uint8_t Small_A_State()
{
    Serial.print("Small_A_State: ");
    Serial.println(I2C_Read(SW6208_address, 0x2E) & 0X1);

    return I2C_Read(SW6208_address, 0x2E) & 0X1;
}
/**
 * @brief 开启或退出小电流  bit:4   0X10   处于非小电流模式时,写1后进入小电流模式;  处于小电流模式时,写1后退出小电流模式
 * @brief 本设备没有A2物理接口，给小电流开关用    打开口才能开启小电流
 */
void Small_A_ON_or_OFF()
{
    I2C_Write(SW6208_address, 0x19, 0X4);  // A2口触发插入事件
    I2C_Write(SW6208_address, 0x2E, 0X10); // 写1开启或退出小电流
    I2C_Write(SW6208_address, 0x19, 0X8);  // A2口触发拔出事件
}

// 关闭所有输出口
void AC_OFF()
{
    I2C_Write(SW6208_address, 0x18, 0X10) == 0;
    Serial.println("--------------AC_OFF-----------");
}
/**

 * -------------------------------------------------------------------------以下为setup配置----初始化------------
 * -------------------------------------------------------------------------以下为setup配置----初始化------------
 * -------------------------------------------------------------------------以下为setup配置----初始化------------
 * -------------------------------------------------------------------------以下为setup配置----初始化------------
 */

/**
 * @param 1.按键作用  2.轻载电流和时间设置  3.MCU小电流使能
 * @param 2.设置NTC温度上限保护50℃   改为 60℃
 * @param 3.打开12V输入
 */
void SW6208init()
{
    if (I2C_Read(SW6208_address, 0x03) != 0x7F || I2C_Read(SW6208_address, 0x30) != 0x54 || I2C_Read(SW6208_address, 0x33) != 0x3)
    {
        I2C_Write(SW6208_address, 0x03, 0x7F); // 0000 1111  // 1: 双击开启小电流(无单击动画使用)(0x7F), 有动画用(0xBF)(去按键作用)        3: 仅显示电量
        I2C_Write(SW6208_address, 0x30, 0x54); // 0100  // 轻载检测电流设置 VOUT<7.65V 或者 VOUT>7.65V 且 reg0x30[0]=0:   此设置为:25mA    轻载时间设置为8s
        I2C_Write(SW6208_address, 0x33, 0x3);  // 0011  // MCU 蓝牙小电流使能
    }
    delay(200);
    if (I2C_Read(SW6208_address, 0x47) != 0X0 || I2C_Read(SW6208_address, 0x48) != 0XE0)
    {
        I2C_Write(SW6208_address, 0x47, 0X0);  // Boost NTC温度自适应功能使能   使能    高于自适应温度门限后，ntc温度每上升1度，vout下降800mv   (温度达不到，自适应阈值100°)基本没用
        I2C_Write(SW6208_address, 0x48, 0XE0); // Charger NTC高温保护门限 60°       0xC0  55°    默认50°
    }
    // delay(200);
    // if (I2C_Read(SW6208_address, 0x1A) != 0X18)
    // {
    //     I2C_Write(SW6208_address, 0x1A, 0X18); // 打开12V输入
    // }
}
