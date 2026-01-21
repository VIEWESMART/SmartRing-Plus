/*
 * @file smartring_plus.h
 * @author Ayang
 * @date 2026-01
 * @brief ESP BSP: SmartRing_Plus
 */

#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "bsp/display.h"
/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/

#define BSP_CAPS_DISPLAY        1
#define BSP_CAPS_TOUCH          1
#define BSP_CAPS_BUTTONS        1
#define BSP_CAPS_AUDIO          1
#define BSP_CAPS_AUDIO_SPEAKER  1
#define BSP_CAPS_AUDIO_MIC      1
#define BSP_CAPS_IMU            1
#define PCB_STATE               1
/**************************************************************************************************
 *  ESP-BOX pinout
 **************************************************************************************************/
/* I2C */
#if PCB_STATE
#define BSP_I2C_SCL           (GPIO_NUM_9)   // TP_SCL (shared with audio codec I2C)
#define BSP_I2C_SDA           (GPIO_NUM_8)   // TP_SDA (shared with audio codec I2C)

/* Audio */
#define BSP_I2S_SCLK          (GPIO_NUM_39) // BCLK
#define BSP_I2S_MCLK          (GPIO_NUM_NC)
#define BSP_I2S_LCLK          (GPIO_NUM_42) // WS
#define BSP_I2S_DOUT          (GPIO_NUM_41)     // To Codec ES8311
#define BSP_I2S_DSIN          (GPIO_NUM_40)    // From ADC ES7210
#define BSP_POWER_AMP_IO      (GPIO_NUM_45)

/* Display */
#define BSP_LCD_DATA3         (GPIO_NUM_14)  // LCD_QSPI_D3
#define BSP_LCD_DATA2         (GPIO_NUM_15)  // LCD_QSPI_D2
#define BSP_LCD_DATA1         (GPIO_NUM_13)  // LCD_QSPI_D1
#define BSP_LCD_DATA0         (GPIO_NUM_12)  // LCD_QSPI_D0
#define BSP_LCD_PCLK          (GPIO_NUM_10)  // LCD_QSPI_SCL
#define BSP_LCD_CS            (GPIO_NUM_11)  // LCD_QSPI_CS
#define BSP_LCD_DC            (GPIO_NUM_17)  // Keep DC pin (not mentioned in new config)
#define BSP_LCD_RST           (GPIO_NUM_39)  // LCD_RST
#define LCD_BACKLIIGHT_CHANNEL LEDC_CHANNEL_1
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_46)  // LCD_BLK
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_41)  // TP_INT
#define BSP_LCD_TOUCH_RST     (GPIO_NUM_40)  // TP_RST

/* Buttons */
#define BSP_BUTTON            (GPIO_NUM_2)
#define BSP_BUTTON_CTRL       (GPIO_NUM_1)

/* Power */
#define BSP_POWER_OFF         (GPIO_NUM_5)
#define BSP_POWER_VOUT_EN     (GPIO_NUM_11)
#define PMS_BTN_DISABLE       1


/* Vibration */
#define BSP_VIBRATION_PIN     (GPIO_NUM_38)
#define PWM_CHANNEL           LEDC_CHANNEL_0
#define PWM_FREQ_HZ           2000
#define PWM_RESOLUTION        LEDC_TIMER_10_BIT

/* LED */
#define BSP_LEDR_GPIO          (GPIO_NUM_NC)
#define BSP_LEDG_GPIO          (GPIO_NUM_NC)
#define BSP_LEDB_GPIO          (GPIO_NUM_NC)


#else
#define BSP_I2C_SCL           (GPIO_NUM_13)
#define BSP_I2C_SDA           (GPIO_NUM_14)

/* Audio */
#define BSP_I2S_SCLK          (U0TXD_GPIO_NUM) // BCLK
#define BSP_I2S_MCLK          (GPIO_NUM_NC)
#define BSP_I2S_LCLK          (U0RXD_GPIO_NUM) // WS
#define BSP_I2S_DOUT          (GPIO_NUM_9)     // To Codec ES8311
#define BSP_I2S_DSIN          (GPIO_NUM_21)    // From ADC ES7210
#define BSP_POWER_AMP_IO      (GPIO_NUM_10)

/* Display */
#define BSP_LCD_DATA3         (GPIO_NUM_40)
#define BSP_LCD_DATA2         (GPIO_NUM_42)
#define BSP_LCD_DATA1         (GPIO_NUM_39)
#define BSP_LCD_DATA0         (GPIO_NUM_41)
#define BSP_LCD_PCLK          (GPIO_NUM_45)
#define BSP_LCD_CS            (GPIO_NUM_38)
#define BSP_LCD_DC            (GPIO_NUM_46)
#define BSP_LCD_RST           (GPIO_NUM_47)

#define BSP_LCD_BACKLIGHT     (GPIO_NUM_17)
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_8)

/* Buttons */
#define BSP_UP_BUTTON         (GPIO_NUM_7)
#define BSP_DOWN_BUTTON       (GPIO_NUM_6)

/* Power */
#define BSP_POWER_VCC_EN      (GPIO_NUM_10)
#define BSP_POWER_VOUT_EN     (GPIO_NUM_18)
#define BSP_POWER_SLEEP_EN    (GPIO_NUM_0)

/* Vibration */
#define BSP_VIBRATION_PIN     (GPIO_NUM_48)
#define PWM_CHANNEL           LEDC_CHANNEL_0
#define PWM_FREQ_HZ           2000
#define PWM_RESOLUTION        LEDC_TIMER_10_BIT

/* IR Learning */
#define BSP_IR_TX_GPIO        (GPIO_NUM_2)

/* LED */
#define BSP_LED_GPIO          (GPIO_NUM_11)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BSP display configuration structure
 *
 */
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;  /*!< LVGL port configuration */
    uint32_t        buffer_size;    /*!< Size of the buffer for the screen in pixels */
    bool            double_buffer;  /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
    } flags;
} bsp_display_cfg_t;


/**************************************************************************************************
 *
 * I2C interface
 *
 * There are multiple devices connected to I2C peripheral:
 *  - Codec ES8311 (configuration only)
 *  - ADC ES7210 (configuration only)
 *  - Encryption chip ATECC608A (NOT populated on most boards)
 *  - LCD Touch controller
 *  - Inertial Measurement Unit ICM-42607-P
 *
 * After initialization of I2C, use BSP_I2C_NUM macro when creating I2C devices drivers ie.:
 * \code{.c}
 * icm42670_handle_t imu = icm42670_create(BSP_I2C_NUM, ICM42670_I2C_ADDRESS);
 * \endcode
 **************************************************************************************************/
#define BSP_I2C_NUM     (I2C_NUM_0)

/**
 * @brief Init I2C driver
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *      - ESP_FAIL              I2C driver installation error
 *
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief Deinit I2C driver and free its resources
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *
 */
esp_err_t bsp_i2c_deinit(void);


/**************************************************************************************************
 *
 * LCD interface
 *
 * ESP-BOX is shipped with 2.4inch ST7789 display controller.
 * It features 16-bit colors, 320x240 resolution and capacitive touch controller.
 *
 * LVGL is used as graphics library. LVGL is NOT thread safe, therefore the user must take LVGL mutex
 * by calling bsp_display_lock() before calling and LVGL API (lv_...) and then give the mutex with
 * bsp_display_unlock().
 *
 * Display's backlight must be enabled explicitly by calling bsp_display_backlight_on()
 **************************************************************************************************/
#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM            (SPI2_HOST)

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_disp_t *bsp_display_start(void);

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @param cfg display configuration
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_disp_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);

/**
 * @brief Get pointer to input device (touch, buttons, ...)
 *
 * @note The LVGL input device is initialized in bsp_display_start() function.
 *
 * @return Pointer to LVGL input device or NULL when not initialized
 */
lv_indev_t *bsp_display_get_input_dev(void);

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return true  Mutex was taken
 * @return false Mutex was NOT taken
 */
bool bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 *
 */
void bsp_display_unlock(void);

/**
 * @brief Set display enter sleep mode
 *
 * All the display (LCD, backlight, touch) will enter sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_enter_sleep(void);

/**
 * @brief Set display exit sleep mode
 *
 * All the display (LCD, backlight, touch) will exit sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_exit_sleep(void);

/**
 * @brief Rotate screen
 *
 * Display must be already initialized by calling bsp_display_start()
 *
 * @param[in] disp Pointer to LVGL display
 * @param[in] rotation Angle of the display rotation
 */
void bsp_display_rotate(lv_disp_t *disp, lv_disp_rot_t rotation);

#ifdef __cplusplus
}
#endif
