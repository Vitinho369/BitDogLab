#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// HTTP server details
#define HTTP_SERVER "example.com"
#define HTTP_PORT 80
#define HTTP_PATH "/"

// TCP connection callback
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p == NULL)
  {
    // Connection closed by server
    printf("Connection closed by server\n");
    tcp_close(tpcb);
    return ERR_OK;
  }

  // Print received data
  printf("Received data:\n");
  while (p != NULL)
  {
    printf("%.*s", p->len, (char *)p->payload);
    p = p->next;
  }

  // Free the pbuf
  pbuf_free(p);
  return ERR_OK;
}

// TCP connection established callback
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  if (err != ERR_OK)
  {
    printf("Failed to establish connection\n");
    return err;
  }

  // Set receive callback
  tcp_recv(tpcb, tcp_recv_callback);

  // Send HTTP GET request
  char request[128];
  snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", HTTP_PATH, HTTP_SERVER);
  err = tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
  if (err != ERR_OK)
  {
    printf("Failed to send HTTP request\n");
    return err;
  }

  // Flush the request
  tcp_output(tpcb);
  printf("HTTP GET request sent\n");
  return ERR_OK;
}

static void dns_resolve_callback(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
  if (ipaddr == NULL) {
    return;
  }

  struct tcp_pcb *pcb = tcp_new();

  if (!pcb) {
    return;
  }

  tcp_arg(pcb, NULL);
  tcp_connect(pcb, ipaddr, HTTP_PORT, tcp_connected_callback);
}

// Main function
int main()
{
  stdio_init_all();

  // Initialize WiFi
  if (cyw43_arch_init())
  {
    printf("Failed to initialize CYW43\n");
    return 1;
  }
  cyw43_arch_enable_sta_mode();

  // Connect to WiFi
  printf("Connecting to WiFi...\n");
  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
  {
    printf("Failed to connect to WiFi\n");
    return 1;
  }
  printf("Connected to WiFi\n");

  // Resolve DNS and start HTTP request
  ip_addr_t server_ip;
  err_t err = dns_gethostbyname(HTTP_SERVER, &server_ip, dns_resolve_callback, NULL);
  if (err == ERR_OK)
  {
    // IP address already resolved
    dns_resolve_callback(HTTP_SERVER, &server_ip, NULL);
  }
  else if (err != ERR_INPROGRESS)
  {
    printf("Failed to start DNS resolution\n");
    return 1;
  }

  // Main loop
  while (true)
  {
    tight_loop_contents();
  }

  // Cleanup
  cyw43_arch_deinit();
  return 0;
}