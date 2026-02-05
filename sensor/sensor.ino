const int numSensores = 6; // Ajustado para 6 sensores
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};

const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// --- DIAGNÓSTICO (Sem bloqueio) ---
int tentativasRega[numSensores]; 
const int LIMITE_ALERTA_FALHA = 3; // Avisa após 3 tentativas sem sucesso

// --- CALIBRAÇÃO ---
const int amostras = 15;
const int VALOR_SECO = 635;    
const int VALOR_MOLHADO = 215; 
const int LIMITE_REGA = 80;    

// --- TEMPOS ---
const unsigned long tempoRega = 10000; 
const unsigned long tempoEspera = 5000; 
const unsigned long intervaloCiclo = 30000; 

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numSensores; i++) {
    pinMode(releSaidas[i], OUTPUT);
    digitalWrite(releSaidas[i], HIGH); 
    tentativasRega[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);
  Serial.println("Sistema 6 Canais - Monitoramento de Falhas (Sem Bloqueio)");
}

void loop() {
  if (digitalRead(sensorNivelPin)) {
    bloqueioSeguranca();
  } 
  else {
    noTone(buzzerPin);
    processarRega();
    Serial.println("------------------------------------------");
    delay(intervaloCiclo); 
  }
}

void processarRega() {
  for (int i = 0; i < numSensores; i++) {
    if(digitalRead(sensorNivelPin)) return; 

    // Limpeza de cache do ADC
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

    Serial.print("Sensor "); Serial.print(i);
    Serial.print(" | Umidade: "); Serial.print(umidade); Serial.print("%");

    if (umidade < LIMITE_REGA) {
      tentativasRega[i]++; 
      
      // Se houver suspeita de falha, apenas avisa, não bloqueia
      if (tentativasRega[i] > LIMITE_ALERTA_FALHA) {
        Serial.print(" -> [AVISO: Bomba "); Serial.print(i); Serial.println(" pode estar com defeito]");
        alertaErroBomba();
      } else {
        Serial.println(" -> Solo Seco. Regando...");
      }

      // Aciona a bomba de qualquer forma
      digitalWrite(releSaidas[i], LOW);
      delay(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delay(tempoEspera);
      
    } else {
      Serial.println(" -> OK.");
      tentativasRega[i] = 0; // Reseta contador se a água for detectada
      digitalWrite(releSaidas[i], HIGH);
    }
    delay(100);
  }
}

void bloqueioSeguranca() {
  Serial.println("!!! RESERVATORIO VAZIO !!!");
  for (int i = 0; i < numSensores; i++) digitalWrite(releSaidas[i], HIGH);
  tone(buzzerPin, 1000); delay(200); noTone(buzzerPin); delay(200);
}

void alertaErroBomba() {
  // Beep de alerta curto enquanto rega/checa bomba suspeita
  tone(buzzerPin, 2000); delay(150);
  noTone(buzzerPin);
}