#include <motor.h>
#include <rfid_sensor.h>
#include <stdio.h>

EventGroupHandle_t gate_events;
#define SENSOR_CLOSE 0x1
#define SENSOR_OPEN 0x2
#define SENSOR_OBSTRUCTION 0X4 // can clear
#define BLOCKED 0x8            // do not clear unless the manager
#define CLOSE_ADMIN 0x10
#define OPEN_ADMIN 0x20

#define MOTOR_PIN_1 10
#define MOTOR_PIN_2 11
#define MOTOR_CLOSE MOTOR_CCW_MODE
#define MOTOR_OPEN MOTOR_CW_MODE

#define MOTOR_TIMEOUT pdMS_TO_TICKS(5000)

// todo
bool valid_tag(uint32_t tag);

void app_main(void)
{
    gate_events = xEventGroupCreate();
    xEventGroupClearBits(gate_events, 0xFF);
    init_rfid_task();
}

enum GATE_STATE
{
    GATE_CLOSED,
    GATE_CLOSED_BLOCKED,
    GATE_CLOSING,
    GATE_OPENING,
    GATE_OPEN,
    GATE_OPEN_BLOCKED,
    GATE_ERROR,
};

// add messages
void gate_task(void *pvParam)
{
    uint32_t events;
    motor_type motor;
    enum GATE_STATE state = GATE_CLOSED;
    motor_init(&motor, MOTOR_PIN_1, MOTOR_PIN_2);

    if (initial_test_gate(&motor) == ESP_ERR_TIMEOUT)
        state = GATE_ERROR;

    for (;;)
    {
        switch (state)
        {
        case GATE_CLOSED:
            if (closed())
                state = GATE_OPENING;
            else
                state = GATE_CLOSED_BLOCKED;
            break;

        case GATE_CLOSED_BLOCKED:
            if (closed_blocked())
                state = GATE_OPENING;
            else
                state = GATE_CLOSED;
            break;

        case GATE_OPENING:
            motor_set(&motor, MOTOR_OPEN);
            events = xEventGroupWaitBits(gate_events, SENSOR_OPEN, true, true, MOTOR_TIMEOUT);
            motor_set(&motor, MOTOR_STOP_MODE);
            if (!(events & SENSOR_OPEN))
                state = GATE_ERROR;
            if (events & BLOCKED)
                state = GATE_OPEN_BLOCKED;
            else
                state = GATE_OPEN;
            break;

        case GATE_OPEN:
            vTaskDelay(pdTICKS_TO_MS(50000));
            events = xEventGroupGetBits(gate_events);
            if (events & BLOCKED)
                state = GATE_OPEN_BLOCKED;
            else
                state = GATE_CLOSING;
            break;

        case GATE_OPEN_BLOCKED:
            if (open_blocked())
                state = GATE_CLOSING;
            else
                state = GATE_OPEN;
            break;

        case GATE_CLOSING:
            xEventGroupClearBits(gate_events, SENSOR_OBSTRUCTION);
            motor_set(&motor, MOTOR_CLOSE);
            events = xEventGroupWaitBits(gate_events, SENSOR_CLOSE | SENSOR_OBSTRUCTION, true, false, MOTOR_TIMEOUT);
            motor_set(&motor, MOTOR_STOP_MODE);
            if (events & SENSOR_OBSTRUCTION)
            {
                motor_set(&motor, MOTOR_OPEN);
                events = xEventGroupWaitBits(gate_events, SENSOR_OPEN, true, true, MOTOR_TIMEOUT);
                motor_set(&motor, MOTOR_STOP_MODE);
                if (!(events & SENSOR_OPEN))
                    state = GATE_ERROR;
                else
                {
                    vTaskDelay(pdTICKS_TO_MS(5000));
                    state = GATE_CLOSING;
                }
            }
            else if (events & SENSOR_CLOSE)
            {
                if (events & BLOCKED)
                    state = GATE_CLOSED_BLOCKED;
                else
                    state = GATE_CLOSED;
            }
            else
            {
                state = GATE_ERROR;
            }

            break;

        default:
            vTaskDelay(1000);
            // say(error detected, restart)
            break;
        }
    }
}

esp_err_t initial_test_gate(motor_type *motor)
{
    uint16_t test;
    test = xEventGroupGetBits(gate_events);
    if (!(test & SENSOR_CLOSE))
    {
        motor_set(motor, MOTOR_CLOSE);
        test = xEventGroupWaitBits(gate_events, SENSOR_CLOSE, true, true, MOTOR_TIMEOUT);
        motor_set(motor, MOTOR_STOP_MODE);
        if (test & SENSOR_CLOSE)
            return ESP_OK;
        else
            return ESP_ERR_TIMEOUT;
    }
}

// 0 -> blocked
// 1 -> can open
int closed()
{
    uint16_t events;

    uint32_t rfid = 0X00000000;
    while (!valid_tag(rfid))
    {
        rfid = wait_until_lrfid(pdMS_TO_TICKS(50), false);
        events = xEventGroupGetBits(gate_events);
        if (events & BLOCKED)
            return 0;
    }
    return 1;
}

int closed_blocked()
{
    uint16_t events = 0x0;
    while (events & BLOCKED)
    {
        events = xEventGroupWaitBits(gate_events, OPEN_ADMIN, true, false, pdMS_TO_TICKS(50));
        if (events & OPEN_ADMIN)
        {
            xEventGroupClearBits(gate_events, OPEN_ADMIN);
            return 1;
        }
    }
    return 0;
}

int open_blocked()
{
    uint16_t events = 0x0;
    while (events & BLOCKED)
    {
        events = xEventGroupWaitBits(gate_events, CLOSE_ADMIN, true, false, pdMS_TO_TICKS(50));
        if (events & CLOSE_ADMIN)
        {
            return 1;
        }
    }
    return 0;
}
