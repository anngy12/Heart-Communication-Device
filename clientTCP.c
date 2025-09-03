#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#define WIFI_SSID "PicoAP"
#define WIFI_PASS "12345678"

static struct tcp_pcb *client_pcb;

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb,
                             struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Empfangene Nachricht vom Server
    printf("Client empfing: %.*s\n", p->len, (char *)p->payload);

    tcp_recved(tpcb, p->len);
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_client_send(struct tcp_pcb *pcb, const char *msg) {
    tcp_write(pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
    printf("Client sendet: %s\n", msg);
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    printf("Mit Server verbunden!\n");
    tcp_recv(tpcb, tcp_client_recv);

    // Test-Nachricht senden
    tcp_client_send(tpcb, "Hallo vom Client!");
    return ERR_OK;
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                           CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("WLAN-Verbindung fehlgeschlagen!\n");
        return -1;
    }
    printf("Mit AP verbunden!\n");

    // TCP-Verbindung zu Server starten
    ip_addr_t server_ip;
    ip4addr_aton("192.168.4.1", &server_ip);
    client_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    tcp_connect(client_pcb, &server_ip, 4242, tcp_client_connected);

    while (true) {
        cyw43_arch_poll();
        sleep_ms(1000);

        // Alle paar Sekunden etwas senden
        tcp_client_send(client_pcb, "Ping vom Client");
    }
}
