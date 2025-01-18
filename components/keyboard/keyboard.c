#include "keyboard.h"
#include <freertos/FreeRTOS.h>
#include <rom/ets_sys.h>
#include <stdio.h>
const gpio_num_t out_keyboard[4] = {GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17};
const gpio_num_t in_keyboard[4] = {GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13};
static EventGroupHandle_t keyboard_click_events;
static QueueHandle_t keyboard_mailbox;

void keyboard_setup_gpio()
{
    for (int i = 0; i < 4; i++)
    {
        gpio_config_t cfg = {.pin_bit_mask = 1 << out_keyboard[i],
                             .mode = GPIO_MODE_OUTPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&cfg);
        gpio_set_level(out_keyboard[i], 1);
    }
    for (int i = 0; i < 4; i++)
    {
        gpio_config_t cfg = {.pin_bit_mask = 1 << in_keyboard[i],
                             .mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_ENABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&cfg);
    }
}

uint16_t read_matrix()
{
    static uint16_t last = 0x0000;
    uint16_t read = 0x0000;

    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(out_keyboard[i], 0);
        for (int j = 0; j < 4; j++)
            read = (read << 1) | gpio_get_level(in_keyboard[j]);
    }
    // if multiple clicked, keep last valid read
    uint8_t sum = 0;
    for (int i = 0; i < sizeof(read); i++)
    {
        sum += (read >> i) & 0x1;
    }
    if (sum >= 1)
        return last;
    last = ~read;
    return last; // invert as 0 is clicked and 1 is not clicked in this implementation
}

void keyboard_task(void *pvParam)
{
    keyboard_setup_gpio();
    keyboard_click_events = xEventGroupCreate();
    xEventGroupClearBits(keyboard_click_events, 0xFFFF);
    uint16_t last_pressed_pins = 0x0000;
    keyboard_mailbox = xQueueCreate(1, sizeof(uint16_t));
    xQueueOverwrite(keyboard_mailbox, &last_pressed_pins);
    for (;;)
    {
        uint16_t pressed_pins = read_matrix();
        ets_delay_us(100);
        pressed_pins = pressed_pins & read_matrix();
        xEventGroupSetBits(keyboard_click_events, pressed_pins & (~last_pressed_pins));
        last_pressed_pins = pressed_pins;
        xQueueOverwrite(keyboard_mailbox, &last_pressed_pins);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

EventGroupHandle_t get_click_events()
{
    return keyboard_click_events;
}

uint16_t get_pressed_keys(TickType_t xTicksToWait)
{
    uint16_t val;
    if (xQueuePeek(keyboard_mailbox, &val, xTicksToWait))
        return val;
    return 0;
}
