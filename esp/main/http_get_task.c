#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"       
#include "sdkconfig.h"
#include <stdbool.h>

#define WEB_SERVER "example.com"
#define WEB_PORT 80

extern bool wifi_connected;  

void http_get_task(void *pvParameter)
{
    // Czekamy na połączenie WiFi
    while (!wifi_connected) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    char request[256];
    int sock;
    struct sockaddr_in dest_addr;
    char rx_buffer[1024];

    struct hostent *he = gethostbyname(WEB_SERVER);
    if (he == NULL) {
        ESP_LOGE("HTTP", "DNS lookup failed for %s", WEB_SERVER);
        vTaskDelete(NULL);
        return;
    }

    dest_addr.sin_addr = *(struct in_addr *)he->h_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(WEB_PORT);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE("HTTP", "Unable to create socket");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI("HTTP", "Connecting to %s:%d", WEB_SERVER, WEB_PORT);
    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        ESP_LOGE("HTTP", "Socket connect failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    snprintf(request, sizeof(request),
             "GET / HTTP/1.0\r\nHost: %s\r\n\r\n", WEB_SERVER);

    send(sock, request, strlen(request), 0);

    int len;
    while ((len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0)) > 0) {
        rx_buffer[len] = 0; 
        printf("%s", rx_buffer);
    }

    ESP_LOGI("HTTP", "Done receiving data");

    close(sock);
    vTaskDelete(NULL);
}
