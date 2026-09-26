#include <mqtt_client.hpp>

#include <lwip/dns.h>
#include <lwip/apps/mqtt.h>

#include <stdexcept>
#include <string_view>
#include <stdio.h>
#include <string.h>

namespace
{
struct MQTT_Connection_Status
{
    int status = -1;

    explicit operator std::string_view() const
    {
        switch(status)
        {
        case MQTT_CONNECT_ACCEPTED:
            return "Connected";
        case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION:
            return "Refused protocol version";
        case MQTT_CONNECT_REFUSED_IDENTIFIER:
            return "Refused identifier";
        case MQTT_CONNECT_REFUSED_SERVER:
            return "Refused server";
        case MQTT_CONNECT_REFUSED_USERNAME_PASS:
            return "Refused username/password";
        case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_:
            return "Refused not authorized";
        case MQTT_CONNECT_DISCONNECTED:
            return "Disconnected";
        case MQTT_CONNECT_TIMEOUT:
            return "Timeout";
        default:
            return "Unknown error";
        }
    }
};

struct MQTT_Publish_Status
{
    volatile err_t error = ERR_OK;
    volatile bool published = false;
};

struct DNS_Query_Status
{
    ip_addr_t ip;
    bool resolved = false;
    bool failed = false;
};
}

template<>
std::tuple<const void*, uint32_t> get_data_view<std::string>(const std::string& data)
{
    return {static_cast<const void*>(data.data()), sizeof(data.length())};
}

ip_addr_t run_dns_lookup(const char* hostname)
{
    printf("Running DNS query for %s.\n", hostname);

    auto dns_gethostbyname_cb = [](const char* /*name*/, const ip_addr_t *ipaddr, void *callback_arg)
    {
        auto& query_status = *static_cast<DNS_Query_Status*>(callback_arg);
        query_status.resolved = true;
        if(ipaddr)
        {
            query_status.ip = *ipaddr;
        }
        else
        {
            query_status.failed = true;
        }
    };

    DNS_Query_Status query_status;
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname, &query_status.ip, dns_gethostbyname_cb, &query_status);
    cyw43_arch_lwip_end();

    if (err == ERR_INPROGRESS)
    {
        while (!query_status.resolved) // wait until the callback was called
        {
            sleep_ms(5);
        }
    }

    if ((err != ERR_OK && err != ERR_INPROGRESS) || query_status.failed)
    {
        throw std::runtime_error("DNS lookup failed.");
    }

    printf("DNS query finished with resolved addr of %s.\n", ip4addr_ntoa(&query_status.ip));
    return query_status.ip;
}

mqtt_client::mqtt_client(const char* client_id, ip_addr_t remote_addr_in, const uint32_t port, const char* user, const char* pass):
    remote_addr(remote_addr_in)
{
    cyw43_arch_lwip_begin();
    lwip_mqtt_client = mqtt_client_new();
    cyw43_arch_lwip_end();
    if (!lwip_mqtt_client)
    {
        throw std::runtime_error("Could not allocate MQTT client");
    }
    struct mqtt_connect_client_info_t ci;
    err_t err;

    memset(&ci, 0, sizeof(ci));

    ci.client_id = client_id;
    ci.client_user = user;
    ci.client_pass = pass;
    ci.keep_alive = 30;
    ci.will_topic = NULL;

    auto connection_cb = [](mqtt_client_t* /*client*/, void* arg, mqtt_connection_status_t status)
    {
        *static_cast<volatile int*>(arg) = status;
    };

    cyw43_arch_lwip_begin();
    err = mqtt_client_connect(lwip_mqtt_client, &remote_addr, port, connection_cb, const_cast<int*>(&connection_status), &ci);
    cyw43_arch_lwip_end();

    if (err != ERR_OK)
    {
        close();
        throw std::runtime_error(std::string("mqtt_connect returned ") + std::to_string(err));
    }

    while(!is_connected())
    {
        if(connection_status > 0)
        {
            MQTT_Connection_Status status{connection_status};
            close();
            throw std::runtime_error(std::string("MQTT connection failed: ") + std::string(std::string_view(status)));
        }
        sleep_ms(5);
    }


    printf("MQTT connected.\n");
}

mqtt_client::mqtt_client(const char* client_id, const char* hostname, const uint32_t port, const char* user, const char* pass):
    mqtt_client(
        client_id,
        run_dns_lookup(hostname),
        port,
        user,
        pass
    )
{
}

mqtt_client::~mqtt_client()
{
    close();
}

void mqtt_client::close()
{
    if (lwip_mqtt_client)
    {
        cyw43_arch_lwip_begin();
        mqtt_disconnect(lwip_mqtt_client);
        mqtt_client_free(lwip_mqtt_client);
        cyw43_arch_lwip_end();
        lwip_mqtt_client = nullptr;
    }
}

void mqtt_client::publish(const char* topic, const void *data, uint32_t data_len)
{
    auto pub_request_cb = [](void *callback_arg, err_t err)
    {
        auto& status = *static_cast<MQTT_Publish_Status*>(callback_arg);
        status.published = true;
        status.error = err;
    };
    constexpr const u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
    constexpr const u8_t retain = 0;
    cyw43_arch_lwip_begin();
    MQTT_Publish_Status status;
    auto err = mqtt_publish(lwip_mqtt_client, topic, data, data_len, qos, retain, pub_request_cb, &status);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        throw std::runtime_error("MQTT calling publish returned error: " + std::to_string(err));
    }

    const auto deadline = make_timeout_time_ms(10000);
    while(!status.published)
    {
        if (!is_connected() || time_reached(deadline))
        {
            cyw43_arch_lwip_begin();
            mqtt_disconnect(lwip_mqtt_client);
            cyw43_arch_lwip_end();
            throw std::runtime_error("Timeout waiting for MQTT publish MQTT");
        }
        sleep_ms(5);
    }
    if(status.error != ERR_OK)
    {
        throw std::runtime_error("MQTT publish failed: " + std::to_string(status.error));
    }
}

bool mqtt_client::is_connected()
{
    cyw43_arch_lwip_begin();
    const bool connected = mqtt_client_is_connected(lwip_mqtt_client);
    cyw43_arch_lwip_end();
    return connected;
}
