#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H


#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <stdbool.h>

#define WIFI_SSID "BPM"
#define WIFI_PASSWORD "123456789"
#define TCP_SERVER_IP "192.168.4.1"
#define TCP_PORT 4242

extern char recv_buf[64];
extern int recv_pos;

typedef struct {
    struct tcp_pcb *pcb;
    bool connected;
} tcp_client_t;

extern tcp_client_t client;

// 
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

void tcp_client_send_bpm(tcp_client_t *client, int bpm);

bool tcp_client_open(tcp_client_t *client);

void tcp_client_send(tcp_client_t *client, const char *msg);

void set_static_ip(struct netif *netif);

int init_client_wifi();

bool poll_client_wifi(tcp_client_t *client);

#endif // CLIENT_TCP_H