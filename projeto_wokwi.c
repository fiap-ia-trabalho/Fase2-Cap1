#include "DHTesp.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ------------------ Pinos ------------------
#define PIN_BOMBA   4
#define LDR_PIN     14
#define ADC_MAX     4095
#define BTN_PIN_K   35
#define BTN_PIN_P   34
#define BTN_PIN_N   32
const int DHT_PIN = 18;

// ------------------ Sensores/Atuação ------------------
DHTesp dhtSensor;

// ------------------ Variaves -------------------------
unsigned long ultimaLeituraUmid = 0;
const unsigned long PERIODO_UMID_MS = 2000;
float umidade = 0;
String capital;
float valor_cidade_api = 0;

// ------------------ Parâmetros ------------------
bool bombaLigada = false;
const float UMI_ON  = 40.0;
const float UMI_OFF = 45.0;

// ---------- Wi-Fi / Cidades.json (OPCIONAL) ----------
#define FEATURE_WIFI_FETCH 1           
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* CIDADES_URL = "https://raw.githubusercontent.com/fiap-ia-trabalho/Fase2-Cap1/refs/heads/main/cidades.json";
float indiceCidade = NAN;
bool  wifiConectado = false;

// ------------------ Logger: imprime apenas quando muda ------------------
struct LogEntry {
  const char* tag;
  String lastMsg;
  unsigned long lastMs;
};
LogEntry logs[8];
size_t logsCount = 0;

void log_change(const char* tag, const String& msg, unsigned long minIntervalMs = 0) {
  for (size_t i = 0; i < logsCount; i++) {
    if (strcmp(logs[i].tag, tag) == 0) {
      if (msg != logs[i].lastMsg || (minIntervalMs && (millis() - logs[i].lastMs >= minIntervalMs))) {
        Serial.println(msg);
        logs[i].lastMsg = msg;
        logs[i].lastMs = millis();
      }
      return;
    }
  }
  if (logsCount < (sizeof(logs)/sizeof(logs[0]))) {
    logs[logsCount] = { tag, msg, millis() };
    Serial.println(msg);
    logsCount++;
  }
}

// ------------------ Botões ------------------
bool botao_pressionado(int pin){
  return digitalRead(pin) == HIGH;
}

void aguardar_botao(const char* nome, int pin){
  log_change("BTN_PROMPT", String("Por favor, pressione o botão ") + nome + ".", 0);
  while (!botao_pressionado(pin)) { delay(10); }
  log_change("BTN_CONF", String(nome) + " adicionado!", 0);
  delay(400);
}

// ------------------ Bomba ------------------
bool estadoBomba = false;
void ligar_bomba(bool liga){
  if (liga != estadoBomba) {
    estadoBomba = liga;
    digitalWrite(PIN_BOMBA, liga ? HIGH : LOW);
    log_change("BOMBA", liga ? "Bomba LIGADA." : "Bomba DESLIGADA.");
  }
}

// ------------------ pH ------------------
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum class PHEstado { Adequado, Baixo, MedioAlto, Alto, Desconhecido };
PHEstado phEstado = PHEstado::Desconhecido;

void analisar_ph(){
  float valorLido = analogRead(LDR_PIN);
  float ph = mapFloat(valorLido, 0, ADC_MAX, 0.0, 14.0);

  PHEstado novo;
  if (ph >= 5.0 && ph <= 6.0)           novo = PHEstado::Adequado;
  else if (ph < 4.8)                    novo = PHEstado::Baixo;
  else if (ph > 6.2 && ph < 8.0)        novo = PHEstado::MedioAlto;
  else if (ph >= 8.0)                   novo = PHEstado::Alto;
  else                                  novo = phEstado;

  if (novo != phEstado) {
    phEstado = novo;
    switch (phEstado) {
      case PHEstado::Adequado:
        log_change("PH", "pH adequado (5–6). Nenhuma ação necessária.");
        break;
      case PHEstado::Baixo:
        log_change("PH", "pH baixo (<5). Solicitar K (Potássio).");
        aguardar_botao("K (Potássio)", BTN_PIN_K);
        if(umidade < 50){
          ligar_bomba(true);
          delay(30000); // 30 segundos
          ligar_bomba(false);
        }
        break;
      case PHEstado::MedioAlto:
        log_change("PH", "pH entre 6 e 8. Solicitar N (Nitrogênio).");
        aguardar_botao("N (Nitrogênio)", BTN_PIN_N);
        break;
      case PHEstado::Alto:
        log_change("PH", "pH alto (>=8). Solicitar P (Fósforo).");
        aguardar_botao("P (Fósforo)", BTN_PIN_P);
        break;
      default: break;
    }
  }

  log_change("PH_RAW", String("Leitura pH ~ ") + String(ph, 2), 30000);
}

// ------------------ Umidade ------------------
void analisar_umidade(){
  if (millis() - ultimaLeituraUmid < PERIODO_UMID_MS) return;
  ultimaLeituraUmid = millis();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  umidade = data.humidity;

  if (isnan(umidade)) {
    log_change("UMID", "Falha ao ler umidade do DHT.", 5000);
    return;
  }

  if( valor_cidade_api >=  50){
    log_change("UMID_STAT", String("Bomba não precisa ser ligada chance de chuva alto: ") + String(valor_cidade_api, 1));
  } else if (!bombaLigada && umidade <= UMI_ON) {
    ligar_bomba(true);
    bombaLigada = true;
    log_change("UMID_STAT", String("Umidade baixa: ") + String(umidade, 1) + "%. Bomba ligada.");
  } else if (bombaLigada && umidade >= UMI_OFF) {
    ligar_bomba(false);
    bombaLigada = false;
    log_change("UMID_STAT", String("Umidade ok: ") + String(umidade, 1) + "%. Bomba desligada.");
  } else {
    log_change("UMID_RAW", String("Umidade: ") + String(umidade, 1) + "%", 30000);
  }
}

String ler_linha_do_serial_bloqueante(const char* prompt) {
  log_change("LOG_SETUP", prompt);
  String linha = "";

  while (Serial.available() > 0) Serial.read();

  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        linha.trim();
        if (linha.length() > 0) return linha;
        log_change("LOG_SETUP", "Digite algo e pressione ENTER:");
      } else {
        linha += c;
      }
    }
    delay(5);
  }
}

// --------- Wi-Fi helpers ----------
static bool conectar_wifi(uint32_t timeoutMs = 15000) {
#if FEATURE_WIFI_FETCH
  if (!WIFI_SSID || WIFI_SSID[0] == 0) { log_change("WIFI", "SSID vazio. Pulei Wi-Fi."); return false; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  log_change("WIFI", "Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) { delay(100); }
  if (WiFi.status() == WL_CONNECTED) {
    log_change("WIFI", String("Wi-Fi OK: ") + WiFi.localIP().toString());
    return true;
  }
  log_change("WIFI", "Falha ao conectar Wi-Fi (timeout).");
  return false;
#else
  (void)timeoutMs; return false;
#endif
}

static bool baixar_cidades(String& payload) {
#if FEATURE_WIFI_FETCH
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure(); // evita precisar de certificado (apenas para testes)
  HTTPClient http;
  if (!http.begin(client, CIDADES_URL)) {
    log_change("HTTP", "Falha ao iniciar HTTPClient.");
    return false;
  }
  int code = http.GET();
  if (code == 200) {
    payload = http.getString();
    http.end();
    return true;
  }
  log_change("HTTP", String("GET falhou. Code: ") + code);
  http.end();
  return false;
#else
  (void)payload; return false;
#endif
}

static bool extrair_indice_cidade(const String& json, const String& cidade, float& outVal) {
  String key = String('"') + cidade + String('"');
  int pos = json.indexOf(key);
  if (pos < 0) return false;
  int colon = json.indexOf(':', pos);
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < (int)json.length() && (json[i] == ' ' || json[i] == '\t')) i++;

  int j = i;
  while (j < (int)json.length() && json[j] != '}' && json[j] != ',') j++;

  String num = json.substring(i, j);
  num.trim();

  if (num.length() == 0) return false;

  bool temDigito = false;
  for (size_t k = 0; k < num.length(); k++) {
    if (isDigit(num[k])) { temDigito = true; break; }
  }
  if (!temDigito) return false;

  outVal = num.toFloat();
  return true;
}

// ------------------ Setup/Loop ------------------
void setup(){
  Serial.begin(115200);
  delay(200);
  log_change("BOOT", "FarmTech Solutions. ESP32 iniciado. Cultura utilizada (Café)");
  
  pinMode(PIN_BOMBA, OUTPUT);
  digitalWrite(PIN_BOMBA, LOW);

  pinMode(LDR_PIN, INPUT);
  pinMode(BTN_PIN_K, INPUT);
  pinMode(BTN_PIN_P, INPUT);
  pinMode(BTN_PIN_N, INPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  capital = ler_linha_do_serial_bloqueante("Qual a capital do cultivo? Digite e pressione ENTER:");

  wifiConectado = conectar_wifi();
  if (wifiConectado) {
    String json;
    if (baixar_cidades(json)) {
      float val = NAN;
      if (extrair_indice_cidade(json, capital, val)) {
        valor_cidade_api = val;

        log_change("CITY_IDX", String("Cidade '") + capital + "' encontrada. Valor = " + String(valor_cidade_api, 1));
      } else {
        log_change("CITY_IDX", String("Cidade '") + capital + "' não encontrada no JSON.");
      }
    } else {
      log_change("HTTP", "Não foi possível baixar cidades.json.");
    }
    // Desliga Wi-Fi para liberar ADC2 (pino 14) e manter leituras estáveis
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    log_change("WIFI", "Wi-Fi desligado após download único.");
  }
}

unsigned long ultimoPH = 0;
const unsigned long PERIODO_PH_MS = 3000;

void loop(){
  analisar_umidade();

  if (millis() - ultimoPH >= PERIODO_PH_MS) {
    ultimoPH = millis();
    analisar_ph();
  }

  delay(10);
}
