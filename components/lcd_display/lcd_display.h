#pragma once
#include <driver/gpio.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#define LCD_THREAD_SAFE
// change if TIMEOUT is wanted to access lcd, set to portMAX_DELAY to wait forever
#define LCD_THREAD_TIMEOUT portMAX_DELAY

// comment to remove debug statements
// #define LCD_DEBUG

#define LCD_PIN_RS GPIO_NUM_2
#define LCD_PIN_EN GPIO_NUM_3
#define LCD_PIN_D4 GPIO_NUM_4
#define LCD_PIN_D5 GPIO_NUM_5
#define LCD_PIN_D6 GPIO_NUM_6
#define LCD_PIN_D7 GPIO_NUM_7

/*LCD Abstraction struct*/
typedef struct lcd_type
{
    // lines can be edited directly, and updated via the lcd_update command
    char lines[4][21];             // both lines of the display to be written
    char cur_lines_private[4][21]; // lines currently written on display

    SemaphoreHandle_t mutex_private;

} lcd_type;

typedef enum LCD_LINES_TYPE
{
    LCD_LINE_0 = 0,
    LCD_LINE_1 = 1,
    LCD_LINE_2 = 2,
    LCD_LINE_3 = 3
} LCD_LINES_TYPE;

#ifdef __cplusplus
extern "C"
{
#endif
    /*
    @brief Initializes lcd_type struct, abstraction to the lcd display
    @param lcd lcd abstraction struct to be initialized
    */
    esp_err_t lcd_init(lcd_type *lcd);
    /*
    @brief Writes text on buffer of a given line
    @param lcd lcd abstraction struct
    @param line line to be written on
    @param text pointer to the text to be written
    @attention MUST USE lcd_update after to changes to take effect
    */
    esp_err_t lcd_write(lcd_type *lcd, LCD_LINES_TYPE line, const char *text);
    /*
    @brief Updates text, can be used after changing text directly via .lines or after lcd_write
    @param lcd lcd abstraction struct
    */
    esp_err_t lcd_update(lcd_type *lcd);
    /*
    @brief Clears text,
    @param lcd lcd abstraction struct
    */
    esp_err_t lcd_clear(lcd_type *lcd);
#ifdef __cplusplus
}

#endif
