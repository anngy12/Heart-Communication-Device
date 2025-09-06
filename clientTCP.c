#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "BPM"
#define WIFI_PASSWORD "123456789"
#define TCP_SERVER_IP "192.168.4.1"
#define TCP_PORT 4242

typedef struct {
    struct tcp_pcb *pcb;
    bool connected;
} tcp_client_t;

// Empfangs-Callback
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        printf("Server closed connection\n");
        tcp_close(tpcb);
        return ERR_OK;
    }

    printf("Empfangen: %.*s\n", p->len, (char*)p->payload);
    tcp_recved(tpcb, p->len);
    pbuf_free(p);
    return ERR_OK;
}

// Verbindung-Callback
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    tcp_client_t *client = (tcp_client_t*)arg;
    if (err == ERR_OK) {
        client->connected = true;
        printf("Mit Server verbunden!\n");
        tcp_recv(tpcb, tcp_client_recv);
    } else {
        printf("Verbindung fehlgeschlagen: %d\n", err);
    }
    return ERR_OK;
}

// TCP-Verbindung starten
bool tcp_client_open(tcp_client_t *client) {
    client->pcb = tcp_new();
    if (!client->pcb) return false;

    ip_addr_t server_ip;
    ip4addr_aton(TCP_SERVER_IP, &server_ip);

    tcp_arg(client->pcb, client);
    tcp_connect(client->pcb, &server_ip, TCP_PORT, tcp_client_connected);
    return true;
}

// Nachricht senden
void tcp_client_send(tcp_client_t *client, const char *msg) {
    if (client->connected) {
        cyw43_arch_lwip_begin();
        tcp_write(client->pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
        cyw43_arch_lwip_end();
        printf("Gesendet: %s\n", msg);
    }
}

void set_static_ip(struct netif *netif) {
    ip4_addr_t ip, mask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 10);    // Your desired static IP
    IP4_ADDR(&mask, 255, 255, 255, 0); // Subnet mask
    IP4_ADDR(&gw, 192, 168, 4, 1);     // Gateway (AP Pico’s IP)

    printf("Setting static IP address\n");
    netif_set_addr(netif, &ip, &mask, &gw);
    printf("Set static IP address\n");
}

int init_client_wifi(){
    sleep_ms(3000);
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    set_static_ip(&cyw43_state.netif[0]);

    printf("Verbinde mit WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("WiFi Verbindung fehlgeschlagen\n");
        return -1;
    }
    printf("WiFi verbunden\n");
}

tcp_client_t client = {0};

// function for tcp connection
bool poll_client_wifi(tcp_client_t *client){
    cyw43_arch_poll();
    tcp_client_open(client);

    while (!client->connected) {
        cyw43_arch_poll();       
        sleep_ms(10);
    }
    return client->connected;
}