#include "DHTesp.h"

#define PIN_BOMBA 4

const int DHT_PIN = 18;

DHTesp dhtSensor;

void ligar_bomba(bool bomba){
  if (bomba){
    digitalWrite(PIN_BOMBA, HIGH);
    return;
  }
  digitalWrite(PIN_BOMBA, LOW);
}
// Funcao para anlisar o ph
// void analisar_ph(){
//   int valorLido = analogRead(LDR_PIN);
//   int escala14 = map(valorLido, 0, ADC_MAX, 0, 14);
//   if ((5.5 < escala14) && (escala14 < 6.5))
//   {
//     Serial.println("Nível de nutrientes dentro do adequado. Não é necessária nenhuma ação.");
//   }
//   else if (escala14 < 5)
//   {
//     ler_botao_k(true);
//   }
//   else if ((ph > 6.5) && (ph < 8))
//   {
//     ler_botao_n(true);
//   }
//   else if (ph > 8)
//   {
//     ler_botao_p(true);
//   }
// }

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

// // funcao do botao se está precionado
// bool botao_pressionado(uint8_t pin)
// {
//   uint8_t leitura = LOW;
//   do
//   {
//     uint8_t leitura = digitalRead(pin)
//   } while (leitura == HIGH)

//       return true;
// }
// // Funcao Ler botão P
// void ler_botao_p(fosforo)
// {
//   Serial.println("Por favor, pressione o botão P (Fósforo)");
//   botao_pressionado(12);
// }
// // Funcao Ler botão K
// void ler_botao_k(potassio)
// {
//   Serial.println("Por favor, pressione K (Potássio)");
//   botao_pressionado(13);
// }
// // Funcao Ler botão N
// void ler_botao_n(nitrogenio)
// {
//   Serial.println("Por favor, pressione N (Nitrogênio)");
//   botao_pressionado(14);
// }

void setup(){
  Serial.begin(115200);
  Serial.println("FarmTech Solutions. ESP32");
  pinMode(PIN_BOMBA, OUTPUT);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
}

void loop(){
  analisar_umidade();
  delay(2000);
}