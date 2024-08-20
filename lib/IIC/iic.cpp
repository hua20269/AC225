#include <iic.h>

void IICinit()
{
    Wire.begin(SDA_PIN, SCL_PIN);
}

/**
 * @brief IIC数据写入
 *
 * @param mcuAddr 设备地址
 * @param regAddr 寄存器地址
 * @param senddate 发送的数据
 * @return uint8_t  通讯成功：0 其他通讯失败
 */
uint8_t I2C_Write(uint8_t mcuAddr, uint8_t regAddr, uint8_t senddate)
{
    /*  Write Device Address */
    Wire.beginTransmission(mcuAddr);
    /*  Write Subaddresses */
    Wire.write(regAddr);
    /*  Write Databytes */
    Wire.write(senddate);
    /* 完成一次IIC通讯 默认发送一个停止位*/
    return Wire.endTransmission();
    delay(10);
}

/**
 * @brief IIC数据读取
 *
 * @param mcuAddr 设备地址
 * @param regAddr 寄存器地址
 * @param getdate 读取到的数据
 * @return uint8_t 读取成功：0 其他：读取失败
 */
uint8_t I2C_Read(uint8_t mcuAddr, uint8_t regAddr)
{
    uint8_t t = 200;
    uint8_t ret = 0;
    uint8_t getdate;
    Wire.beginTransmission(mcuAddr); // 设备地址 写入
    Wire.write(regAddr);             // 寄存器地址 写入
    ret = Wire.endTransmission(false);
    if (ret == 0) // 判断是否成功传输
    {
        Wire.requestFrom(mcuAddr, (size_t)1, (bool)1); // 请求数据
        /* 5. 读出AT24C02返回的值，成功读取后写入缓存变量处，读取失败返回失败码 */
        while (!Wire.available()) // 读取数据非一个字节遍历
        {
            t--;
            delay(1);
            if (t == 0)
                return 1;
        }
        getdate = Wire.read(); // 读取数据
        return getdate;
    }
    else
    {
        // 根据不同的错误代码进行处理
        Serial.print("Error: Transmission failed with code ");
        Serial.println(ret);
        return 0;
    }
}
