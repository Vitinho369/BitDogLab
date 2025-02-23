#include "client.h"

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "cyw43.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

// Definição do timeout para resolução DNS e operações TCP (em milissegundos)
#define DNS_TIMEOUT_MS 5000
#define TCP_CONNECT_TIMEOUT_MS 5000
#define TCP_WRITE_TIMEOUT_MS 5000

//----------------------------------------------------------------------
// Função: connect_wifi
// Conecta-se à rede Wi-Fi especificada.
// Retorna true em caso de sucesso, false em caso de falha.
bool connect_wifi(const char *ssid, const char *password)
{
    if (cyw43_arch_init())
    {
        printf("Falha ao inicializar cyw43_arch\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();

    printf("Tentando conectar à rede Wi-Fi: %s...\n", ssid);
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        cyw43_arch_deinit();
        return false;
    }
    printf("Conectado ao Wi-Fi com sucesso\n");
    return true;
}

//----------------------------------------------------------------------
// Função: hostname_found (callback DNS)
// Callback para a função dns_gethostbyname.
// Armazena o endereço IP resolvido em *arg (ponteiro para ip_addr_t).
static void hostname_found(const char *name, const ip_addr_t *resolved, void *arg)
{
    ip_addr_t *result = (ip_addr_t *)arg;
    if (resolved)
    {
        *result = *resolved;
    }
    else
    {
        result->addr = IPADDR_NONE; // Define como IPADDR_NONE em caso de falha na resolução
    }
}

//----------------------------------------------------------------------
// Função: resolve_name
// Resolve um hostname para um endereço IP.
// Utiliza DNS e aguarda o resultado ou timeout.
void resolve_name(const char *name, ip_addr_t *ip_addr)
{
    ip_addr->addr = IPADDR_ANY; // Inicializa para indicar que a resolução está pendente

    printf("Resolvendo hostname: %s...\n", name);
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(name, ip_addr, hostname_found, ip_addr);
    cyw43_arch_lwip_end();

    if (err == ERR_INPROGRESS)
    {
        uint32_t start_time = to_ms_since_boot(get_absolute_time());
        while (ip_addr->addr == IPADDR_ANY)
        {
            sleep_ms(1);
            if (to_ms_since_boot(get_absolute_time()) - start_time > DNS_TIMEOUT_MS)
            {
                printf("Timeout na resolução do hostname: %s\n", name);
                break;
            }
        }
    }
    else if (err != ERR_OK)
    {
        printf("Erro na resolução DNS para %s: %d\n", name, err);
    }

    if (ip_addr->addr != IPADDR_NONE && ip_addr->addr != IPADDR_ANY)
    {
        printf("Hostname %s resolvido para: %s\n", name, ipaddr_ntoa(ip_addr));
    }
    else
    {
        printf("Falha ao resolver hostname %s\n", name);
    }
}

//----------------------------------------------------------------------
// Callbacks para a conexão TCP
//----------------------------------------------------------------------

// receive_callback: Chamado quando dados são recebidos na conexão TCP.
static err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    LWIP_UNUSED_ARG(arg); // Evita warning de argumento não utilizado
    if (p != NULL)
    {
        printf("Dados recebidos do servidor: %.*s\n", p->len, (char *)p->payload);
        pbuf_free(p);                 // Libera o buffer de pacote após processamento
        tcp_recved(tpcb, p->tot_len); // Indica que os dados foram recebidos e podem ser liberados
    }
    else
    {
        // Conexão fechada pelo servidor (p == NULL indica EOF).
        printf("Conexão encerrada pelo servidor\n");
        tcp_close(tpcb);
        return ERR_OK;
    }
    return ERR_OK;
}

// connect_callback: Chamado quando a conexão TCP é estabelecida.
static err_t connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    bool *connect_result = (bool *)arg;
    if (err == ERR_OK)
    {
        printf("Conexão TCP estabelecida com o servidor\n");
        *connect_result = true;
        tcp_recv(tpcb, recv_callback); // Define o callback para recepção de dados
    }
    else
    {
        printf("Falha na conexão TCP: %d\n", err);
        *connect_result = false;
    }
    return ERR_OK; // Retorna ERR_OK para indicar que o callback foi processado
}

// error_callback: Chamado em caso de erro na conexão TCP.
static void error_callback(void *arg, err_t err)
{
    bool *write_result = (bool *)arg;
    printf("Erro TCP: %d\n", err);
    if (write_result)
    {
        *write_result = false;
    }
    // Não é necessário fechar a conexão aqui, lwIP cuida disso.
}

// sent_callback: Chamado quando os dados são enviados com sucesso.
static err_t sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    LWIP_UNUSED_ARG(len); // Evita warning de argumento não utilizado
    bool *write_result = (bool *)arg;
    printf("Dados enviados para o servidor\n");
    if (write_result)
    {
        *write_result = true;
    }
    return ERR_OK;
}

//----------------------------------------------------------------------
// Função: client_create
// Cria e estabelece uma conexão TCP com o servidor.
// Retorna true em caso de sucesso, false em caso de falha.
bool client_create(struct tcp_pcb *pcb, ip_addr_t *ip_addr, unsigned short port)
{
    if (!pcb || !ip_addr)
    {
        printf("client_create: parâmetros inválidos (pcb ou ip_addr nulo)\n");
        return false;
    }

    bool connect_success = false;
    bool *connect_result_ptr = &connect_success;

    tcp_arg(pcb, (void *)connect_result_ptr); // Passa ponteiro para callback
    tcp_err(pcb, error_callback);
    tcp_recv(pcb, recv_callback); // Define o callback de recepção AQUI, no create

    printf("Tentando conectar ao servidor TCP...\n");
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(pcb, ip_addr, port, connect_callback);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        printf("Falha ao iniciar tcp_connect: %d\n", err);
        return false; // Não fechar pcb aqui, tcp_err_callback será chamado se a conexão falhar
    }

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!(*connect_result_ptr))
    {
        sleep_ms(1);
        if (to_ms_since_boot(get_absolute_time()) - start_time > TCP_CONNECT_TIMEOUT_MS)
        {
            printf("Timeout ao conectar ao servidor TCP\n");
            tcp_close(pcb); // Fecha o PCB em caso de timeout
            return false;
        }
    }

    return true; // Retorna true se a conexão foi estabelecida com sucesso
}

//----------------------------------------------------------------------
// Função: client_close
// Encerra a conexão TCP.
void client_close(struct tcp_pcb *pcb)
{
    if (pcb)
    {
        tcp_close(pcb);
        printf("Conexão TCP encerrada\n");
    }
}

//----------------------------------------------------------------------
// Função: client_write
// Envia dados para o servidor TCP.
// Retorna true se a escrita foi iniciada com sucesso, false em caso de falha.
bool client_write(struct tcp_pcb *pcb, const char *data)
{
    if (!pcb || !data)
    {
        printf("client_write: parâmetros inválidos (pcb ou data nulo)\n");
        return false;
    }

    bool write_success = false;
    bool *write_result_ptr = &write_success;

    tcp_arg(pcb, (void *)write_result_ptr); // Passa ponteiro para callback
    tcp_sent(pcb, sent_callback);
    tcp_err(pcb, error_callback);

    cyw43_arch_lwip_begin();
    err_t err = tcp_write(pcb, data, strlen(data), TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        printf("Falha ao escrever dados: %d\n", err);
        return false; // Falha ao enfileirar para escrita, não necessariamente falha no envio
    }

    cyw43_arch_lwip_begin();
    err = tcp_output(pcb); // Envia os dados imediatamente
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        printf("Erro ao forçar tcp_output: %d\n", err);
        return false; // Falha ao enviar os dados
    }
    else
    {
        printf("Iniciando envio de dados para o servidor...\n");
    }

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!(*write_result_ptr))
    {
        sleep_ms(1);
        if (to_ms_since_boot(get_absolute_time()) - start_time > TCP_WRITE_TIMEOUT_MS)
        {
            printf("Timeout ao escrever dados para o servidor\n");
            return false;
        }
    }

    printf("client_write concluído\n");
    return true; // Retorna true se a escrita foi iniciada e confirmada por callback
}