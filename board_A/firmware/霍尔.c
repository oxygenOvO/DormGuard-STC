#include "STC15F2K60S2.H"
#include "sys.H"
#include "hall.H"
#include "displayer.H"

code unsigned long SysClock = 11059200;

#ifdef _displayer_H_
code char decode_table[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
    0x7f, 0x6f, 0x00, 0x08, 0x40, 0x01, 0x76, 0x38
};
#endif

typedef enum
{
    DOOR_STATE_UNKNOWN = 0,
    DOOR_STATE_CLOSED,
    DOOR_STATE_OPEN
} DoorState;

#define LED_DOOR_UNKNOWN 0x00
#define LED_DOOR_CLOSED  0x01
#define LED_DOOR_OPEN    0x02

DoorState door_state = DOOR_STATE_UNKNOWN;
unsigned char door_changed = 0;

void door_led_100ms_callback(void)
{
    if (door_state == DOOR_STATE_CLOSED)
    {
        LedPrint(LED_DOOR_CLOSED);
    }
    else if (door_state == DOOR_STATE_OPEN)
    {
        LedPrint(LED_DOOR_OPEN);
    }
    else
    {
        LedPrint(LED_DOOR_UNKNOWN);
    }
}

void hall_callback(void)
{
    unsigned char hall_action;
    DoorState new_state;

    hall_action = GetHallAct();
    new_state = door_state;

    /* This mapping assumes the magnet is near Hall when the door is closed. */
    if (hall_action == enumHallGetClose)
    {
        new_state = DOOR_STATE_CLOSED;
    }
    else if (hall_action == enumHallGetAway)
    {
        new_state = DOOR_STATE_OPEN;
    }

    if (new_state != door_state)
    {
        door_state = new_state;
        door_changed = 1;
    }
}

void main(void)
{
    DisplayerInit();
    HallInit();
    SetDisplayerArea(0, 7);
    Seg7Print(10, 10, 10, 10, 10, 10, 10, 10);
    LedPrint(LED_DOOR_UNKNOWN);

    SetEventCallBack(enumEventSys100mS, door_led_100ms_callback);
    SetEventCallBack(enumEventHall, hall_callback);

    MySTC_Init();

    while (1)
    {
        MySTC_OS();
    }
}
