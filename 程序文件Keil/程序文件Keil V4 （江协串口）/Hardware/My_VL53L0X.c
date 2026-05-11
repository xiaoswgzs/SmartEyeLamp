#include "My_VL53L0X.h"
#include "MyI2C.h"
#include "vl53l0x_api.h"

// 声明一个全局的设备结构体
VL53L0X_Dev_t MySensor;
VL53L0X_DEV   Dev = &MySensor;

void MyVL53L0X_Init(void)
{
    uint8_t VhvSettings;
    uint8_t PhaseCal;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;

    // 1. 初始化底层软件IIC引脚
    MyI2C_Init();
    
    // 2. 配置传感器基础信息 (器件地址默认0x52)
    Dev->I2cDevAddr = 0x52;
    Dev->comms_type = 1;
    Dev->comms_speed_khz = 400;

    // 3. 按照官方标准流程一顿猛如虎的初始化 (全靠官方API)
    VL53L0X_DataInit(Dev);
    VL53L0X_StaticInit(Dev);
    
    // 核心校准：温度/相位校准
    VL53L0X_PerformRefCalibration(Dev, &VhvSettings, &PhaseCal);
    
    // 核心校准：SPAD 阵列校准
    VL53L0X_PerformRefSpadManagement(Dev, &refSpadCount, &isApertureSpads);
    
    // 设置为连续测距模式
    VL53L0X_SetDeviceMode(Dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    
    // 正式启动测量！
    VL53L0X_StartMeasurement(Dev);
}

// 读取距离 (单位: 毫米) 【修复版：加超时，防止卡死重启】
uint16_t MyVL53L0X_GetDistance(void)
{
    VL53L0X_RangingMeasurementData_t RangingData;
    uint8_t dataReady = 0;
    uint32_t timeout = 100000; // 超时计数器（关键！）
    
    // 等待数据就绪，加超时退出，不会死循环
    while (dataReady == 0 && timeout-- > 0) 
    {
        VL53L0X_GetMeasurementDataReady(Dev, &dataReady);
    }

    // 超时处理：返回无效值，不卡死
    if(timeout == 0)
    {
        return 8190; // 超量程标识
    }
    
    // 获取数据
    VL53L0X_GetRangingMeasurementData(Dev, &RangingData);
    
    // 清除中断
    VL53L0X_ClearInterruptMask(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
    
    return RangingData.RangeMilliMeter;
}
