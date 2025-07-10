
---

# 🔐 LockEase - Monitoramento e Controle de Acesso com Pico W (BITDOGLAB) via MQTT

![Linguagem](https://img.shields.io/badge/Linguagem-C-blue.svg)
![Plataforma](https://img.shields.io/badge/Plataforma-Raspberry%20Pi%20Pico%20W-purple.svg)
![Protocolo](https://img.shields.io/badge/Protocolo-MQTT-orange.svg)
![Interface](https://img.shields.io/badge/Interface-Web%20Customizada-green.svg)

Este projeto implementa um **sistema de controle de acesso** baseado em **RFID**, usando a placa **Raspberry Pi Pico W (BITDOGLAB)**. O sistema identifica usuários autorizados por RFID, aciona LED/buzzer e envia eventos MQTT para visualização remota. Também simula o **estado da porta** e permite visualização em tempo real em uma **página web personalizada**.

---

## 📲 Demonstração da Interface Web

|   Porta Aberta (Verde)  |  Porta Fechada (Vermelha)  |
| :---------------------: | :------------------------: |
| ![Aberta](img_open.png) | ![Fechada](img_closed.png) |

---

## ✨ Funcionalidades

* ✅ Leitura de tags RFID (MFRC522)
* 📶 Conectividade Wi-Fi
* 🔐 Autenticação de acesso com UID
* 🔊 Buzzer e LEDs como feedback local
* 🌐 Comunicação via MQTT com publicação de eventos:

  * `acesso_autorizado`
  * `acesso_negado`
  * `porta_aberta`
  * `porta_fechada`
* 📡 Interface web que muda de estado dinamicamente via WebSocket (MQTT)

---

## 🛠️ Hardware e Software

### Hardware

* Raspberry Pi Pico W (BitDogLab)
* Leitor RFID RC522
* Jumpers, LEDs (vermelho/verde), Buzzer

### Software

* [Pico SDK (C/C++)](https://github.com/raspberrypi/pico-sdk)
* Broker MQTT (ex: Mosquitto)
* Página HTML/CSS/JS personalizada (usa MQTT via WebSocket)
* Navegador Web

---

## 🧱 Arquitetura

```
[ Usuário com RFID ] → [ Pico W (RC522) ]
                         ↓ Wi-Fi
                  [ Broker MQTT (Mosquitto) ]
                         ↓ WebSocket
            [ Interface Web (HTML + JS + MQTT.js) ]
```

---

## ⚙️ Configurações Necessárias

### 📁 `src/configura_geral.h`

```c
#define WIFI_SSID       "SEU_WIFI"
#define WIFI_PASSWORD   "SENHA_WIFI"
#define MQTT_BROKER_IP  "192.168.1.100" // IP do seu Mosquitto
#define DEVICE_ID       "fechadura"
```

---

### 📄 `mosquitto.conf` (Broker Mosquitto)

Para ativar WebSocket:

```conf
listener 1883
protocol mqtt

listener 9001
protocol websockets
```

---

## 🚀 Compilando e Gravando no Pico W

1. **Clone o repositório:**

   ```bash
   git clone https://github.com/seu-usuario/LockEase.git
   cd LockEase
   ```

2. **Configure variáveis e Wi-Fi em `configura_geral.h`**

3. **Compile:**

   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Grave no Pico W:**

   * Segure o botão `BOOTSEL` e conecte o cabo USB
   * Copie o `.uf2` gerado para o dispositivo que aparecer

---

## 🌐 Interface Web (frontend)

Arquivo `index.html`:

```html
<script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>
<!-- veja versão completa com animação e botão dinâmico no repositório -->
```

O frontend se conecta via WebSocket ao Mosquitto e exibe o estado da porta com base nos eventos MQTT publicados.

---


## 🔮 Melhorias Futuras

* 📱 Aplicativo mobile com Flutter/PWA
* 🔒 Suporte a múltiplos usuários
* 📈 Log de acessos com timestamps
* 📧 Envio de alertas por email/Telegram

---

## 👨‍💻 Autor

**ASCCJR**
Projeto desenvolvido na residência tecnológica em sistemas embarcado apoiado pelo Ministério de tecnológia e comunicação(MCTI)
GitHub: [antonio-collab](https://github.com/antonio-collab)

---
