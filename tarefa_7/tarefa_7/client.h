#pragma once

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"

// tenta conectar ao wifi
bool connect_wifi(const char *ssid, const char *password);

// resolve o ip via dns
void resolve_name(const char *name, ip_addr_t *ip_addr);

// instancia um cliente
bool client_create(struct tcp_pcb *pcb, ip_addr_t *ip_addr, unsigned short port);

// fecha a instancia de um cliente
void client_close(struct tcp_pcb *pcb);

// escreve em um cliente
bool client_write(struct tcp_pcb *pcb, const char *data);