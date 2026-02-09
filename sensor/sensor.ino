const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};

const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// Variáveis para armazenar o histórico do último ciclo
int historicoUmidade[numSensores];
int tentativasRega[numSensores]; 
const int LIMITE_ALERTA_FALHA = 3;

// Calibração
const int amostras = 15;
const int VALOR_SECO = 635;    
const int VALOR_MOLHADO = 215; 
const int LIMITE_REGA = 90;    

// Tempos
const unsigned long tempoRega = 10000; 
const unsigned long tempoEspera = 5000; 
const unsigned long intervaloCiclo = 30000; 

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numSensores; i++) {
    pinMode(releSaidas[i], OUTPUT);
    digitalWrite(releSaidas[i], HIGH); 
    tentativasRega[i] = 0;
    historicoUmidade[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);
  Serial.println("SISTEMA DE IRRIGACAO AUTOMATICA INICIADO");
}

void loop() {
  checarNivelImediato();
  
  processarRega();
  
  apresentarHistorico(); // Chama a nova função de apresentação
  
  delayInteligente(intervaloCiclo);
}

void processarRega() {
  for (int i = 0; i < numSensores; i++) {
    checarNivelImediato();
    analogRead(sensoresAnalogicos[i]);
    delay(50);

    long somaLeituras = 0;
    for (int j = 0; j < amostras; j++) {
      somaLeituras += analogRead(sensoresAnalogicos[i]);
      delay(20);
    }
    int leituraMedia = somaLeituras / amostras;
    int umidade = map(leituraMedia, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    historicoUmidade[i] = umidade; // Salva para a apresentação final

    if (umidade < LIMITE_REGA) {
      tentativasRega[i]++; 
      if (tentativasRega[i] > LIMITE_ALERTA_FALHA) {
        alertaErroBomba(); 
      }
      digitalWrite(releSaidas[i], LOW);
      delayInteligente(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delayInteligente(tempoEspera);
    } else {
      tentativasRega[i] = 0;
      digitalWrite(releSaidas[i], HIGH);
    }
  }
}

void apresentarHistorico() {
  Serial.println("\n\n================================================");
  Serial.println("       RELATORIO DE UMIDADE DO CICLO            ");
  Serial.println("================================================");
  Serial.println(" VASO  |  UMIDADE  |  STATUS  | TENTATIVAS FALHAS ");
  Serial.println("-------|-----------|----------|-------------------");

  for (int i = 0; i < numSensores; i++) {
    Serial.print("  ["); Serial.print(i); Serial.print("]  |    ");
    Serial.print(historicoUmidade[i]); Serial.print("%    |  ");

    if (historicoUmidade[i] < LIMITE_REGA) {
      Serial.print("SECO    |       ");
    } else {
      Serial.print("OK      |       ");
    }

    Serial.println(tentativasRega[i]); // Quebra de linha final da linha da tabela
  }
  
  Serial.println("================================================");
  Serial.print("RESERVATORIO: ");
  if (digitalRead(sensorNivelPin)) Serial.println("VAZIO! ⚠️");
  else Serial.println("NIVEL OK ✅");
  Serial.println("================================================\n");
}

void checarNivelImediato() {
  while (digitalRead(sensorNivelPin)) {
    for (int i = 0; i < numSensores; i++) digitalWrite(releSaidas[i], HIGH);
    Serial.println("!!! ALERTA: RESERVATORIO SECO - SISTEMA SUSPENSO !!!");
    tone(buzzerPin, 1000); delay(300); noTone(buzzerPin); delay(300);
  }
}

void delayInteligente(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    checarNivelImediato();
    delay(1); 
  }
}

void alertaErroBomba() {
  for (int r = 0; r < 5; r++) {
    tone(buzzerPin, 2000); delay(150); noTone(buzzerPin); delay(150);
  }
}