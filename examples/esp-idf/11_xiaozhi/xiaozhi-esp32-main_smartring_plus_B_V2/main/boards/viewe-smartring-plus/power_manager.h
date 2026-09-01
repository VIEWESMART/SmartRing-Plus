#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__

#include <esp_log.h>
#include <esp_timer.h>
#include <functional>

// 基于 AXP2101 电量百分比的低电监测（不用 ADC）
class PowerManager {
public:
    using GetBatteryFn = std::function<bool(int& level, bool& charging, bool& discharging)>;

private:
    static constexpr const char* TAG = "PowerManager";
    static constexpr int kWarningPercent = 15;
    static constexpr int kShutdownPercent = 5;
    static constexpr int kShutdownConfirmCount = 3;  // 连续确认次数，防抖
    static constexpr int64_t kCheckIntervalUs = 1000000;  // 1s

    GetBatteryFn get_battery_;
    esp_timer_handle_t timer_handle_ = nullptr;

    bool low_battery_warning_sent_ = false;
    bool shutdown_triggered_ = false;
    int shutdown_confirm_count_ = 0;

    std::function<void()> on_low_battery_warning_ = nullptr;
    std::function<void()> on_battery_shutdown_ = nullptr;
    std::function<void()> on_battery_recovered_ = nullptr;

    void CheckBatteryStatus() {
        if (!get_battery_) {
            return;
        }

        int level = 0;
        bool charging = false;
        bool discharging = false;
        if (!get_battery_(level, charging, discharging)) {
            return;  // 读失败不累计确认，避免误关
        }

        // 充电中：清标志，不关机
        if (charging) {
            bool was_warned = low_battery_warning_sent_ || shutdown_triggered_;
            low_battery_warning_sent_ = false;
            shutdown_triggered_ = false;
            shutdown_confirm_count_ = 0;
            if (was_warned && on_battery_recovered_) {
                on_battery_recovered_();
            }
            return;
        }

        // 放电或待机：按电量判断
        if (!shutdown_triggered_ && level <= kShutdownPercent) {
            if (!low_battery_warning_sent_) {
                low_battery_warning_sent_ = true;
                ESP_LOGW(TAG, "Battery low warning (level=%d%%)", level);
                if (on_low_battery_warning_) {
                    on_low_battery_warning_();
                }
            }
            shutdown_confirm_count_++;
            ESP_LOGW(TAG, "Low battery confirm %d/%d (level=%d%%)",
                     shutdown_confirm_count_, kShutdownConfirmCount, level);
            if (shutdown_confirm_count_ >= kShutdownConfirmCount) {
                shutdown_triggered_ = true;
                ESP_LOGW(TAG, "Battery critically low (%d%%), triggering shutdown", level);
                if (on_battery_shutdown_) {
                    on_battery_shutdown_();
                }
            }
            return;
        }

        shutdown_confirm_count_ = 0;

        if (!low_battery_warning_sent_ && !shutdown_triggered_ && level <= kWarningPercent) {
            low_battery_warning_sent_ = true;
            ESP_LOGW(TAG, "Battery low warning (level=%d%%)", level);
            if (on_low_battery_warning_) {
                on_low_battery_warning_();
            }
        } else if (level > kWarningPercent) {
            bool was_warned = low_battery_warning_sent_ || shutdown_triggered_;
            low_battery_warning_sent_ = false;
            shutdown_triggered_ = false;
            if (was_warned && on_battery_recovered_) {
                on_battery_recovered_();
            }
        }
    }

public:
    explicit PowerManager(GetBatteryFn get_battery) : get_battery_(std::move(get_battery)) {
        esp_timer_create_args_t timer_args = {
            .callback = [](void* arg) { static_cast<PowerManager*>(arg)->CheckBatteryStatus(); },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "axp_battery_check",
            .skip_unhandled_events = true,
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
        ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, kCheckIntervalUs));
        ESP_LOGI(TAG, "AXP battery monitor started (warn<=%d%%, shutdown<=%d%% x%d)",
                 kWarningPercent, kShutdownPercent, kShutdownConfirmCount);
    }

    ~PowerManager() {
        if (timer_handle_) {
            esp_timer_stop(timer_handle_);
            esp_timer_delete(timer_handle_);
            timer_handle_ = nullptr;
        }
    }

    void SetLowBatteryWarningCallback(std::function<void()> callback) {
        on_low_battery_warning_ = std::move(callback);
    }
    void SetBatteryShutdownCallback(std::function<void()> callback) {
        on_battery_shutdown_ = std::move(callback);
    }
    void SetBatteryRecoveredCallback(std::function<void()> callback) {
        on_battery_recovered_ = std::move(callback);
    }
};

#endif  // __POWER_MANAGER_H__
