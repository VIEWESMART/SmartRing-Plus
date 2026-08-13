#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <functional>

class PowerManager {
private:
    static constexpr struct {
        uint16_t adc;
        uint8_t level;
    } BATTERY_LEVELS[] = {{2150, 0}, {2450, 100}};
    static constexpr size_t BATTERY_LEVELS_COUNT = 2;
    static constexpr size_t ADC_VALUES_COUNT = 10;

    static constexpr uint16_t ADC_WARNING_THRESHOLD = 1924;   // 3.1V
    static constexpr uint16_t ADC_SHUTDOWN_THRESHOLD = 1861;  // 3.0V

    esp_timer_handle_t timer_handle_ = nullptr;
    gpio_num_t charging_pin_;
    adc_unit_t adc_unit_;
    adc_channel_t adc_channel_;
    uint16_t adc_values_[ADC_VALUES_COUNT];
    size_t adc_values_index_ = 0;
    size_t adc_values_count_ = 0;
    uint8_t battery_level_ = 100;
    bool is_charging_ = false;
    uint32_t average_adc_ = 0;
    bool low_battery_warning_sent_ = false;
    bool shutdown_triggered_ = false;

    adc_oneshot_unit_handle_t adc_handle_;

    std::function<void()> on_low_battery_warning_ = nullptr;
    std::function<void()> on_battery_shutdown_ = nullptr;
    std::function<void()> on_battery_recovered_ = nullptr;

    static bool rtc_shutdown_flag_;

    void CheckBatteryStatus() {
        if (charging_pin_ != GPIO_NUM_NC) {
            is_charging_ = gpio_get_level(charging_pin_) == 0;
        } else {
            is_charging_ = false;
        }
        ReadBatteryAdcData();

        if (is_charging_) {
            low_battery_warning_sent_ = false;
            shutdown_triggered_ = false;
            return;
        }

        if (!shutdown_triggered_ && average_adc_ <= ADC_SHUTDOWN_THRESHOLD) {
            shutdown_triggered_ = true;
            low_battery_warning_sent_ = true;
            rtc_shutdown_flag_ = true;
            ESP_LOGW("PowerManager", "Battery voltage low, triggering shutdown (ADC: %lu, voltage: %.2fV)",
                     (unsigned long)average_adc_, GetBatteryVoltage());
            if (on_battery_shutdown_) {
                on_battery_shutdown_();
            }
        } else if (!low_battery_warning_sent_ && !shutdown_triggered_ && average_adc_ <= ADC_WARNING_THRESHOLD) {
            low_battery_warning_sent_ = true;
            ESP_LOGW("PowerManager", "Battery voltage low, playing warning (ADC: %lu, voltage: %.2fV)",
                     (unsigned long)average_adc_, GetBatteryVoltage());
            if (on_low_battery_warning_) {
                on_low_battery_warning_();
            }
        } else if (average_adc_ > ADC_WARNING_THRESHOLD) {
            bool was_warning_sent = low_battery_warning_sent_ || shutdown_triggered_;
            low_battery_warning_sent_ = false;
            shutdown_triggered_ = false;
            rtc_shutdown_flag_ = false;
            if (was_warning_sent && on_battery_recovered_) {
                on_battery_recovered_();
            }
        }
    }

    void ReadBatteryAdcData() {
        int adc_value;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, adc_channel_, &adc_value));
        adc_values_[adc_values_index_] = adc_value;
        adc_values_index_ = (adc_values_index_ + 1) % ADC_VALUES_COUNT;
        if (adc_values_count_ < ADC_VALUES_COUNT) {
            adc_values_count_++;
        }
        average_adc_ = 0;
        for (size_t i = 0; i < adc_values_count_; i++) {
            average_adc_ += adc_values_[i];
        }
        average_adc_ /= adc_values_count_;
        CalculateBatteryLevel(average_adc_);
    }

    void CalculateBatteryLevel(uint32_t average_adc) {
        if (average_adc <= BATTERY_LEVELS[0].adc) {
            battery_level_ = 0;
        } else if (average_adc >= BATTERY_LEVELS[BATTERY_LEVELS_COUNT - 1].adc) {
            battery_level_ = 100;
        } else {
            float ratio = static_cast<float>(average_adc - BATTERY_LEVELS[0].adc) /
                          (BATTERY_LEVELS[1].adc - BATTERY_LEVELS[0].adc);
            battery_level_ = ratio * 100;
        }
    }

public:
    PowerManager(gpio_num_t charging_pin, adc_unit_t adc_unit = ADC_UNIT_2,
                 adc_channel_t adc_channel = ADC_CHANNEL_3)
        : charging_pin_(charging_pin), adc_unit_(adc_unit), adc_channel_(adc_channel) {
        if (charging_pin_ != GPIO_NUM_NC) {
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pin_bit_mask = (1ULL << charging_pin_);
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&io_conf);
        }

        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = adc_unit_,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));
        adc_oneshot_chan_cfg_t chan_config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, adc_channel_, &chan_config));

        if (rtc_shutdown_flag_) {
            vTaskDelay(pdMS_TO_TICKS(100));
            ReadBatteryAdcData();
            if (average_adc_ <= ADC_WARNING_THRESHOLD) {
                esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
                rtc_gpio_pullup_en(GPIO_NUM_0);
                rtc_gpio_pulldown_dis(GPIO_NUM_0);
                esp_deep_sleep_start();
            } else {
                rtc_shutdown_flag_ = false;
                shutdown_triggered_ = false;
                low_battery_warning_sent_ = false;
            }
        }

        esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) { static_cast<PowerManager*>(arg)->CheckBatteryStatus(); },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "battery_check_timer",
            .skip_unhandled_events = true,
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
        ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, 1000000));
    }

    ~PowerManager() {
        if (timer_handle_) {
            esp_timer_stop(timer_handle_);
            esp_timer_delete(timer_handle_);
        }
        if (adc_handle_) {
            adc_oneshot_del_unit(adc_handle_);
        }
    }

    bool IsCharging() { return is_charging_; }
    uint8_t GetBatteryLevel() { return battery_level_; }
    float GetBatteryVoltage() {
        float adc_voltage = (static_cast<float>(average_adc_) / 4095.0f) * 3.3f;
        return adc_voltage * 2.0f;
    }

    void SetLowBatteryWarningCallback(std::function<void()> callback) { on_low_battery_warning_ = callback; }
    void SetBatteryShutdownCallback(std::function<void()> callback) { on_battery_shutdown_ = callback; }
    void SetBatteryRecoveredCallback(std::function<void()> callback) { on_battery_recovered_ = callback; }
};

RTC_DATA_ATTR bool PowerManager::rtc_shutdown_flag_ = false;

#endif  // __POWER_MANAGER_H__
