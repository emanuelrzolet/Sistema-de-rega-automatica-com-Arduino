// =====================================================
// SISTEMA DE IRRIGACAO AUTOMATICA - VERSAO JSON READY
// =====================================================

// ---------------- CONFIGURACAO BASICA ----------------
const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};
const int buzzerPin = 44;
const int sensorNivelPin = 53;

// ---------------- CONTROLE DE FALHA ----------------
int tentativasRega[numSensores];
const int LIMITE_ALERTA_FALHA = 3;

// ---------------- ESTATISTICAS ----------------
float somaUmidadeDia[numSensores];
float somaBrutoDia[numSensores];
float litrosPorVasoDia[numSensores];
float litrosTotalDia = 0;

unsigned long contagemCiclosDia = 0;
unsigned long tempoInicioDia = 0;
const unsigned long VINTE_QUATRO_HORAS = 86400000;

// ---------------- VAZAO ----------------
const float VAZAO_BOMBA_L_MIN = 1.25;

// ---------------- CONFIGURACAO ----------------
const int amostras = 15;
const int VALOR_SECO = 635;
const int VALOR_MOLHADO = 215;
int LIMITE_REGA = 90;

// ---------------- TEMPOS ----------------
const unsigned long tempoRega = 10000;
const unsigned long tempoEspera = 3000;
const unsigned long intervaloCiclo = 10000;

// =====================================================

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < numSensores; i++) {
    pinMode(releSaidas[i], OUTPUT);
    digitalWrite(releSaidas[i], HIGH);

    tentativasRega[i] = 0;
    somaUmidadeDia[i] = 0;
    somaBrutoDia[i] = 0;
    litrosPorVasoDia[i] = 0;
  }

  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);

  tempoInicioDia = millis();
}

// =====================================================

void loop() {
  processarRega();
  checarViradaDeDia();
  delayInteligente(intervaloCiclo);
}

// =====================================================

void processarRega() {

  contagemCiclosDia++;

  for (int i = 0; i < numSensores; i++) {

    if (digitalRead(sensorNivelPin)) {
      emergenciaReservatorio();
      return;
    }

    long somaLeiturasLocal = 0;

    for (int j = 0; j < amostras; j++) {
      somaLeiturasLocal += analogRead(sensoresAnalogicos[i]);
      delay(100);
    }

    int leituraMediaBruta = somaLeiturasLocal / amostras;

    if (leituraMediaBruta < 50 || leituraMediaBruta > 1000)
      continue;

    int umidade = map(leituraMediaBruta, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    somaUmidadeDia[i] += umidade;
    somaBrutoDia[i] += leituraMediaBruta;

    bool irrigou = false;
    float litrosAplicados = 0;

    // ---------- IRRIGACAO ----------
    if (umidade < LIMITE_REGA) {

      int umidadeAntes = umidade;

      digitalWrite(releSaidas[i], LOW);
      delay(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delay(tempoEspera);

      irrigou = true;

      litrosAplicados = (tempoRega / 60000.0) * VAZAO_BOMBA_L_MIN;
      litrosPorVasoDia[i] += litrosAplicados;
      litrosTotalDia += litrosAplicados;

      delay(5000); // tempo absorcao

      int novaLeitura = analogRead(sensoresAnalogicos[i]);
      int novaUmidade = map(novaLeitura, VALOR_SECO, VALOR_MOLHADO, 0, 100);
      novaUmidade = constrain(novaUmidade, 0, 100);

      if (novaUmidade <= umidadeAntes) {
        tentativasRega[i]++;

        if (tentativasRega[i] >= LIMITE_ALERTA_FALHA) {
          dispararAlertaSonoro();
        }

      } else {
        tentativasRega[i] = 0;
      }

      umidade = novaUmidade;
    } else {
      tentativasRega[i] = 0;
    }

    // ---------- LOG JSON POR VASO ----------
    Serial.print("{");
    Serial.print("\"timestamp\":");
    Serial.print(millis());
    Serial.print(",\"vaso\":");
    Serial.print(i);
    Serial.print(",\"umidade\":");
    Serial.print(umidade);
    Serial.print(",\"adc\":");
    Serial.print(leituraMediaBruta);
    Serial.print(",\"irrigou\":");
    Serial.print(irrigou ? "true" : "false");
    Serial.print(",\"litros_aplicados\":");
    Serial.print(litrosAplicados, 3);
    Serial.print(",\"litros_dia\":");
    Serial.print(litrosPorVasoDia[i], 3);
    Serial.print(",\"total_dia\":");
    Serial.print(litrosTotalDia, 3);
    Serial.print(",\"falhas\":");
    Serial.print(tentativasRega[i]);
    Serial.println("}");
  }

  // ---------- LOG JSON CONSOLIDADO DO CICLO ----------
  Serial.print("{\"ciclo\":");
  Serial.print(contagemCiclosDia);
  Serial.print(",\"total_dia\":");
  Serial.print(litrosTotalDia, 3);
  Serial.println("}");
}

// =====================================================

void checarViradaDeDia() {

  if ((unsigned long)(millis() - tempoInicioDia) >= VINTE_QUATRO_HORAS) {

    for (int i = 0; i < numSensores; i++) {
      somaUmidadeDia[i] = 0;
      somaBrutoDia[i] = 0;
      litrosPorVasoDia[i] = 0;
      tentativasRega[i] = 0;
    }

    litrosTotalDia = 0;
    contagemCiclosDia = 0;
    tempoInicioDia = millis();
  }
}

// =====================================================

void dispararAlertaSonoro() {
  for (int r = 0; r < 5; r++) {
    tone(buzzerPin, 2000);
    delay(150);
    noTone(buzzerPin);
    delay(150);
  }
}

// =====================================================

void emergenciaReservatorio() {

  for (int i = 0; i < numSensores; i++)
    digitalWrite(releSaidas[i], HIGH);

  Serial.println("{\"erro\":\"reservatorio_vazio\"}");

  while (digitalRead(sensorNivelPin)) {
    tone(buzzerPin, 1000);
    delay(300);
    noTone(buzzerPin);
    delay(300);
  }
}

// =====================================================

void delayInteligente(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    yield();
  }
}
