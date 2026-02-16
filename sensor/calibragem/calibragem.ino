/* --- CALIBRADOR DE 6 SENSORES DE UMIDADE ---
   Instruções:
   1. Abra o Serial Monitor em 9600 baud.
   2. Coloque o sensor no substrato SECO e anote o valor (ADC).
   3. Adicione 135ml de água (IDEAL) e veja como o valor se comporta.
   4. Adicione água até SATURAR (225ml+) e anote o menor valor atingido.
*/

const int numSensores = 6;
const int sensores[numSensores] = {A0, A1, A2, A3, A4, A5};

void setup() {
  Serial.begin(9600);
  Serial.println(F("================================================"));
  Serial.println(F("      MODO CALIBRACAO: LEITURA BRUTA (ADC)      "));
  Serial.println(F("================================================"));
  delay(2000);
}

void loop() {
  Serial.print(F("VALORES: | "));
  
  for (int i = 0; i < numSensores; i++) {
    // Realiza uma média rápida de 10 leituras para evitar oscilação visual
    long soma = 0;
    for(int j = 0; j < 10; j++) {
      soma += analogRead(sensores[i]);
      delay(10);
    }
    int leituraEstavel = soma / 10;

    Serial.print(F("S"));
    Serial.print(i);
    Serial.print(F(": "));
    
    // Alinhamento estético para valores com menos dígitos
    if (leituraEstavel < 1000) Serial.print(F(" "));
    if (leituraEstavel < 100)  Serial.print(F(" "));
    
    Serial.print(leituraEstavel);
    Serial.print(F(" | "));
  }
  
  Serial.println(); // Nova linha para a próxima varredura
  delay(800);      // Velocidade de atualização confortável para leitura
}