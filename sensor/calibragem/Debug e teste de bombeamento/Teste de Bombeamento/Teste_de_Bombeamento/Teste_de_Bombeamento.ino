// Definição dos seus pinos conforme informado
const int numSensores = 8;
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35, 36, 37};

void setup() {
  // Inicializa cada pino do array como SAÍDA
  for (int i = 0; i < numSensores; i++) {
    pinMode(releSaidas[i], OUTPUT);
    
    // Garante que iniciem DESLIGADOS (High geralmente desliga módulos de relé comuns)
    digitalWrite(releSaidas[i], HIGH); 
  }
}

void loop() {
  // 1. Liga um por um em sequência
  for (int i = 0; i < numSensores; i++) {
    digitalWrite(releSaidas[i], LOW); // Liga o relé
    delay(300);                       // Pequeno intervalo para o "click"
  }

  delay(1000); // Mantém todos ligados por 1 segundo

  // 2. Desliga um por um em sequência
  for (int i = 0; i < numSensores; i++) {
    digitalWrite(releSaidas[i], HIGH); // Desliga o relé
    delay(300);
  }

  delay(1000); // Pausa antes de recomeçar o ciclo
}