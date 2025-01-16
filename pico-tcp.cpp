#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

// DEFINIÇÕES DE REDE WIFI
#define WIFI_SSID "HUAWEI-2.4G-S7mw"
#define WIFI_PASSWORD "labirasifpi"
#define SERVER_PORT 8050

// LED RGB
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11


// Função de callback para receber dados TCP
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        // Copiar os dados para um buffer legível
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char*)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula

        printf("Recebido: %s\n", buffer);
       
        // Liberar buffer
        pbuf_free(p);
    } else {
        // Se p for NULL, a conexão foi fechada
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função de callback para gerenciar nova conexão
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // Definir a função de recebimento para esta nova conexão
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

// Função para iniciar o servidor TCP
void tcp_server(void) {
    struct tcp_pcb *pcb;
    err_t err;

    printf("Iniciando servidor TCP...\n");

    // Criar um novo PCB (control block) para o servidor TCP
    pcb = tcp_new();
    if (pcb == NULL) {
        printf("Erro ao criar o PCB TCP.\n");
        return;
    }

    // Vincular o servidor ao endereço e porta desejada
    ip_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);  // Ou use IP_ADDR_ANY para todas as interfaces
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);
    if (err != ERR_OK) {
        printf("Erro ao vincular ao endereço e porta.\n");
        return;
    }

    // Colocar o servidor para ouvir conexões
    pcb = tcp_listen(pcb);
    if (pcb == NULL) {
        printf("Erro ao colocar o servidor em escuta.\n");
        return;
    }

    // Configurar a função de aceitação das conexões
    tcp_accept(pcb, tcp_accept_callback);
    printf("Servidor TCP iniciado na porta %d.\n", SERVER_PORT);
}

// Função principal
int main() {
    // Inicializar entradas e saídas padrão
    stdio_init_all();

    // Inicializar LED RGB
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    sleep_ms(1000);    

    char message[50] = "Conectando a ";
    strcat(message, WIFI_SSID);
    printf("%s\n", message);
    
    printf("Iniciando WiFi...\n");
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 1);


    // Inicializar a interface WiFi e conectar-se
    cyw43_arch_init();

    // Definir o modo de estabelecimento de conexão como se fosse um dispositivo qualquer. Ex: telefone, tablet...
    cyw43_arch_enable_sta_mode(); 
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha ao conectar ao WiFi.\n");
        gpio_put(LED_G_PIN, 0);

    }else{
        printf("Conectado ao WiFi.\n");
        
        // Exibe o endereço IP de maneira legível
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);

        gpio_put(LED_R_PIN, 0);
    };
    
    
    sleep_ms(300);

    // Iniciar o servidor TCP
    tcp_server();

    while (1) {
        // sleep_ms(100);  // Garantir que o loop não sobrecarregue o processador
        tight_loop_contents();
    }

    return 0;
}
