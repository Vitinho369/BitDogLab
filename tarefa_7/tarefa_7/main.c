#include "pico/stdlib.h"

#include "client.h"

int main() {
    stdio_init_all();

    while (!connect_wifi("SSID", "PASSWORD")) {
        printf("Tentando novamente...\n");
    }

    ip_addr_t server_ip;

    resolve_name("api.thingspeak.com", &server_ip);
    
    printf("IP resolvido: %s\n", ipaddr_ntoa(&server_ip));

    struct tcp_pcb *pcb = tcp_new();

    client_create(pcb, &server_ip, 80);

    const char *request =
        "GET /update.json?api_key=APY_KEY&field1=1 HTTP/1.1\r\n"
        "Host: api.thingspeak.com\r\n"
        "Connection: close\r\n\r\n";

    client_write(pcb, request);

    client_close(pcb);

    while (true) {
        sleep_ms(1000);
    }
    return 0;
}