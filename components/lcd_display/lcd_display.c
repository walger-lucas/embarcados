#include "lcd_display.h"
#include <rom/ets_sys.h>
#include <stdio.h>
#include <string.h>
const static gpio_num_t nums[6] = {LCD_PIN_RS, LCD_PIN_EN, LCD_PIN_D4, LCD_PIN_D5, LCD_PIN_D6, LCD_PIN_D7};
#ifdef LCD_DEBUG
#define LCD_DEBUGF(format, ...) printf(format, ##__VA_ARGS__)
#define LCD_ASSERT(x)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        esp_err_t err = (x);                                                                                           \
        if (unlikely(err != ESP_OK))                                                                                   \
        {                                                                                                              \
            printf("%s:%d\t%s() -> ERROR: %s \n\r", __FILE__, __LINE__, __FUNCTION__, esp_err_to_name(err));           \
            return err;                                                                                                \
        }                                                                                                              \
    } while (0);
#else
#define LCD_DEBUGF(format, ...)
#define LCD_ASSERT(x)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        esp_err_t err = (x);                                                                                           \
        if (unlikely(err != ESP_OK))                                                                                   \
            return err;                                                                                                \
    } while (0);
#endif

#ifdef LCD_THREAD_SAFE
#define LCD_MUTEX_PROTECTION(mutex, operation)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if (LCD_TAKE_MUTEX(mutex) == pdTRUE)                                                                           \
        {                                                                                                              \
            esp_err_t err = operation;                                                                                 \
            LCD_GIVE_MUTEX(mutex);                                                                                     \
            return err;                                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            return ESP_ERR_TIMEOUT;                                                                                    \
        }                                                                                                              \
    } while (0)

#else
#define LCD_MUTEX_PROTECTION(mutex, operation) return operation
#endif

#ifdef LCD_THREAD_SAFE
#define LCD_TAKE_MUTEX(x) xSemaphoreTake(x, LCD_THREAD_TIMEOUT)
#define LCD_GIVE_MUTEX(x) xSemaphoreGive(x)
#else
#define LCD_TAKE_MUTEX(x)
#define LCD_GIVE_MUTEX(x)
#endif

// line pointers
#define LCD_FIRST_LINE_START 0x00
#define LCD_SECOND_LINE_START 0x40
#define LCD_THIRD_LINE_START 0x14
#define LCD_FORTH_LINE_START 0x54
// lcd delays uS
#define LCD_CLR_DELAY 1530
#define LCD_COMMON_DELAY 43

// Basic Commands
#define LCD_CLR 0x01            // Clear Screen
#define LCD_INIT_4BIT 0x02      // Init LCD in 4bit mode
#define LCD_USE2_LINE_4BIT 0x28 // use 2 line and initialize 5*8 matrix in (4-bit mode)
#define LCD_CURSOR_OFF 0x0c     // display on cursor off
#define LCD_INC_CURSOR 0x06     // display on cursor off

esp_err_t lcd_instruction(lcd_type *lcd, uint8_t command, uint8_t char_flag)
{
    for (int i = 2; i < 6; i++)
    {
        gpio_set_level(nums[i], ((command >> (2 + i)) & 1));
    }
    gpio_set_level(LCD_PIN_RS, char_flag);
    gpio_set_level(LCD_PIN_EN, 1);
    ets_delay_us(1);
    gpio_set_level(LCD_PIN_EN, 0);
    ets_delay_us(1);

    for (int i = 2; i < 6; i++)
    {
        gpio_set_level(nums[i], ((command >> (i - 2)) & 1));
    }

    gpio_set_level(LCD_PIN_EN, 1);
    ets_delay_us(1);
    gpio_set_level(LCD_PIN_EN, 0);
    ets_delay_us(1);

    if (command == LCD_CLR)
        ets_delay_us(LCD_CLR_DELAY);
    else
        ets_delay_us(LCD_COMMON_DELAY);

    return ESP_OK;
}

esp_err_t lcd_command(lcd_type *lcd, uint8_t command)
{
    return lcd_instruction(lcd, command, false);
}

esp_err_t lcd_char(lcd_type *lcd, uint8_t character)
{
    return lcd_instruction(lcd, character, true);
}

esp_err_t lcd_init_unsafe(lcd_type *lcd)
{
    lcd->lines[LCD_LINE_0][0] = '\0';
    lcd->lines[LCD_LINE_1][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_0][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_1][0] = '\0';

    for (int i = 0; i < 6; i++)
    {
        gpio_config_t cfg = {.pin_bit_mask = 1 << nums[i],
                             .mode = GPIO_MODE_OUTPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&cfg);
    }

    ets_delay_us(LCD_CLR_DELAY);
    LCD_ASSERT(lcd_command(lcd, LCD_INIT_4BIT));
    LCD_ASSERT(lcd_command(lcd, LCD_USE2_LINE_4BIT));
    LCD_ASSERT(lcd_command(lcd, LCD_CLR));
    LCD_ASSERT(lcd_command(lcd, LCD_CURSOR_OFF));
    LCD_ASSERT(lcd_command(lcd, LCD_INC_CURSOR));

    return ESP_OK;
}

esp_err_t lcd_init(lcd_type *lcd)
{
    lcd->mutex_private = xSemaphoreCreateMutex();
    if (lcd->mutex_private == NULL)
        return ESP_ERR_NO_MEM;
    LCD_GIVE_MUTEX(lcd->mutex_private);
    LCD_MUTEX_PROTECTION(lcd->mutex_private, lcd_init_unsafe(lcd));
}

esp_err_t lcd_update_line(lcd_type *lcd, LCD_LINES_TYPE line)
{
    uint8_t location;
    switch (line)
    {
    case LCD_LINE_0:
        location = LCD_FIRST_LINE_START;
        break;
    case LCD_LINE_1:
        location = LCD_SECOND_LINE_START;
        break;
    case LCD_LINE_2:
        location = LCD_THIRD_LINE_START;
        break;
    case LCD_LINE_3:
        location = LCD_FORTH_LINE_START;
        break;
    default:
        location = LCD_FIRST_LINE_START;
        break;
    }
    location |= 0x80;
    LCD_ASSERT(lcd_command(lcd, location));
    char *msg = lcd->lines[line];
    char *old_msg = lcd->cur_lines_private[line];
    uint8_t equal = false;
    uint8_t size = 0;
    // Shows every letter on text and sets all others
    // if equal to the old msg, do not redraw, if not equal, go to correct position and draw.
    while (*msg && *old_msg && size < 20)
    {
        if (*msg == *old_msg)
        {
            equal = true;
        }
        else if (equal)
        {
            equal = false;
            LCD_ASSERT(lcd_command(lcd, location));
            LCD_ASSERT(lcd_char(lcd, *msg));
        }
        else
        {
            LCD_ASSERT(lcd_char(lcd, *msg));
        }

        location++;
        size++;
        msg++;
        old_msg++;
    }
    // preencha o resto da mensagem nova
    while (*msg && size < 20)
    {
        LCD_ASSERT(lcd_char(lcd, *msg));
        size++;
        msg++;
    }
    // preencha o resto com ' '
    while (size < 20)
    {
        LCD_ASSERT(lcd_char(lcd, ' '));
        size++;
    }

    memcpy(lcd->cur_lines_private[line], lcd->lines[line], sizeof(char[17]));
    return ESP_OK;
}

esp_err_t lcd_update_unsafe(lcd_type *lcd)
{
    LCD_ASSERT(lcd_update_line(lcd, LCD_LINE_0));
    LCD_ASSERT(lcd_update_line(lcd, LCD_LINE_1));
    LCD_ASSERT(lcd_update_line(lcd, LCD_LINE_2));
    LCD_ASSERT(lcd_update_line(lcd, LCD_LINE_3));
    LCD_DEBUGF("WRITTEN TO LCD LINE 0 FROM BUFFER: \"%-20s\"\n\r", lcd->cur_lines_private[0]);
    LCD_DEBUGF("WRITTEN TO LCD LINE 1 FROM BUFFER: \"%-20s\"\n\r", lcd->cur_lines_private[1]);
    LCD_DEBUGF("WRITTEN TO LCD LINE 2 FROM BUFFER: \"%-20s\"\n\r", lcd->cur_lines_private[2]);
    LCD_DEBUGF("WRITTEN TO LCD LINE 3 FROM BUFFER: \"%-20s\"\n\r", lcd->cur_lines_private[3]);
    LCD_DEBUGF("LCD UPDATED\n\r");
    return ESP_OK;
}

esp_err_t lcd_update(lcd_type *lcd)
{
    LCD_MUTEX_PROTECTION(lcd->mutex_private, lcd_update_unsafe(lcd));
}

esp_err_t lcd_write_unsafe(lcd_type *lcd, LCD_LINES_TYPE line, const char *text)
{

    memcpy(lcd->lines[line], text, sizeof(char[21]));
    LCD_DEBUGF("LCD LINE %d BUFFER CHANGED TO: \"%-20s\"\n\r", line, lcd->lines[line]);
    lcd->lines[line][20] = '\0';
    return ESP_OK;
}
esp_err_t lcd_write(lcd_type *lcd, LCD_LINES_TYPE line, const char *text)
{
    LCD_MUTEX_PROTECTION(lcd->mutex_private, lcd_write_unsafe(lcd, line, text));
}

esp_err_t lcd_clear_unsafe(lcd_type *lcd)
{

    lcd->lines[LCD_LINE_0][0] = '\0';
    lcd->lines[LCD_LINE_1][0] = '\0';
    lcd->lines[LCD_LINE_2][0] = '\0';
    lcd->lines[LCD_LINE_3][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_0][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_1][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_2][0] = '\0';
    lcd->cur_lines_private[LCD_LINE_3][0] = '\0';
    LCD_ASSERT(lcd_command(lcd, LCD_CLR));
    LCD_DEBUGF("LCD CLEARED\n\r");

    return ESP_OK;
}
esp_err_t lcd_clear(lcd_type *lcd)
{
    LCD_MUTEX_PROTECTION(lcd->mutex_private, lcd_clear_unsafe(lcd));
}
