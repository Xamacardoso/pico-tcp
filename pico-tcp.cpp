#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

// Definições de rede
#define WIFI_SSID "20c"
#define WIFI_PASSWORD "12345678"
#define SERVER_PORT 8050

// Pinos do LED para indicar status de conexão
#define LED_PIN_R 13
#define LED_PIN_G 11

// Função de callback para receber dados TCP
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        printf("Conexão encerrada pelo cliente.\n");
        tcp_close(tpcb); // Fecha o bloco de controle da conexão encerrada
        return ERR_OK;
    }

    // Exibe a mensagem recebida do cliente
    printf("Recebido: %s\n", (char *)p->payload);

    // Enviar uma resposta ao cliente indicando que o servidor recebeu a mensagem
    const char *response = "Mensagem recebida pelo servidor!";
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    // Libera o buffer, evitando sobrecarga do servidor
    pbuf_free(p);
    return ERR_OK;
}

// Função de callback para gerenciar nova conexão
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    printf("Nova conexão aceita...\n");

    // A cada conexão, executa uma função de callback para gerenciar o recebimento
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

// Inicializa um servidor TCP na porta configurada
void start_tcp_server(){
    struct tcp_pcb *pcb;
    err_t err;

    printf("Iniciando servidor TCP...\n");

    // Cria um bloco de controle de processos (PCB) para o servidor TCP
    pcb = tcp_new();
    if (pcb == NULL){
        printf("Erro ao criar o PCB TCP");
        return;
    }

    // Vincular o servidor ao endereço e porta deseja
    ip_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0); // Use IP_ADDR_ANY para todas as interfaces
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);

    if (err != ERR_OK){
        printf("Erro ao vincular ao endereço e porta solicitados!\n");
        return;
    }

    // Coloca o servidor para ouvir conexões
    pcb = tcp_listen(pcb);
    if (pcb == NULL){
        printf("Erro ao colocar o servidor em escuta!\n");
        return;
    }

    // Configurar a função de aceitação das conexões
    tcp_accept(pcb, tcp_accept_callback);
    printf("Servidor TCP iniciado na porta %d.\n", SERVER_PORT);
}

int main()
{   
    // Inicializa as entradas e saídas seriais
    stdio_init_all();

    // Inicializa os pinos do LED RGB
    gpio_init(LED_PIN_R);
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);

    // Amarelo
    gpio_put(LED_PIN_R, 1);
    gpio_put(LED_PIN_G, 1);

    // Inicializa o chip WiFi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    /* Habilita o modo Station, permitindo que o raspberry pi pico w
    * se conecte a uma rede WiFi como um cliente, assim como um
    * celular, laptop ou outro dispositivo qualquer
    */
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao WiFi...\n");


    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 50000)) {
        printf("Falha ao conectar à rede!\n");
        gpio_put(LED_PIN_G, 0);
        return 1;
        

    } else {
        printf("Conectado á rede %s.\n", WIFI_SSID);
        gpio_put(LED_PIN_R, 0);

        // Converte e exibe o endereço IP do dispositivo na rede de uma forma legível
        char *ip_address = ip4addr_ntoa(&cyw43_state.netif[0].ip_addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    sleep_ms(500);

    start_tcp_server();

    while (true) {
        sleep_ms(100);
    }

    return 0;
}
