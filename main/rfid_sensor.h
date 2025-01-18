#pragma once
#include "driver/gpio.h"
#include "driver/uart.h"
#define LOCAL_RFID_UART UART_NUM_2
#define LOCAL_RFID_TXD_PIN GPIO_NUM_30
#define LOCAL_RFID_RXD_PIN GPIO_NUM_31
#define LOCAL_RFID_TIMEOUT_MS 100

/// @brief starts rfid task, permitting the use of the other local_rfid functions
void init_rfid_task();

/// @brief waits ticks given to an rfid uart call
/// @param xTicksToWait time until timeout
/// @param menu utilize the menu queue and not the main queue to send data
/// @return code of the rfid
uint32_t wait_until_lrfid(TickType_t xTicksToWait, bool menu);

/// @brief forgets last rfid if not gotten
void lrfid_forget();

/// @brief suspends lrfid task
void lrfid_suspend();

/// @brief resumes lrfid task
void lrfid_resume();

/// transforms tag into str of the tag, size of str will be 12 with the \0 "000123,09241\0"
void rfidtag_2_str(uint32_t tag, char *str);

/// @brief Gets a string and transforms int into an rfidtag value
/// @param separator separator between the 2 halfwords
/// @param str
/// @return rfidtag value
uint32_t str_2_rfidtag(char separator, const char *str);

/// @brief set to where the event will be sent
/// @param menu
void set_menu(bool menu);