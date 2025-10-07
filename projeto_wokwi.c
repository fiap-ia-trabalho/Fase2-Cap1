#include "DHTesp.h"

#define PIN_BOMBA 4
#define LDR_PIN 14
#define ADC_MAX 4063
#define BTN_PIN_K 35
#define BTN_PIN_P 34
#define BTN_PIN_N 32

const int DHT_PIN = 18;

DHTesp dhtSensor;

bool botao_pressionado(int pin){
  int leitura = digitalRead(pin);
  return (leitura == HIGH);
}

void ler_botao_p(){
  Serial.println("Por favor, pressione o botão P (Fósforo)");
  while (!botao_pressionado(BTN_PIN_P));
  Serial.println("P (Fósforo) adicionado!");
  delay(2000);
}

void ler_botao_k(){
  Serial.println("Por favor, pressione K (Potássio)");
  while (!botao_pressionado(BTN_PIN_K));
  Serial.println("K (Potássio) adicionado!!");
  delay(2000);
}

void ler_botao_n(){
  Serial.println("Por favor, pressione N (Nitrogênio)");
  while (!botao_pressionado(BTN_PIN_N));
  Serial.println("N (Nitrogênio) adicionado!");
  delay(2000);
}

void ligar_bomba(bool bomba){
  if (bomba){
    digitalWrite(PIN_BOMBA, HIGH);
    return;
  }
  digitalWrite(PIN_BOMBA, LOW);
}


void analisar_ph(){
  float valorLido = analogRead(LDR_PIN);
  float ph = map(valorLido, 0, ADC_MAX, 0, 14);
  if ( ph >= 5 && ph <= 6){
    Serial.println("Nível de nutrientes dentro do adequado. Não é necessária nenhuma ação.");
  }
  else if (ph < 5){
    ler_botao_k();
  }
  else if (ph > 6 && ph < 8){
    ler_botao_n(); 
  }
  else if (ph > 8){
    ler_botao_p();
  }
}

void analisar_umidade(){
  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  float umidade = data.humidity;
  if (!isnan(umidade) && umidade <= 40){
    Serial.println("Nível de umidade abaixo do recomendado. Ligando bomba.");
    ligar_bomba(true);
    delay(2000);
    analisar_umidade();
  }
  else{
    ligar_bomba(false);
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println("FarmTech Solutions. ESP32");
  pinMode(PIN_BOMBA, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BTN_PIN_K, INPUT);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
}

void loop(){
  analisar_umidade();
  analisar_ph();
  delay(2000);
}