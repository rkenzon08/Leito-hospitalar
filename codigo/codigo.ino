#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// --- Configuracao da Rede Wi-Fi ---
const char* ssid = "CLARO_2GAB01DD"; // SUBSTITUA PELA SUA REDE
const char* password = "N@k@T@ni"; // Adicione a senha aqui, se houver

const char* mqtt_server = "192.168.0.154"; // IP do broker Mosquitto
const int mqtt_port = 1883; // Porta padrão do MQTT
const char* mqtt_user = ""; // Usuário (deixe vazio se não usar autenticação)
const char* mqtt_password = ""; // Senha (idem)

WiFiClient espClient;
PubSubClient client(espClient);

// --- Mapeamento dos Pinos no NodeMCU ESP8266 ---
const int TRIG_PIN = 16; // D0
const int ECHO_PIN = 5; // D1

// PINAGEM DOS LEDS
const int LED_VERMELHO_PIN = 4; // D2
const int LED_VERDE_PIN = 14; // D5

// Variaveis para a Logica e Calculo
long duracao;
int distancia;
// Limiar de ocupacao (20cm)
const int LIMIAR_OCUPACAO = 20; 


void setup() {
  // Inicia a comunicacao serial para debug
  Serial.begin(74880); 
  Serial.println();
  Serial.println("Sistema de Gerenciamento de Leito - NodeMCU ESP8266 Iniciado.");

  // Configuracao dos pinos digitais
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  
  // Garante que os LEDs estao desligados no inicio
  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  // --- 1. Conexao Wi-Fi ---
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se a ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("NodeMCU_Leito1")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void enviarStatusMQTT(String status) {
  if (!client.connected()) {
    reconnectMQTT();
  }

  String topic = "hospital/leito1/status";
  String payload = "{\"status\":\"" + status + "\",\"distancia\":" + String(distancia) + "}";
  
  // Publica no tópico
  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Status enviado via MQTT com sucesso!");
  } else {
    Serial.println("Falha ao enviar status via MQTT.");
  }
}

void loop() {
  // --- 1. Logica de leitura do sensor (calcula a 'distancia') ---
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // 1a. Disparo do Pulso (Pulso LOW para garantir limpeza)
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // 1b. Envia um pulso HIGH por 10 microssegundos
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 1c. Mede a duracao do pulso de retorno no pino Echo
  duracao = pulseIn(ECHO_PIN, HIGH);

  // 1d. Calcula a distancia em centimetros (distancia = duracao / 58)
  distancia = duracao / 58;
  
  // --- 2. Determina o status do leito ---

  String statusLeito;
  // Leito OCUPADO se a distancia for MENOR que o LIMIAR (20 cm) E MAIOR que 0 cm
  if (distancia < LIMIAR_OCUPACAO && distancia > 0) {
    statusLeito = "OCUPADO";
  } else {
    statusLeito = "LIVRE";
  }

  // --- 3. Atuacao Local (LEDs) ---
  
  if (statusLeito == "OCUPADO") {
    // Liga LED VERMELHO e Desliga LED VERDE
    digitalWrite(LED_VERMELHO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
  } else {
    // Liga LED VERDE e Desliga LED VERMELHO
    digitalWrite(LED_VERMELHO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
  }

  // --- 4. Comunicacao Serial e Envio via Wi-Fi ---
  
  // Imprime no Monitor Serial para depuracao
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.print(" cm. Status: ");
  Serial.println(statusLeito);
  
  // Envia o status via MQTT
  enviarStatusMQTT(statusLeito);
  
  delay(2000); // Intervalo de 2 segundos entre as leituras
}
