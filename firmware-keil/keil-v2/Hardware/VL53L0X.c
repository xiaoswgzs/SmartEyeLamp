#include "stm32f10x.h"
#include "MyI2C.h"
#include "Delay.h"

#define VL53L0X_ADDRESS  0x52  // 传感器的IIC设备地址

// ------------------------------------------------------------------
// 核心桥梁：向 VL53L0X 的某个寄存器写入 1 个字节
// ------------------------------------------------------------------
void VL53L0X_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    MyI2C_Start();
    MyI2C_SendByte(VL53L0X_ADDRESS);       // 发送设备写地址
    MyI2C_ReceiveAck();                    // (省略了应答判断，默认传感器在线)
    MyI2C_SendByte(RegAddress);            // 发送要写入的寄存器地址
    MyI2C_ReceiveAck();
    MyI2C_SendByte(Data);                  // 发送数据
    MyI2C_ReceiveAck();
    MyI2C_Stop();
}

// ------------------------------------------------------------------
// 核心桥梁：从 VL53L0X 的某个寄存器读取 1 个字节
// ------------------------------------------------------------------
uint8_t VL53L0X_ReadReg(uint8_t RegAddress)
{
    uint8_t Data;
    MyI2C_Start();
    MyI2C_SendByte(VL53L0X_ADDRESS);       // 发送设备写地址
    MyI2C_ReceiveAck();
    MyI2C_SendByte(RegAddress);            // 发送要读取的寄存器地址
    MyI2C_ReceiveAck();
    
    MyI2C_Start();                         // 重新启动总线
    MyI2C_SendByte(VL53L0X_ADDRESS | 0x01);// 发送设备读地址 (0x52 | 0x01 = 0x53)
    MyI2C_ReceiveAck();
    Data = MyI2C_ReceiveByte();            // 读取数据
    MyI2C_SendAck(1);                      // 产生非应答，告诉它不读了
    MyI2C_Stop();
    
    return Data;
}

// ------------------------------------------------------------------
// 极简版初始化：唤醒传感器，配置为默认的高精度单次测距模式
// ------------------------------------------------------------------
void VL53L0X_Init(void)
{
    MyI2C_Init(); // 初始化底层IIC引脚
    Delay_ms(50); // 等待传感器上电稳定
    
    // 检查传感器是否在线 (读取它的设备ID寄存器 0xC0，正常应该返回 0xEE)
    uint8_t id = VL53L0X_ReadReg(0xC0);
    if(id != 0xEE)
    {
        // 如果卡在这里，说明线接错了，或者IIC通信失败！
        // 可以加个 OLED_ShowString 报错提示
        while(1); 
    }

    // ====== 以下是开源社区逆向出的“魔法初始化序列” ======
    VL53L0X_WriteReg(0x88, 0x00);
    VL53L0X_WriteReg(0x80, 0x01);
    VL53L0X_WriteReg(0xFF, 0x01);
    VL53L0X_WriteReg(0x00, 0x00);
    VL53L0X_ReadReg(0x91);
    VL53L0X_WriteReg(0x00, 0x01);
    VL53L0X_WriteReg(0xFF, 0x00);
    VL53L0X_WriteReg(0x80, 0x00);
    
    // 配置测距参数 (采用默认范围)
    VL53L0X_WriteReg(0x01, 0xFF);
    
    // 开启连续单次测量模式 (Continuous mode)
    VL53L0X_WriteReg(0x00, 0x02); 
    // =======================================================
}

// ------------------------------------------------------------------
// 获取距离数据 (单位：毫米 mm)
// ------------------------------------------------------------------
uint16_t VL53L0X_ReadDistance(void)
{
    uint16_t distance = 0;
    uint8_t data_High, data_Low;
    
    // 轮询等待数据准备好 (寄存器 0x13 的最低位变 1)
    // 注意：如果在实际工程里怕卡死，这里应该加个超时退出机制
    while ((VL53L0X_ReadReg(0x13) & 0x07) == 0)
    {
        Delay_ms(1);
    }
    
    // 数据准备好了，读取高8位和低8位 (距离数据存在 0x1E 和 0x1F)
    data_High = VL53L0X_ReadReg(0x1E); // 读取高8位
    data_Low  = VL53L0X_ReadReg(0x1F); // 读取低8位
    
    // 拼接成 16 位的真实距离
    distance = (data_High << 8) | data_Low;
    
    // 清除中断标志，告诉传感器“我读完了，你可以测下一次了”
    VL53L0X_WriteReg(0x0B, 0x01);
    
    // 【防错误数据处理】：如果测不到东西(对着天空)，它会返回 8190 或 20 这种乱码
    if (distance > 2000 || distance < 20) 
    {
        distance = 8888; // 自定义一个错误码，代表超出量程
    }
    
    return distance; // 返回毫米距离！
}