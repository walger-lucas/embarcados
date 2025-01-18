#ifndef MOTOR_H
#define MOTOR_H
#include <driver/gpio.h>
/*MOTOR Abstraction struct*/
typedef struct motor_type
{
    gpio_num_t pin_relay1;
    gpio_num_t pin_relay2;
} motor_type;
/*MOTOR STATES, movement to be done*/
typedef enum MOTOR_MODE_TYPE
{
    MOTOR_CW_MODE,   // MOTOR CLOCKWISE
    MOTOR_CCW_MODE,  // MOTOR COUNTER CLOCKWISE
    MOTOR_STOP_MODE, // NOT MOVE
} MOTOR_MODE_TYPE;

// Definitions of logic levels to the motor
typedef enum MOTOR_STATE_TYPE
{
    MOTOR_LOGIC_OFF = 0,
    MOTOR_LOGIC_ON = 1,
} MOTOR_STATE_TYPE;

/*
@brief Initializes motor_type struct, abstraction to the motor
@param motor motor abstraction struct to be initialized
@param pin_relay1 pin connected to   and the relay 1 to control the motor
@param pin_relay2 pin connected to the  and the relay 2, if there is no relay 2, put 0 here
*/
esp_err_t motor_init(motor_type *motor, uint16_t pin_relay1, uint16_t pin_relay2);

/*
@brief Sets the state of the MOTOR
@param lcd lcd abstraction struct to be initialized
@param MOTOR_MODE motor mode to be set;
*/
esp_err_t motor_set(motor_type *motor, MOTOR_MODE_TYPE MOTOR_STATE);

#endif
