#include <pico/time.h>
#include <watchdog.hpp>

#include <hardware/watchdog.h>

#include <stdio.h>

namespace watchdog
{
namespace
{
    uint32_t timeout_ms = 60 * 1000;
    constexpr uint32_t watchdog_enable_timeout_ms = 8000;
    // Main and the timer callback run on core 0; aligned 32-bit accesses are atomic.
    volatile uint32_t last_fed_ms = 0;
    repeating_timer_t watchdog_timer;
}// namespace

void init(const uint32_t timeout_seconds_in)
{
    timeout_ms = timeout_seconds_in * 1000;
    if (watchdog_caused_reboot()) { printf("Restarted by watchdog\n"); }

    repeating_timer_callback_t eat = [](repeating_timer_t*)
    {
        const uint32_t now = to_ms_since_boot(get_absolute_time());
        const uint32_t last = last_fed_ms;
        if (now - last < timeout_ms) { watchdog_update(); }
        return true;
    };

    feed();
    watchdog_enable(watchdog_enable_timeout_ms, true);
    if (!add_repeating_timer_ms(1000, eat, nullptr, &watchdog_timer))
    {
        printf("Could not start watchdog feed timer\n");
        while (true) { tight_loop_contents(); }
    }
}

void feed() { last_fed_ms = to_ms_since_boot(get_absolute_time()); }
}// namespace watchdog
