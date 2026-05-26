#include "Protocol.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Lamp serial protocol
 *
 * RX: @COMMAND\r\n
 * TX: #TYPE,K=V\r\n
 *
 * Keep the payload short. This runs over a BLE UART transparent module,
 * not a large network stack.
 */

extern uint8_t Serial_RXFlag;
extern char Serial_RXPacket[100];
void Serial_SendString(char *String);
void Serial_Printf(char *format, ...);

static uint8_t Protocol_Equals(const char *a, const char *b)
{
    return strcmp(a, b) == 0 ? 1 : 0;
}

static uint8_t Protocol_StartsWith(const char *s, const char *prefix)
{
    while (*prefix != '\0')
    {
        if (*s != *prefix)
        {
            return 0;
        }
        s++;
        prefix++;
    }
    return 1;
}

static int16_t Protocol_ClampPercent(int16_t value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 100)
    {
        return 100;
    }
    return value;
}

uint8_t Protocol_FetchCommand(ProtocolPacket_t *packet)
{
    char *cmd;

    if (packet == 0)
    {
        return 0;
    }

    packet->command = PROTOCOL_CMD_NONE;
    packet->value = 0;

    if (Serial_RXFlag == 0)
    {
        return 0;
    }

    cmd = Serial_RXPacket;

    if (Protocol_Equals(cmd, "PING"))
    {
        packet->command = PROTOCOL_CMD_PING;
    }
    else if (Protocol_Equals(cmd, "GET") || Protocol_Equals(cmd, "STATUS?"))
    {
        packet->command = PROTOCOL_CMD_GET_STATUS;
    }
    else if (Protocol_Equals(cmd, "MODE=AUTO") || Protocol_Equals(cmd, "MODE_AUTO"))
    {
        packet->command = PROTOCOL_CMD_MODE_AUTO;
    }
    else if (Protocol_Equals(cmd, "MODE=MANUAL") || Protocol_Equals(cmd, "MODE_MANUAL"))
    {
        packet->command = PROTOCOL_CMD_MODE_MANUAL;
    }
    else if (Protocol_Equals(cmd, "MODE=TOGGLE") || Protocol_Equals(cmd, "MODE_TOGGLE"))
    {
        packet->command = PROTOCOL_CMD_MODE_TOGGLE;
    }
    else if (Protocol_Equals(cmd, "LED=ON") || Protocol_Equals(cmd, "LED_ON"))
    {
        packet->command = PROTOCOL_CMD_LED_ON;
    }
    else if (Protocol_Equals(cmd, "LED=OFF") || Protocol_Equals(cmd, "LED_OFF"))
    {
        packet->command = PROTOCOL_CMD_LED_OFF;
    }
    else if (Protocol_Equals(cmd, "BRIGHT+" ) || Protocol_Equals(cmd, "BRIGHT_UP") || Protocol_Equals(cmd, "BRTI"))
    {
        packet->command = PROTOCOL_CMD_BRIGHT_UP;
    }
    else if (Protocol_Equals(cmd, "BRIGHT-" ) || Protocol_Equals(cmd, "BRIGHT_DOWN") || Protocol_Equals(cmd, "BRTD"))
    {
        packet->command = PROTOCOL_CMD_BRIGHT_DOWN;
    }
    else if (Protocol_StartsWith(cmd, "BRIGHT="))
    {
        packet->command = PROTOCOL_CMD_BRIGHT_SET;
        packet->value = Protocol_ClampPercent((int16_t)atoi(cmd + 7));
    }
    else if (Protocol_Equals(cmd, "COUNT?") || Protocol_Equals(cmd, "HUMAN_COUNT?"))
    {
        packet->command = PROTOCOL_CMD_COUNT_GET;
    }
    else if (Protocol_Equals(cmd, "COUNT_RESET") || Protocol_Equals(cmd, "HUMAN_COUNT_RESET"))
    {
        packet->command = PROTOCOL_CMD_COUNT_RESET;
    }
    else
    {
        packet->command = PROTOCOL_CMD_UNKNOWN;
    }

    Serial_RXFlag = 0;
    Serial_RXPacket[0] = '\0';
    return 1;
}

void Protocol_SendOK(const char *cmd)
{
    Serial_Printf("#OK,CMD=%s\r\n", cmd);
}

void Protocol_SendError(const char *code)
{
    Serial_Printf("#ERR,CODE=%s\r\n", code);
}

void Protocol_SendPong(void)
{
    Serial_SendString("#PONG\r\n");
}

void Protocol_SendStatus(const ProtocolStatus_t *status)
{
    if (status == 0)
    {
        return;
    }

    Serial_Printf("#STATUS,MODE=%s,LIGHT=%d,BRIGHT=%d,TARGET=%d,HUMAN=%d,HIT=%lu\r\n",
                  status->modeAuto ? "AUTO" : "MANUAL",
                  status->lightOn,
                  status->brightness,
                  status->target,
                  status->human,
                  status->humanTriggerCount);
}

void Protocol_SendMode(uint8_t modeAuto)
{
    Serial_Printf("#EVT,MODE=%s\r\n", modeAuto ? "AUTO" : "MANUAL");
}

void Protocol_SendLight(uint8_t lightOn)
{
    Serial_Printf("#EVT,LIGHT=%d\r\n", lightOn ? 1 : 0);
}

void Protocol_SendHuman(uint8_t human, uint32_t count)
{
    Serial_Printf("#EVT,HUMAN=%d,HIT=%lu\r\n", human ? 1 : 0, count);
}

void Protocol_SendBrightness(uint8_t brightness, uint8_t target)
{
    Serial_Printf("#EVT,BRIGHT=%d,TARGET=%d\r\n", brightness, target);
}

void Protocol_SendCount(uint32_t count)
{
    Serial_Printf("#COUNT,HUMAN_HIT=%lu\r\n", count);
}

void Protocol_SendGesture(const char *name)
{
    Serial_Printf("#EVT,GESTURE=%s\r\n", name);
}

void Protocol_SendDim(const char *state)
{
    Serial_Printf("#EVT,DIM=%s\r\n", state);
}
