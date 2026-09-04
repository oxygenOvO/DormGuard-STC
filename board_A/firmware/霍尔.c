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

/* Set to 0 before building firmware for real Hall hardware. */
#define HALL_SOFT_TEST 1

#define HALL_TEST_1_FAILED 0x01
#define HALL_TEST_2_FAILED 0x02
#define HALL_TEST_3_FAILED 0x04
#define HALL_TEST_4_FAILED 0x08
#define HALL_TEST_5_FAILED 0x10

DoorState door_state = DOOR_STATE_UNKNOWN;
unsigned char door_changed = 0;

#if HALL_SOFT_TEST
unsigned char hall_soft_test_failures = 0;
unsigned char hall_soft_test_completed = 0;
#endif

void process_hall_action(unsigned char hall_action)
{
    DoorState new_state;

    new_state = door_state;

    /* TODO: verify physical Hall mapping with magnet. */
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

    hall_action = GetHallAct();
    process_hall_action(hall_action);
}

#if HALL_SOFT_TEST
void run_hall_soft_tests(void)
{
    hall_soft_test_failures = 0;
    hall_soft_test_completed = 0;
    door_state = DOOR_STATE_UNKNOWN;
    door_changed = 0;

    process_hall_action(enumHallGetClose);
    if ((door_state != DOOR_STATE_CLOSED) || (door_changed != 1))
    {
        hall_soft_test_failures |= HALL_TEST_1_FAILED;
    }

    door_changed = 0;
    process_hall_action(enumHallGetClose);
    if ((door_state != DOOR_STATE_CLOSED) || (door_changed != 0))
    {
        hall_soft_test_failures |= HALL_TEST_2_FAILED;
    }

    door_changed = 0;
    process_hall_action(enumHallGetAway);
    if ((door_state != DOOR_STATE_OPEN) || (door_changed != 1))
    {
        hall_soft_test_failures |= HALL_TEST_3_FAILED;
    }

    door_changed = 0;
    process_hall_action(enumHallNull);
    if ((door_state != DOOR_STATE_OPEN) || (door_changed != 0))
    {
        hall_soft_test_failures |= HALL_TEST_4_FAILED;
    }

    door_changed = 0;
    process_hall_action(enumHallGetClose);
    if ((door_state != DOOR_STATE_CLOSED) || (door_changed != 1))
    {
        hall_soft_test_failures |= HALL_TEST_5_FAILED;
    }

    hall_soft_test_completed = 1;
}
#endif

void main(void)
{
    DisplayerInit();
#if HALL_SOFT_TEST
    run_hall_soft_tests();
#else
    HallInit();
#endif
    SetDisplayerArea(0, 7);
    Seg7Print(10, 10, 10, 10, 10, 10, 10, 10);
    LedPrint(LED_DOOR_UNKNOWN);

    SetEventCallBack(enumEventSys100mS, door_led_100ms_callback);
#if !HALL_SOFT_TEST
    SetEventCallBack(enumEventHall, hall_callback);
#endif

    MySTC_Init();

    while (1)
    {
        MySTC_OS();
    }
}
