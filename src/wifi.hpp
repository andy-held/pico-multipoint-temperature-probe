#pragma once

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

namespace wifi
{
void init(uint32_t country);

void connect(const char *ssid, const char *pass, uint32_t auth = CYW43_AUTH_WPA2_AES_PSK, const uint32_t timeout = 10000);

bool is_connected();
}
