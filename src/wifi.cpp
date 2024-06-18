#include <wifi.hpp>

#include <stdexcept>

namespace wifi
{
void init(uint32_t country)
{
    if (cyw43_arch_init_with_country(country))
    {
        throw std::runtime_error("Could not init to Wifi");
    }
    cyw43_arch_enable_sta_mode();
}

void connect(const char *ssid, const char *pass, uint32_t auth, const uint32_t timeout)
{
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, auth, timeout))
    {
        throw std::runtime_error("Wifi connection timed out");
    }
}

bool is_connected()
{
    return cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_JOIN;
}
}
