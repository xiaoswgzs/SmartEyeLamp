#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_def.h"
#include "vl53l0x_api.h"  // 加这个！解决 VL53L0X_DEV 未定义
#include "MyI2C.h"
#include "Delay.h"

// ==============================================================
// 1. IIC 写多个字节
// ==============================================================
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    MyI2C_Start();
    MyI2C_SendByte(Dev->I2cDevAddr); // 默认是 0x52
    MyI2C_ReceiveAck();
    MyI2C_SendByte(index);           // 寄存器地址
    MyI2C_ReceiveAck();
    
    for(uint32_t i = 0; i < count; i++)
    {
        MyI2C_SendByte(pdata[i]);    // 连续发数据
        MyI2C_ReceiveAck();
    }
    MyI2C_Stop();
    return VL53L0X_ERROR_NONE;
}

// ==============================================================
// 2. IIC 读多个字节
// ==============================================================
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    MyI2C_Start();
    MyI2C_SendByte(Dev->I2cDevAddr);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(index);
    MyI2C_ReceiveAck();
    
    MyI2C_Start(); // 重启总线
    MyI2C_SendByte(Dev->I2cDevAddr | 0x01); // 变成读指令 0x53
    MyI2C_ReceiveAck();
    
    for(uint32_t i = 0; i < count; i++)
    {
        pdata[i] = MyI2C_ReceiveByte();
        if(i == count - 1)
            MyI2C_SendAck(1); // 最后一个字节给非应答 (1)
        else
            MyI2C_SendAck(0); // 前面的字节给应答 (0)
    }
    MyI2C_Stop();
    return VL53L0X_ERROR_NONE;
}

// ==============================================================
// 3. IIC 读写单个 字节/字/双字 (直接套用上面的 Multi 即可)
// ==============================================================
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    return VL53L0X_WriteMulti(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    return VL53L0X_ReadMulti(Dev, index, data, 1);
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    return VL53L0X_WriteMulti(Dev, index, buf, 2);
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    uint8_t buf[2];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buf, 2);
    *data = ((uint16_t)buf[0] << 8) | buf[1];
    return status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buf[4];
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8)  & 0xFF;
    buf[3] = (data & 0xFF);
    return VL53L0X_WriteMulti(Dev, index, buf, 4);
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    uint8_t buf[4];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buf, 4);
    *data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    return status;
}

// ==============================================================
// 4. 提供官方需要的延时接口
// ==============================================================
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    Delay_ms(2);
    return VL53L0X_ERROR_NONE;
}

// 新增：官方API需要的接口
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    uint8_t data;

    // 读一个字节
    status = VL53L0X_RdByte(Dev, index, &data);
    if (status != VL53L0X_ERROR_NONE)
        return status;

    // 与运算 + 或运算
    data = (data & AndData) | OrData;

    // 写回去
    status = VL53L0X_WrByte(Dev, index, data);

    return status;
}
