#include "motor.h"
#include <stdio.h>

esp_err_t motor_init(motor_type *motor, uint16_t pin_relay1, uint16_t pin_relay2)
{
    motor->pin_relay1 = pin_relay1;
    motor->pin_relay2 = pin_relay2;
    gpio_config_t cfg = {.pin_bit_mask = 1 << pin_relay1,
                         .mode = GPIO_MODE_OUTPUT,
                         .pull_up_en = GPIO_PULLUP_DISABLE,
                         .pull_down_en = GPIO_PULLDOWN_DISABLE,
                         .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&cfg);
    gpio_config_t cfg2 = {.pin_bit_mask = 1 << pin_relay2,
                          .mode = GPIO_MODE_OUTPUT,
                          .pull_up_en = GPIO_PULLUP_DISABLE,
                          .pull_down_en = GPIO_PULLDOWN_DISABLE,
                          .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&cfg2);
    return motor_set(motor, MOTOR_STOP_MODE);
}

esp_err_t motor_set(motor_type *motor, MOTOR_MODE_TYPE MOTOR_MODE)
{
    switch (MOTOR_MODE)
    {
    case MOTOR_CW_MODE:
        gpio_set_level(motor->pin_relay1, MOTOR_LOGIC_OFF);
        if (motor->pin_relay2)
            gpio_set_level(motor->pin_relay2, MOTOR_LOGIC_ON);
        break;
    case MOTOR_CCW_MODE:
        gpio_set_level(motor->pin_relay1, MOTOR_LOGIC_ON);
        if (motor->pin_relay2)
            gpio_set_level(motor->pin_relay2, MOTOR_LOGIC_OFF);
        break;
    case MOTOR_STOP_MODE:
        gpio_set_level(motor->pin_relay1, MOTOR_LOGIC_ON);
        gpio_set_level(motor->pin_relay2, MOTOR_LOGIC_ON);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
        break;
    }
    return ESP_OK;
}