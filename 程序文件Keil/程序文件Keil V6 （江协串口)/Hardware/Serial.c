#include "stm32f10x.h"                  // 设备头文件，包含STM32F10x系列MCU的寄存器定义
#include "stdio.h"                      // 标准输入输出头文件，用于printf等函数
#include "stdarg.h"                     // 可变参数头文件，用于处理可变参数函数

// 全局变量声明
uint8_t Serial_RXFlag;                  // 串口接收完成标志位，1表示接收到完整数据包
char Serial_RXPacket[100];              // 串口接收数据包缓冲区，100字节

/**
  * @brief  串口初始化函数
  * @param  无
  * @retval 无
  * @note   初始化USART1，配置为9600波特率，8位数据位，1位停止位，无校验位
  *         使用GPIOA的Pin9(TX)和Pin10(RX)，开启接收中断
  */
void Serial_Init(void)
{
    // 开启USART1和GPIOA的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置GPIOA Pin9为复用推挽输出（USART1_TX）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;      // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;            // PA9引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    // 50MHz速度
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置GPIOA Pin10为上拉输入（USART1_RX）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;        // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;           // PA10引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    // 50MHz速度
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART1参数
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;                     // 波特率9600
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;    // 8位数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;         // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;            // 无校验位
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 使能发送和接收
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控制
    USART_Init(USART1, &USART_InitStructure);
    
    // 开启USART1的接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);     // 接收缓冲区非空中断
    
    // 配置中断优先级分组（分组2：2位抢占优先级，2位子优先级）
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    // 配置USART1中断通道
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;              // USART1中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                // 使能中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;      // 抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;             // 子优先级1
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}

/**
  * @brief  串口发送单个字节
  * @param  Byte: 要发送的字节
  * @retval 无
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);                      // 将数据写入发送数据寄存器
    // 等待发送完成（发送数据寄存器空标志位）
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/**
  * @brief  串口发送字节数组
  * @param  Array: 要发送的数组指针
  * @param  Length: 数组长度
  * @retval 无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Array[i]);                     // 逐个发送字节
    }
}

/**
  * @brief  串口发送字符串
  * @param  String: 要发送的字符串指针（以'\0'结尾）
  * @retval 无
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)                // 遍历字符串直到结束符
    {
        Serial_SendByte(String[i]);                    // 逐个发送字符
    }
}

/**
  * @brief  计算幂函数
  * @param  X: 底数
  * @param  Y: 指数
  * @retval X的Y次幂
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;                                   // 累乘计算幂
    }
    return Result;
}

/**
  * @brief  串口发送数字
  * @param  Number: 要发送的数字
  * @param  Length: 数字的总位数（不足位会补0）
  * @retval 无
  * @note   例如：Serial_SendNumber(123, 5) 会发送"00123"
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        // 计算每一位数字并转换为ASCII码发送
        Serial_SendByte(Number / Serial_Pow(10, Length - 1 - i) % 10 + '0');
    }
}

/**
  * @brief  printf函数的重定向输出函数
  * @param  ch: 要输出的字符
  * @param  f: 文件指针（标准库内部使用）
  * @retval 输出的字符
  * @note   此函数将printf输出重定向到串口，使printf直接输出到串口
  */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);                               // 将字符通过串口发送
    return ch;                                         // 返回字符，符合标准库要求
}

/**
  * @brief  串口格式化输出函数（类似printf）
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval 无
  * @note   支持标准printf格式化功能，通过串口输出格式化后的字符串
  */
void Serial_Printf(char *format, ...)
{
    char String[100];                                  // 临时字符串缓冲区
    va_list arg;                                       // 可变参数列表
    va_start(arg, format);                             // 初始化可变参数
    vsprintf(String, format, arg);                     // 格式化字符串到缓冲区
    va_end(arg);                                       // 结束可变参数
    Serial_SendString(String);                         // 发送格式化后的字符串
}

/**
  * @brief  获取串口接收完成标志
  * @param  无
  * @retval 1: 接收到新数据包（调用后标志位自动清零）
  *         0: 未接收到新数据包
  * @note   需要在主循环中周期性调用此函数检查接收状态
  */
uint8_t Serial_GetRxFlag(void)
{
    if (Serial_RXFlag == 1)                            // 检查接收标志
    {
        Serial_RXFlag = 0;                             // 清零标志位
        return 1;                                      // 返回接收成功
    }
    return 0;                                          // 返回未接收
}

/**
  * @brief  USART1中断服务函数
  * @param  无
  * @retval 无
  * @note   使用状态机解析接收数据包，数据包格式：@ + 数据 + \r + \n
  *         状态0：等待起始符'@'
  *         状态1：接收数据内容，直到遇到'\r'
  *         状态2：等待结束符'\n'
  *         **注意**：此实现存在缓冲区溢出风险，当接收数据超过99字节时会发生溢出
  */
void USART1_IRQHandler(void)
{
    static uint8_t RXState = 0;                        // 接收状态机状态
    static uint8_t pRXPacket = 0;                      // 接收缓冲区索引
    
    // 检查是否是接收缓冲区非空中断
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t RXData = USART_ReceiveData(USART1);    // 读取接收到的数据
        
        // 状态机处理接收数据
        if (RXState == 0)                              // 状态0：等待数据包起始标志
        {
            // 只有当当前没有未处理的数据包时才接收新数据包
            if (RXData == '@' && Serial_RXFlag == 0)   // 接收到起始标志'@'
            {
                RXState = 1;                           // 进入状态1：接收数据
                pRXPacket = 0;                         // 重置缓冲区索引
            }
        }
        else if (RXState == 1)                         // 状态1：接收数据内容
        {
            if (RXData == '\r')                        // 接收到回车符'\r'
            {
                RXState = 2;                           // 进入状态2：等待换行符
            }
            else                                       // 接收到普通数据
            {
                // **风险点**：此处没有检查索引是否越界，当数据超过99字节时会溢出
                Serial_RXPacket[pRXPacket] = RXData;   // 存储接收到的数据
                pRXPacket++;                           // 索引递增
            }
        }
        else if (RXState == 2)                         // 状态2：等待数据包结束标志
        {
            if (RXData == '\n')                        // 接收到换行符'\n'
            {
                Serial_RXPacket[pRXPacket] = '\0';     // 添加字符串结束符
                Serial_RXFlag = 1;                     // 设置接收完成标志
                RXState = 0;                           // 重置状态机，等待下一个数据包
            }
        }
        
        // 清除接收中断标志位
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// 建议添加以下函数以增强代码健壮性：

/**
  * @brief  获取接收到的数据长度
  * @param  无
  * @retval 接收到的数据长度（不包括'\0'）
  */
uint8_t Serial_GetRxLength(void)
{
    uint8_t length = 0;
    while (Serial_RXPacket[length] != '\0' && length < 100)
    {
        length++;
    }
    return length;
}

/**
  * @brief  重置串口接收状态
  * @param  无
  * @retval 无
  * @note   清空接收缓冲区和标志位，用于异常处理
  */
void Serial_ResetReceiver(void)
{
    Serial_RXFlag = 0;
    Serial_RXPacket[0] = '\0';
}
