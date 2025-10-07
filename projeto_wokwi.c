#include "DHTesp.h"

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

// estados p/ evitar reimpressões
enum class PHEstado { Adequado, Baixo, MedioAlto, Alto, Desconhecido };
PHEstado phEstado = PHEstado::Desconhecido;

// histerese leve para faixas
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
bool bombaLigada = false;
const float UMI_ON  = 40.0;
const float UMI_OFF = 45.0;


void analisar_umidade(){
  if (millis() - ultimaLeituraUmid < PERIODO_UMID_MS) return;
  ultimaLeituraUmid = millis();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  umidade = data.humidity;

  if (isnan(umidade)) {
    log_change("UMID", "Falha ao ler umidade do DHT.", 5000);
    return;
  }

  if (!bombaLigada && umidade <= UMI_ON) {
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

// ------------------ Setup/Loop ------------------
void setup(){
  Serial.begin(115200);
  delay(200);
  log_change("BOOT", "FarmTech Solutions. ESP32 iniciado.");

  pinMode(PIN_BOMBA, OUTPUT);
  digitalWrite(PIN_BOMBA, LOW);

  pinMode(LDR_PIN, INPUT);
  pinMode(BTN_PIN_K, INPUT);
  pinMode(BTN_PIN_P, INPUT);
  pinMode(BTN_PIN_N, INPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
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
