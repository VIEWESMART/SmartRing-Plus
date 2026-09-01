#pragma once

#include <cstdint>
#include <functional>

#include <driver/gpio.h>
#include <esp_timer.h>


class Backlight {
public:
    Backlight();
    ~Backlight();

    void RestoreBrightness();
    void SetBrightness(uint8_t brightness, bool permanent = false);
    inline uint8_t brightness() const { return brightness_; }

protected:
    void OnTransitionTimer();
    virtual void SetBrightnessImpl(uint8_t brightness) = 0;

    esp_timer_handle_t transition_timer_ = nullptr;
    uint8_t brightness_ = 0;
    uint8_t target_brightness_ = 0;
    uint8_t step_ = 1;
};


class PwmBacklight : public Backlight {
public:
    /** @param brightness_invert true if hardware is "high duty = dimmer" (invert logic so AI "调低" = darker) */
    PwmBacklight(gpio_num_t pin, bool output_invert = false, uint32_t freq_hz = 25000, bool brightness_invert = false);
    ~PwmBacklight();

    void SetBrightnessImpl(uint8_t brightness) override;

private:
    bool brightness_invert_ = false;
};
