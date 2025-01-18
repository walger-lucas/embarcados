#include "rfid_sensor.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <string.h>

static QueueHandle_t lrfid_queue_main;
static QueueHandle_t lrfid_queue_menu;
static TaskHandle_t local_rfid_task_handle;
static bool lrfid_in_menu = false;

static void setup_local_rfid(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(LOCAL_RFID_UART, 128, 0, 0, NULL, 0);
    uart_param_config(LOCAL_RFID_UART, &uart_config);
    uart_set_pin(LOCAL_RFID_UART, LOCAL_RFID_TXD_PIN, LOCAL_RFID_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void local_rfid_task()
{
    setup_local_rfid();
    lrfid_queue_main = xQueueCreate(1, sizeof(uint32_t));
    lrfid_queue_menu = xQueueCreate(1, sizeof(uint32_t));
    for (;;)
    {
        uint8_t code[9] = {0};
        while (code[0] != 0x02)
        {
            uart_read_bytes(LOCAL_RFID_UART, code, 1, portMAX_DELAY);
        }
        if (!uart_read_bytes(LOCAL_RFID_UART, code + 1, 8, pdMS_TO_TICKS(LOCAL_RFID_TIMEOUT_MS)))
            continue;

        uint8_t xor = code[1] ^ code[2] ^ code[3] ^ code[4] ^ code[5] ^ code[6];
        if (code[8] == 0x03 && code[7] == xor)
        {
            if (lrfid_in_menu)
                xQueueOverwrite(lrfid_queue_menu, code + 3);
            else
                xQueueOverwrite(lrfid_queue_main, code + 3);
        }
    }
}

uint32_t wait_until_lrfid(TickType_t xTicksToWait, bool menu)
{
    uint32_t code;
    if (menu)
        xQueueReceive(lrfid_queue_menu, &code, xTicksToWait);
    else
        xQueueReceive(lrfid_queue_main, &code, xTicksToWait);
    return code;
}

void init_rfid_task()
{
    xTaskCreate(local_rfid_task, "LOCAL RFID", 2048, NULL, 2, &local_rfid_task_handle);
}

void rfidtag_2_str(uint32_t tag, char *str)
{
    sprintf(str, "%05d,%05d", *(((uint16_t *)tag) + 2), *(((uint16_t *)tag)));
}
uint32_t str_2_rfidtag(char separator, const char *str)
{
    char buffer[32];
    char *buf = buffer;

    strcpy(buffer, str);
    for (char *msg = buffer; *msg != 0; msg++)
    {
        if (*msg == separator)
            *msg = ' ';
    }
    uint32_t half_word1 = 0xFFFFFFFF;
    uint32_t half_word2 = 0xFFFFFFFF;
    half_word1 = strtoul(buffer, &buf, 10); // translate first halfword
    if (half_word1 > 0xFFFF)
        return 0;
    half_word2 = strtoul(buf, &buf, 10); // translate second halfword
    if (half_word2 > 0xFFFF)
        return 0;
    return (half_word1 << 16) | half_word2;
}

void local_rfid_suspend()
{
    vTaskSuspend(local_rfid_task_handle);
}
void local_rfid_resume()
{
    vTaskResume(local_rfid_task_handle);
}

void lrfid_forget()
{
    xQueueReset(lrfid_queue_menu);
    xQueueReset(lrfid_queue_main);
}

void set_menu(bool menu)
{
    lrfid_in_menu = menu;
}