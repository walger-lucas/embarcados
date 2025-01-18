#pragma once
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

/// @brief Eventgroup where the first 16 bits represent every button
/// @return eventgroup for the buttons when clicked
EventGroupHandle_t get_click_events();
/// @brief Gets current bits being pressed right now
/// @param xTicksToWait max time before error
/// @return current bits being pressed right now
uint16_t get_pressed_keys(TickType_t xTicksToWait);