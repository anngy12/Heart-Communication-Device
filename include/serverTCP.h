#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "BPM"
#define WIFI_PASS "123456789"

typedef struct {
    struct tcp_pcb *pcb;
    bool connected;
} tcp_server_t;

extern tcp_server_t server;

int init_wifi();

static void tcp_server_send(void *arg, struct tcp_pcb *tpcb, u16_t len, int mode);

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

void poll_wifi();

#endif // SERVER_TCP_H