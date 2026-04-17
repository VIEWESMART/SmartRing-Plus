#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/spi_master.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_INPUT_REFERENCE    true

#define POWER_CHARGE_DETECT_PIN GPIO_NUM_NC
#define POWER_ADC_UNIT ADC_UNIT_1
#define POWER_ADC_CHANNEL ADC_CHANNEL_0  // GPIO1 corresponds to ADC_CHANNEL_0

// 硬件关机控制引脚：拉高3秒以上实现硬件关机，拉低为开机状态
#define POWER_OFF_PIN GPIO_NUM_47

#define POWER_CTRL  GPIO_NUM_9
#define LED_G       GPIO_NUM_43
#define SD_MISO     GPIO_NUM_NC
#define SD_SCK      GPIO_NUM_NC
#define SD_MOSI     GPIO_NUM_NC

// I2S pins according to schematic
#define AUDIO_I2S_GPIO_MCLK     GPIO_NUM_48
#define AUDIO_I2S_GPIO_WS       GPIO_NUM_17
#define AUDIO_I2S_GPIO_BCLK     GPIO_NUM_21
#define AUDIO_I2S_GPIO_DIN      GPIO_NUM_18
#define AUDIO_I2S_GPIO_DOUT     GPIO_NUM_16

// Audio codec pins - I2C shared with touch panel
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_45
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_8
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_9
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR
#define AUDIO_CODEC_ES7210_ADDR  0x82

#define BUILTIN_LED_GPIO        GPIO_NUM_NC
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

#define DISPLAY_WIDTH       360
#define DISPLAY_HEIGHT      360
#define DISPLAY_MIRROR_X    false
#define DISPLAY_MIRROR_Y    false
#define DISPLAY_SWAP_XY     false

#define QSPI_LCD_H_RES           (360)
#define QSPI_LCD_V_RES           (360)
#define QSPI_LCD_BIT_PER_PIXEL   (16)

#define QSPI_LCD_HOST           SPI2_HOST
#define QSPI_PIN_NUM_LCD_PCLK   GPIO_NUM_10
#define QSPI_PIN_NUM_LCD_CS     GPIO_NUM_11
#define QSPI_PIN_NUM_LCD_DATA0  GPIO_NUM_12
#define QSPI_PIN_NUM_LCD_DATA1  GPIO_NUM_13
#define QSPI_PIN_NUM_LCD_DATA2  GPIO_NUM_15
#define QSPI_PIN_NUM_LCD_DATA3  GPIO_NUM_14
#define QSPI_PIN_NUM_LCD_TE     GPIO_NUM_NC
#define QSPI_PIN_NUM_LCD_RST    GPIO_NUM_39
#define QSPI_PIN_NUM_LCD_BL     GPIO_NUM_46

#define UART1_TX_1     GPIO_NUM_6
#define UART1_TX_2     GPIO_NUM_5
#define UART1_RX_1     GPIO_NUM_5
#define UART1_RX_2     GPIO_NUM_4
#define TOUCH_PAD2_1   GPIO_NUM_NC
#define TOUCH_PAD2_2   GPIO_NUM_6
#define TOUCH_PAD1     GPIO_NUM_7

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

// Touch panel pins
#define TP_PORT          (I2C_NUM_1)
#define TP_PIN_NUM_SDA   (GPIO_NUM_8)
#define TP_PIN_NUM_SCL   (GPIO_NUM_9)
#define TP_PIN_NUM_RST   (GPIO_NUM_40)
#define TP_PIN_NUM_INT   (GPIO_NUM_41)

#define DISPLAY_BACKLIGHT_PIN           QSPI_PIN_NUM_LCD_BL
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT true

#define TAIJIPI_ST77916_PANEL_BUS_QSPI_CONFIG(sclk, d0, d1, d2, d3, max_trans_sz) \
    {                                                                             \
        .data0_io_num = d0,                                                       \
        .data1_io_num = d1,                                                       \
        .sclk_io_num = sclk,                                                      \
        .data2_io_num = d2,                                                       \
        .data3_io_num = d3,                                                       \
        .max_transfer_sz = max_trans_sz,                                          \
    }

#endif // _BOARD_CONFIG_H_
