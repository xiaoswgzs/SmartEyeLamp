#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stm32f10x.h"

typedef enum
{
    PROTOCOL_CMD_NONE = 0,
    PROTOCOL_CMD_PING,
    PROTOCOL_CMD_GET_STATUS,
    PROTOCOL_CMD_MODE_AUTO,
    PROTOCOL_CMD_MODE_MANUAL,
    PROTOCOL_CMD_MODE_TOGGLE,
    PROTOCOL_CMD_LED_ON,
    PROTOCOL_CMD_LED_OFF,
    PROTOCOL_CMD_BRIGHT_UP,
    PROTOCOL_CMD_BRIGHT_DOWN,
    PROTOCOL_CMD_BRIGHT_SET,
    PROTOCOL_CMD_COUNT_GET,
    PROTOCOL_CMD_COUNT_RESET,
    PROTOCOL_CMD_UNKNOWN
} ProtocolCommand_t;

typedef struct
{
    ProtocolCommand_t command;
    int16_t value;
} ProtocolPacket_t;

typedef struct
{
    uint8_t modeAuto;
    uint8_t lightOn;
    uint8_t brightness;
    uint8_t target;
    uint8_t human;
    uint32_t humanTriggerCount;
} ProtocolStatus_t;

uint8_t Protocol_FetchCommand(ProtocolPacket_t *packet);

void Protocol_SendOK(const char *cmd);
void Protocol_SendError(const char *code);
void Protocol_SendPong(void);
void Protocol_SendStatus(const ProtocolStatus_t *status);
void Protocol_SendMode(uint8_t modeAuto);
void Protocol_SendLight(uint8_t lightOn);
void Protocol_SendHuman(uint8_t human, uint32_t count);
void Protocol_SendBrightness(uint8_t brightness, uint8_t target);
void Protocol_SendCount(uint32_t count);
void Protocol_SendGesture(const char *name);
void Protocol_SendDim(const char *state);

#endif
