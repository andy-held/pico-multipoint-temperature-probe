#pragma once

#include <pico/stdlib.h>

namespace watchdog
{
void init(uint32_t timeout_seconds);

void feed();
}// namespace watchdog
