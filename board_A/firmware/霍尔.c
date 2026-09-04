#include "STC15F2K60S2.H"
#include "sys.H"
#include "hall.H"
#include "Vib.h"
#include "Beep.h"
#include "uart1.h"
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

typedef enum
{
    SECURITY_STATE_DISARMED = 0,
    SECURITY_STATE_ARMED,
    SECURITY_STATE_ALARM
} SecurityState;

#define LED_DOOR_UNKNOWN 0x00
#define LED_DOOR_CLOSED  0x01
#define LED_DOOR_OPEN    0x02
#define LED_VIB_EVENT    0x04
#define LED_ARMED        0x08
#define LED_ALARM        0x10

#define ALARM_FLAG_NONE 0x00
#define ALARM_FLAG_DOOR 0x01
#define ALARM_FLAG_VIB  0x02

#define SECURITY_RESULT_FAILED 0
#define SECURITY_RESULT_OK     1

#define PROTOCOL_SEND_FAILED 0
#define PROTOCOL_SEND_OK     1

#define ALARM_BEEP_FREQUENCY 1200
#define ALARM_BEEP_TIME      100

#define UART_BAUD_RATE 1200
#define UART_TX_QUEUE_SIZE 8

#define CMD_ARM        0xA1
#define CMD_DISARM     0xA2
#define CMD_RESET      0xA3

#define MSG_ARM_OK     0xB1
#define MSG_DISARM_OK  0xB2
#define MSG_DOOR_OPEN  0xB3
#define MSG_DOOR_CLOSE 0xB4
#define MSG_DOOR_ALARM 0xB5
#define MSG_VIB_ALARM  0xB6
#define MSG_HEARTBEAT  0xC1

/* Set all switches to 0 before building firmware for real hardware. */
#define HALL_SOFT_TEST 0
#define VIB_SOFT_TEST  0
#define STATE_SOFT_TEST 0
#define UART_SOFT_TEST 1

#define HALL_TEST_1_FAILED 0x01
#define HALL_TEST_2_FAILED 0x02
#define HALL_TEST_3_FAILED 0x04
#define HALL_TEST_4_FAILED 0x08
#define HALL_TEST_5_FAILED 0x10

#define VIB_TEST_1_FAILED 0x01
#define VIB_TEST_2_FAILED 0x02
#define VIB_TEST_3_FAILED 0x04

#define STATE_TEST_1_FAILED  0x0001
#define STATE_TEST_2_FAILED  0x0002
#define STATE_TEST_3_FAILED  0x0004
#define STATE_TEST_4_FAILED  0x0008
#define STATE_TEST_5_FAILED  0x0010
#define STATE_TEST_6_FAILED  0x0020
#define STATE_TEST_7_FAILED  0x0040
#define STATE_TEST_8_FAILED  0x0080
#define STATE_TEST_9_FAILED  0x0100
#define STATE_TEST_10_FAILED 0x0200

#define UART_TEST_1_FAILED 0x01
#define UART_TEST_2_FAILED 0x02
#define UART_TEST_3_FAILED 0x04
#define UART_TEST_4_FAILED 0x08
#define UART_TEST_5_FAILED 0x10
#define UART_TEST_6_FAILED 0x20
#define UART_TEST_7_FAILED 0x40

#define TX_COUNT_ARM_OK     0
#define TX_COUNT_DISARM_OK  1
#define TX_COUNT_DOOR_OPEN  2
#define TX_COUNT_DOOR_CLOSE 3
#define TX_COUNT_DOOR_ALARM 4
#define TX_COUNT_VIB_ALARM  5
#define TX_COUNT_HEARTBEAT  6
#define TX_COUNT_SIZE       7

DoorState door_state = DOOR_STATE_UNKNOWN;
unsigned char door_changed = 0;
unsigned char vib_event = 0;
unsigned char vib_event_count = 0;
SecurityState security_state = SECURITY_STATE_DISARMED;
unsigned char alarm_flags = ALARM_FLAG_NONE;
unsigned char alarm_beep_pending = 0;
unsigned char door_report_pending = 0;
unsigned char alarm_reported_flags = ALARM_FLAG_NONE;

unsigned char uart_rx_byte = 0;
unsigned char uart_tx_byte = 0;
unsigned char uart_tx_queue[UART_TX_QUEUE_SIZE];
unsigned char uart_tx_head = 0;
unsigned char uart_tx_tail = 0;
unsigned char uart_tx_count = 0;
unsigned char uart_tx_drop_count = 0;

#if HALL_SOFT_TEST
unsigned char hall_soft_test_failures = 0;
unsigned char hall_soft_test_completed = 0;
#endif

#if VIB_SOFT_TEST
unsigned char vib_soft_test_failures = 0;
unsigned char vib_soft_test_completed = 0;
#endif

#if STATE_SOFT_TEST
unsigned int state_soft_test_failures = 0;
unsigned char state_soft_test_completed = 0;
#endif

#if UART_SOFT_TEST
unsigned char uart_soft_test_failures = 0;
unsigned char uart_soft_test_completed = 0;
unsigned char last_tx_message = 0;
unsigned char tx_message_count = 0;
unsigned char uart_soft_tx_counts[TX_COUNT_SIZE];
#endif

unsigned char security_arm(void);
void security_disarm(void);
unsigned char security_reset_alarm(void);

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
        door_report_pending = 1;
    }
}

void clear_vib_event(void)
{
    vib_event = 0;
}

void process_vib_action(unsigned char vib_action)
{
    if (vib_action == enumVibQuake)
    {
        vib_event = 1;
        vib_event_count++;
    }
}

unsigned char send_protocol_message(unsigned char message)
{
#if UART_SOFT_TEST
    last_tx_message = message;
    tx_message_count++;

    if (message == MSG_ARM_OK)
    {
        uart_soft_tx_counts[TX_COUNT_ARM_OK]++;
    }
    else if (message == MSG_DISARM_OK)
    {
        uart_soft_tx_counts[TX_COUNT_DISARM_OK]++;
    }
    else if (message == MSG_DOOR_OPEN)
    {
        uart_soft_tx_counts[TX_COUNT_DOOR_OPEN]++;
    }
    else if (message == MSG_DOOR_CLOSE)
    {
        uart_soft_tx_counts[TX_COUNT_DOOR_CLOSE]++;
    }
    else if (message == MSG_DOOR_ALARM)
    {
        uart_soft_tx_counts[TX_COUNT_DOOR_ALARM]++;
    }
    else if (message == MSG_VIB_ALARM)
    {
        uart_soft_tx_counts[TX_COUNT_VIB_ALARM]++;
    }
    else if (message == MSG_HEARTBEAT)
    {
        uart_soft_tx_counts[TX_COUNT_HEARTBEAT]++;
    }

    return PROTOCOL_SEND_OK;
#else
    if (uart_tx_count >= UART_TX_QUEUE_SIZE)
    {
        uart_tx_drop_count++;
        return PROTOCOL_SEND_FAILED;
    }

    uart_tx_queue[uart_tx_tail] = message;
    uart_tx_tail++;
    if (uart_tx_tail >= UART_TX_QUEUE_SIZE)
    {
        uart_tx_tail = 0;
    }
    uart_tx_count++;

    return PROTOCOL_SEND_OK;
#endif
}

void service_uart_tx(void)
{
#if !UART_SOFT_TEST
    if ((uart_tx_count != 0) &&
        (GetUart1TxStatus() == enumUart1TxFree))
    {
        uart_tx_byte = uart_tx_queue[uart_tx_head];
        if (Uart1Print(&uart_tx_byte, 1) == enumUart1TxOK)
        {
            uart_tx_head++;
            if (uart_tx_head >= UART_TX_QUEUE_SIZE)
            {
                uart_tx_head = 0;
            }
            uart_tx_count--;
        }
    }
#endif
}

void process_uart_command(unsigned char command)
{
    if (command == CMD_ARM)
    {
        if (security_arm() == SECURITY_RESULT_OK)
        {
            send_protocol_message(MSG_ARM_OK);
        }
        else if (door_state == DOOR_STATE_OPEN)
        {
            send_protocol_message(MSG_DOOR_OPEN);
        }
        else
        {
            /* TODO: protocol needs response for unknown door state or ARM in ALARM. */
        }
    }
    else if (command == CMD_DISARM)
    {
        security_disarm();
        send_protocol_message(MSG_DISARM_OK);
    }
    else if (command == CMD_RESET)
    {
        security_reset_alarm();
        /* TODO: protocol has no RESET_OK or RESET_FAILED response. */
    }
}

void process_protocol_reports(void)
{
    unsigned char pending_alarm_flags;

    if (door_report_pending != 0)
    {
        if (door_state == DOOR_STATE_CLOSED)
        {
            if (send_protocol_message(MSG_DOOR_CLOSE) == PROTOCOL_SEND_OK)
            {
                door_report_pending = 0;
            }
        }
        else if (door_state == DOOR_STATE_OPEN)
        {
            if (send_protocol_message(MSG_DOOR_OPEN) == PROTOCOL_SEND_OK)
            {
                door_report_pending = 0;
            }
        }
    }

    pending_alarm_flags = alarm_flags & (~alarm_reported_flags);

    if ((pending_alarm_flags & ALARM_FLAG_DOOR) != 0)
    {
        if (send_protocol_message(MSG_DOOR_ALARM) == PROTOCOL_SEND_OK)
        {
            alarm_reported_flags |= ALARM_FLAG_DOOR;
        }
    }

    if ((pending_alarm_flags & ALARM_FLAG_VIB) != 0)
    {
        if (send_protocol_message(MSG_VIB_ALARM) == PROTOCOL_SEND_OK)
        {
            alarm_reported_flags |= ALARM_FLAG_VIB;
        }
    }
}

void uart1_receive_callback(void)
{
    process_uart_command(uart_rx_byte);
}

void heartbeat_1s_callback(void)
{
    send_protocol_message(MSG_HEARTBEAT);
}

unsigned char security_arm(void)
{
    if (security_state == SECURITY_STATE_ALARM)
    {
        return SECURITY_RESULT_FAILED;
    }

    if (door_state != DOOR_STATE_CLOSED)
    {
        return SECURITY_RESULT_FAILED;
    }

    security_state = SECURITY_STATE_ARMED;
    alarm_flags = ALARM_FLAG_NONE;
    alarm_reported_flags = ALARM_FLAG_NONE;
    alarm_beep_pending = 0;
    door_changed = 0;
    clear_vib_event();

    return SECURITY_RESULT_OK;
}

void security_disarm(void)
{
    security_state = SECURITY_STATE_DISARMED;
    alarm_flags = ALARM_FLAG_NONE;
    alarm_reported_flags = ALARM_FLAG_NONE;
    alarm_beep_pending = 0;
    door_changed = 0;
    clear_vib_event();
}

unsigned char security_reset_alarm(void)
{
    if (security_state != SECURITY_STATE_ALARM)
    {
        return SECURITY_RESULT_FAILED;
    }

    if (door_state != DOOR_STATE_CLOSED)
    {
        return SECURITY_RESULT_FAILED;
    }

    security_state = SECURITY_STATE_ARMED;
    alarm_flags = ALARM_FLAG_NONE;
    alarm_reported_flags = ALARM_FLAG_NONE;
    alarm_beep_pending = 0;
    door_changed = 0;
    clear_vib_event();

    return SECURITY_RESULT_OK;
}

void security_raise_alarm(unsigned char alarm_flag)
{
    if (security_state != SECURITY_STATE_ALARM)
    {
        security_state = SECURITY_STATE_ALARM;
        alarm_beep_pending = 1;
    }

    alarm_flags |= alarm_flag;
}

void process_security_state(void)
{
    if (security_state == SECURITY_STATE_DISARMED)
    {
        door_changed = 0;
        clear_vib_event();
        return;
    }

    if (door_changed != 0)
    {
        if (door_state == DOOR_STATE_OPEN)
        {
            security_raise_alarm(ALARM_FLAG_DOOR);
        }
        door_changed = 0;
    }

    if (vib_event != 0)
    {
        security_raise_alarm(ALARM_FLAG_VIB);
        clear_vib_event();
    }
}

void process_alarm_output(void)
{
    if ((security_state == SECURITY_STATE_ALARM) &&
        (alarm_beep_pending != 0) &&
        (GetBeepStatus() == enumBeepFree))
    {
        if (SetBeep(ALARM_BEEP_FREQUENCY, ALARM_BEEP_TIME) == enumSetBeepOK)
        {
            alarm_beep_pending = 0;
        }
    }
}

void sensor_led_100ms_callback(void)
{
    unsigned char led_value;

    led_value = LED_DOOR_UNKNOWN;

    if (door_state == DOOR_STATE_CLOSED)
    {
        led_value |= LED_DOOR_CLOSED;
    }
    else if (door_state == DOOR_STATE_OPEN)
    {
        led_value |= LED_DOOR_OPEN;
    }

    process_security_state();
#if (!STATE_SOFT_TEST) && (!HALL_SOFT_TEST) && (!VIB_SOFT_TEST)
    process_protocol_reports();
    service_uart_tx();
#endif
    process_alarm_output();

    if ((alarm_flags & ALARM_FLAG_VIB) != 0)
    {
        led_value |= LED_VIB_EVENT;
    }

#if VIB_SOFT_TEST
    if ((vib_soft_test_completed != 0) && (vib_soft_test_failures == 0))
    {
        led_value |= LED_VIB_EVENT;
    }
#endif

    if (security_state == SECURITY_STATE_ARMED)
    {
        led_value |= LED_ARMED;
    }
    else if (security_state == SECURITY_STATE_ALARM)
    {
        led_value |= LED_ALARM;
    }

    LedPrint(led_value);
}

void hall_callback(void)
{
    unsigned char hall_action;

    hall_action = GetHallAct();
    process_hall_action(hall_action);
}

void vib_callback(void)
{
    unsigned char vib_action;

    vib_action = GetVibAct();
    process_vib_action(vib_action);
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

#if VIB_SOFT_TEST
void run_vib_soft_tests(void)
{
    vib_soft_test_failures = 0;
    vib_soft_test_completed = 0;
    vib_event = 0;
    vib_event_count = 0;

    process_vib_action(enumVibQuake);
    if ((vib_event != 1) || (vib_event_count != 1))
    {
        vib_soft_test_failures |= VIB_TEST_1_FAILED;
    }

    clear_vib_event();
    process_vib_action(enumVibQuake);
    if ((vib_event != 1) || (vib_event_count != 2))
    {
        vib_soft_test_failures |= VIB_TEST_2_FAILED;
    }

    clear_vib_event();
    process_vib_action(enumVibNull);
    if ((vib_event != 0) || (vib_event_count != 2))
    {
        vib_soft_test_failures |= VIB_TEST_3_FAILED;
    }

    vib_soft_test_completed = 1;
}
#endif

#if STATE_SOFT_TEST
void run_state_soft_tests(void)
{
    state_soft_test_failures = 0;
    state_soft_test_completed = 0;
    door_state = DOOR_STATE_UNKNOWN;
    door_changed = 0;
    vib_event = 0;
    vib_event_count = 0;
    security_state = SECURITY_STATE_DISARMED;
    alarm_flags = ALARM_FLAG_NONE;
    alarm_beep_pending = 0;

    if (security_state != SECURITY_STATE_DISARMED)
    {
        state_soft_test_failures |= STATE_TEST_1_FAILED;
    }

    if ((security_arm() != SECURITY_RESULT_FAILED) ||
        (security_state != SECURITY_STATE_DISARMED))
    {
        state_soft_test_failures |= STATE_TEST_2_FAILED;
    }

    process_hall_action(enumHallGetAway);
    if ((security_arm() != SECURITY_RESULT_FAILED) ||
        (security_state != SECURITY_STATE_DISARMED))
    {
        state_soft_test_failures |= STATE_TEST_3_FAILED;
    }

    process_hall_action(enumHallGetClose);
    if ((security_arm() != SECURITY_RESULT_OK) ||
        (security_state != SECURITY_STATE_ARMED))
    {
        state_soft_test_failures |= STATE_TEST_4_FAILED;
    }

    process_hall_action(enumHallGetAway);
    process_security_state();
    process_security_state();
    if ((security_state != SECURITY_STATE_ALARM) ||
        ((alarm_flags & ALARM_FLAG_DOOR) == 0))
    {
        state_soft_test_failures |= STATE_TEST_5_FAILED;
    }

    security_disarm();
    if ((security_state != SECURITY_STATE_DISARMED) ||
        (alarm_flags != ALARM_FLAG_NONE))
    {
        state_soft_test_failures |= STATE_TEST_6_FAILED;
    }

    process_hall_action(enumHallGetClose);
    security_arm();
    process_vib_action(enumVibQuake);
    process_security_state();
    if ((security_state != SECURITY_STATE_ALARM) ||
        ((alarm_flags & ALARM_FLAG_VIB) == 0))
    {
        state_soft_test_failures |= STATE_TEST_7_FAILED;
    }

    if ((security_reset_alarm() != SECURITY_RESULT_OK) ||
        (security_state != SECURITY_STATE_ARMED) ||
        (alarm_flags != ALARM_FLAG_NONE))
    {
        state_soft_test_failures |= STATE_TEST_8_FAILED;
    }

    process_hall_action(enumHallGetAway);
    process_security_state();
    if ((security_reset_alarm() != SECURITY_RESULT_FAILED) ||
        (security_state != SECURITY_STATE_ALARM) ||
        ((alarm_flags & ALARM_FLAG_DOOR) == 0))
    {
        state_soft_test_failures |= STATE_TEST_9_FAILED;
    }

    security_disarm();
    process_vib_action(enumVibQuake);
    process_security_state();
    process_hall_action(enumHallGetClose);
    if (security_arm() != SECURITY_RESULT_OK)
    {
        state_soft_test_failures |= STATE_TEST_10_FAILED;
    }
    process_security_state();
    if ((security_state != SECURITY_STATE_ARMED) ||
        (vib_event != 0) ||
        (alarm_flags != ALARM_FLAG_NONE))
    {
        state_soft_test_failures |= STATE_TEST_10_FAILED;
    }

    state_soft_test_completed = 1;
}
#endif

#if UART_SOFT_TEST
void reset_uart_soft_tx_records(void)
{
    unsigned char index;

    last_tx_message = 0;
    tx_message_count = 0;
    for (index = 0; index < TX_COUNT_SIZE; index++)
    {
        uart_soft_tx_counts[index] = 0;
    }
}

void run_uart_soft_tests(void)
{
    SecurityState state_before;

    uart_soft_test_failures = 0;
    uart_soft_test_completed = 0;

    security_disarm();
    door_state = DOOR_STATE_CLOSED;
    door_changed = 0;
    door_report_pending = 0;
    reset_uart_soft_tx_records();
    process_uart_command(CMD_ARM);
    if ((security_state != SECURITY_STATE_ARMED) ||
        (last_tx_message != MSG_ARM_OK) ||
        (tx_message_count != 1))
    {
        uart_soft_test_failures |= UART_TEST_1_FAILED;
    }

    security_disarm();
    door_state = DOOR_STATE_OPEN;
    door_changed = 0;
    door_report_pending = 0;
    reset_uart_soft_tx_records();
    process_uart_command(CMD_ARM);
    if ((security_state != SECURITY_STATE_DISARMED) ||
        (last_tx_message != MSG_DOOR_OPEN) ||
        (tx_message_count != 1))
    {
        uart_soft_test_failures |= UART_TEST_2_FAILED;
    }

    security_disarm();
    door_state = DOOR_STATE_CLOSED;
    security_arm();
    reset_uart_soft_tx_records();
    process_uart_command(CMD_DISARM);
    if ((security_state != SECURITY_STATE_DISARMED) ||
        (last_tx_message != MSG_DISARM_OK) ||
        (tx_message_count != 1))
    {
        uart_soft_test_failures |= UART_TEST_3_FAILED;
    }

    door_state = DOOR_STATE_CLOSED;
    security_arm();
    door_report_pending = 0;
    reset_uart_soft_tx_records();
    process_hall_action(enumHallGetAway);
    process_security_state();
    process_protocol_reports();
    process_protocol_reports();
    if ((security_state != SECURITY_STATE_ALARM) ||
        ((alarm_flags & ALARM_FLAG_DOOR) == 0) ||
        (uart_soft_tx_counts[TX_COUNT_DOOR_ALARM] != 1))
    {
        uart_soft_test_failures |= UART_TEST_4_FAILED;
    }

    security_disarm();
    door_state = DOOR_STATE_CLOSED;
    security_arm();
    door_report_pending = 0;
    reset_uart_soft_tx_records();
    process_vib_action(enumVibQuake);
    process_security_state();
    process_protocol_reports();
    process_protocol_reports();
    if ((security_state != SECURITY_STATE_ALARM) ||
        ((alarm_flags & ALARM_FLAG_VIB) == 0) ||
        (uart_soft_tx_counts[TX_COUNT_VIB_ALARM] != 1))
    {
        uart_soft_test_failures |= UART_TEST_5_FAILED;
    }

    state_before = security_state;
    reset_uart_soft_tx_records();
    process_uart_command(0x55);
    if ((security_state != state_before) || (tx_message_count != 0))
    {
        uart_soft_test_failures |= UART_TEST_6_FAILED;
    }

    security_disarm();
    door_state = DOOR_STATE_CLOSED;
    door_changed = 0;
    door_report_pending = 0;
    reset_uart_soft_tx_records();
    process_hall_action(enumHallGetAway);
    process_security_state();
    process_protocol_reports();
    process_hall_action(enumHallGetAway);
    process_security_state();
    process_protocol_reports();
    process_hall_action(enumHallGetClose);
    process_security_state();
    process_protocol_reports();
    if ((uart_soft_tx_counts[TX_COUNT_DOOR_OPEN] != 1) ||
        (uart_soft_tx_counts[TX_COUNT_DOOR_CLOSE] != 1) ||
        (tx_message_count != 2))
    {
        uart_soft_test_failures |= UART_TEST_7_FAILED;
    }

    uart_soft_test_completed = 1;
}
#endif

void main(void)
{
    DisplayerInit();
    BeepInit();
#if UART_SOFT_TEST
    run_uart_soft_tests();
#elif STATE_SOFT_TEST
    run_state_soft_tests();
#elif HALL_SOFT_TEST
    run_hall_soft_tests();
#elif VIB_SOFT_TEST
    run_vib_soft_tests();
#else
    HallInit();
    VibInit();
    Uart1Init(UART_BAUD_RATE);
#endif
    SetDisplayerArea(0, 7);
    Seg7Print(10, 10, 10, 10, 10, 10, 10, 10);
    LedPrint(LED_DOOR_UNKNOWN);

    SetEventCallBack(enumEventSys100mS, sensor_led_100ms_callback);
#if (!HALL_SOFT_TEST) && (!VIB_SOFT_TEST) && (!STATE_SOFT_TEST) && (!UART_SOFT_TEST)
    SetEventCallBack(enumEventHall, hall_callback);
    SetEventCallBack(enumEventVib, vib_callback);
    SetEventCallBack(enumEventUart1Rxd, uart1_receive_callback);
    SetEventCallBack(enumEventSys1S, heartbeat_1s_callback);
    SetUart1Rxd(&uart_rx_byte, 1, 0, 0);
#endif

    MySTC_Init();

    while (1)
    {
        MySTC_OS();
    }
}
