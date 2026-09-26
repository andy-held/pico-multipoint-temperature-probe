#pragma once

#include <tuple>
#include <string>

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

typedef struct mqtt_client_s mqtt_client_t;

template<typename T>
std::tuple<const void*, uint32_t> get_data_view(const T& data)
{
    return {static_cast<const void*>(&data), sizeof(T)};
}

template<>
std::tuple<const void*, uint32_t> get_data_view<std::string>(const std::string& data);

struct mqtt_client
{
    mqtt_client(const char* client_id, ip_addr_t remote_addr, const uint32_t port = 1883, const char* user = nullptr, const char* pass = nullptr);
    mqtt_client(const char* client_id, const char* hostname, const uint32_t port, const char* user = nullptr, const char* pass = nullptr);
    ~mqtt_client();

    mqtt_client(const mqtt_client&) = delete;
    mqtt_client& operator=(const mqtt_client&) = delete;

    void publish(const char* topic, const void* data, uint32_t data_len);

    template<typename T>
    void publish(const char* topic, const T& data)
    {
        auto [ptr, len] = get_data_view(data);
        return publish(topic, ptr, len);
    }

    bool is_connected();

private:
    void close();

    ip_addr_t remote_addr;
    mqtt_client_t* lwip_mqtt_client = nullptr;
    volatile int connection_status = -1;
};
