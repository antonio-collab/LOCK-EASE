#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "mfrc522.h"

#include "configura_geral.h"  // Contém definições como SSID, senha Wi-Fi, DEVICE_ID, etc.
#include "mqtt_lwip.h"        // Biblioteca para comunicação MQTT com lwIP

// --- Definições dos pinos ---
#define VERDE_PIN 11
#define VERMELHO_PIN 13
#define BUZZER_PIN 10

// UID da tag RFID autorizada
uint8_t tag_autorizada[] = {0xE1, 0xA4, 0xD8, 0x51};

/// @brief Inicia o buzzer com frequência e duty cycle definidos usando PWM.
void start_buzzer_pwm(int freq_hz, int duty_percent) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f); // Define clock de 1 MHz
    pwm_config_set_wrap(&config, 1000000 / freq_hz); // Frequência desejada
    pwm_init(slice, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, (1000000 / freq_hz) * duty_percent / 100); // Define duty cycle
}

/// @brief Para o som do buzzer e desativa PWM.
void stop_buzzer_pwm() {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_gpio_level(BUZZER_PIN, 0);      // Nível 0 (sem som)
    pwm_set_enabled(slice, false);          // Desliga PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_SIO); // Volta para controle GPIO normal
    gpio_put(BUZZER_PIN, 0);                // Garante que está em nível baixo
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);     // Define direção como saída
}

int main() {
    stdio_init_all(); // Inicializa USB para saída no terminal serial

    // Aguarda conexão do terminal serial USB
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("Sistema de Fechadura com Monitoramento MQTT\n");

    // --- Inicialização do Wi-Fi ---
    if (cyw43_arch_init()) {
        printf("ERRO: Falha ao inicializar Wi-Fi\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("ERRO: Falha ao conectar ao Wi-Fi\n");
        return -1;
    }
    printf("Wi-Fi conectado: %s\n", WIFI_SSID);

    // --- Inicialização do cliente MQTT ---
    iniciar_mqtt_cliente(); // Conecta-se ao broker MQTT
    for (int i = 0; i < 20 && !cliente_mqtt_esta_conectado(); i++) {
        sleep_ms(500); // Aguarda conexão
    }

    if (!cliente_mqtt_esta_conectado()) {
        printf("ERRO: Não foi possível conectar ao broker MQTT.\n");
    } else {
        printf("Conectado ao broker MQTT com sucesso.\n");
    }

    // --- Inicializa os LEDs e buzzer ---
    gpio_init(VERDE_PIN);
    gpio_set_dir(VERDE_PIN, GPIO_OUT);
    gpio_put(VERDE_PIN, 0);

    gpio_init(VERMELHO_PIN);
    gpio_set_dir(VERMELHO_PIN, GPIO_OUT);
    gpio_put(VERMELHO_PIN, 0);

    // --- Inicializa o módulo RFID ---
    MFRC522Ptr_t mfrc = MFRC522_Init();     // Aloca estrutura
    PCD_Init(mfrc, spi0);                   // Inicializa comunicação via SPI
    printf("Leitor RFID pronto.\n");

    char topico_evento[128];
    char payload_mensagem[128];
    snprintf(topico_evento, sizeof(topico_evento), "%s/acesso", DEVICE_ID); // Tópico de publicação

    // --- Estado da porta e tempo de abertura ---
    bool porta_aberta = false;
    absolute_time_t hora_abertura;

    while (true) {
        // Verifica presença de nova tag e lê UID
        if (cliente_mqtt_esta_conectado() && PICC_IsNewCardPresent(mfrc) && PICC_ReadCardSerial(mfrc)) {
            printf("UID detectado: ");
            char uid_str[32] = {0};
            for (int i = 0; i < mfrc->uid.size; i++) {
                printf("%02X ", mfrc->uid.uidByte[i]);
                sprintf(uid_str + strlen(uid_str), "%02X", mfrc->uid.uidByte[i]);
            }
            printf("\n");

            // --- Verifica se é uma tag autorizada ---
            if (memcmp(mfrc->uid.uidByte, tag_autorizada, 4) == 0) {
                gpio_put(VERDE_PIN, 1);
                start_buzzer_pwm(1000, 50);
                sleep_ms(200);
                stop_buzzer_pwm();
                gpio_put(VERDE_PIN, 0);

                // Marca como porta aberta e salva o tempo
                porta_aberta = true;
                hora_abertura = get_absolute_time();

                snprintf(payload_mensagem, sizeof(payload_mensagem),
                         "{\"evento\":\"porta_aberta\"}");
                publicar_mensagem_mqtt(topico_evento, payload_mensagem);
                printf("Publicado no tópico [%s]: %s\n", topico_evento, payload_mensagem);
            } else {
                // Tag não autorizada
                gpio_put(VERMELHO_PIN, 1);
                start_buzzer_pwm(1000, 50);
                sleep_ms(1000);
                stop_buzzer_pwm();
                gpio_put(VERMELHO_PIN, 0);

                snprintf(payload_mensagem, sizeof(payload_mensagem),
                         "{\"evento\":\"acesso_negado\", \"uid\":\"%s\"}", uid_str);
                publicar_mensagem_mqtt(topico_evento, payload_mensagem);
                printf("Publicado no tópico [%s]: %s\n", topico_evento, payload_mensagem);
            }

            sleep_ms(1500); // Aguarda retirada da tag antes de continuar
        }

        // --- Verifica se a porta ficou aberta por mais de 10 segundos ---
        if (porta_aberta) {
            int64_t diff_ms = absolute_time_diff_us(hora_abertura, get_absolute_time()) / 1000;
            if (diff_ms >= 10000) { // 10 segundos
                snprintf(payload_mensagem, sizeof(payload_mensagem),
                         "{\"evento\":\"porta_fechada\"}");
                publicar_mensagem_mqtt(topico_evento, payload_mensagem);
                printf("Publicado no tópico [%s]: %s\n", topico_evento, payload_mensagem);
                porta_aberta = false; // Marca como fechada
            }
        }

        sleep_ms(100); // Pequena pausa para evitar uso excessivo de CPU
    }

    return 0;
}
