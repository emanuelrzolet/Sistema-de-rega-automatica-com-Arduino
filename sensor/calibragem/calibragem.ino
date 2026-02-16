// =====================================================
// SISTEMA DE IRRIGACAO AUTOMATICA COM MONITORAMENTO
// COM CONTROLE DE FALHA E CALCULO DE CONSUMO DE AGUA
// =====================================================

// ---------------- CONFIGURACAO BASICA ----------------
const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};
const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// ---------------- MONITORAMENTO E FALHAS ----------------
int tentativasRega[numSensores]; 
const int LIMITE_ALERTA_FALHA = 3;

// ---------------- ESTATISTICAS ----------------
float somaUmidadeDia[numSensores];
float somaBrutoDia[numSensores];
unsigned long contagemCiclosDia = 0; 
unsigned long tempoInicioDia = 0;
const unsigned long VINTE_QUATRO_HORAS = 86400000; 

// ---------------- CONTROLE DE VAZAO ----------------
const float VAZAO_BOMBA_L_MIN = 1.25;
float litrosPorVasoDia[numSensores];
float litrosTotalDia = 0;

// ---------------- CONFIGURACAO DE LEITURA ----------------
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

  Serial.println(F("=============================================="));
  Serial.println(F("SISTEMA DE IRRIGACAO INICIADO"));
  Serial.println(F("Pressione 'H' para relatorio diario"));
  Serial.println(F("=============================================="));
}

// =====================================================

void loop() {
  verificarComandos();
  checarNivelImediato();
  processarRega();
  checarViradaDeDia(); 
  delayInteligente(intervaloCiclo);
}

// =====================================================

void processarRega() {

  Serial.println(F("\n--- Varredura dos Vasos ---"));

  for (int i = 0; i < numSensores; i++) {

    checarNivelImediato();

    long somaLeiturasLocal = 0;

    for (int j = 0; j < amostras; j++) {
      somaLeiturasLocal += analogRead(sensoresAnalogicos[i]);
      delayInteligente(133); 
    }

    int leituraMediaBruta = somaLeiturasLocal / amostras;

    if (leituraMediaBruta < 50 || leituraMediaBruta > 1000) {
      Serial.print(F("Vaso "));
      Serial.print(i);
      Serial.println(F(": ERRO SENSOR"));
      continue;
    }

    int umidade = map(leituraMediaBruta, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    Serial.print(F("Vaso "));
    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(umidade);
    Serial.print(F("% (ADC: "));
    Serial.print(leituraMediaBruta);
    Serial.print(F(") "));

    somaUmidadeDia[i] += umidade;
    somaBrutoDia[i] += leituraMediaBruta;

    if (umidade < LIMITE_REGA) {

      Serial.println(F("-> SOLO SECO"));

      int umidadeAntes = umidade;

      digitalWrite(releSaidas[i], LOW);
      delayInteligente(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delayInteligente(tempoEspera);

      float litrosAplicados = (tempoRega / 60000.0) * VAZAO_BOMBA_L_MIN;
      litrosPorVasoDia[i] += litrosAplicados;
      litrosTotalDia += litrosAplicados;

      Serial.print(F("   Agua aplicada: "));
      Serial.print(litrosAplicados, 3);
      Serial.println(F(" L"));

      delayInteligente(5000);

      int novaLeitura = analogRead(sensoresAnalogicos[i]);
      int novaUmidade = map(novaLeitura, VALOR_SECO, VALOR_MOLHADO, 0, 100);
      novaUmidade = constrain(novaUmidade, 0, 100);

      Serial.print(F("   Umidade apos rega: "));
      Serial.print(novaUmidade);
      Serial.println(F("%"));

      if (novaUmidade <= umidadeAntes) {

        tentativasRega[i]++;

        Serial.print(F("   Tentativa sem sucesso: "));
        Serial.println(tentativasRega[i]);

        if (tentativasRega[i] >= LIMITE_ALERTA_FALHA) {
          Serial.print(F("!!! ALERTA CRITICO: FALHA NA BOMBA DO VASO "));
          Serial.println(i);
          dispararAlertaSonoro();
        }

      } else {
        tentativasRega[i] = 0;
      }

    } else {

      Serial.println(F("-> OK"));
      tentativasRega[i] = 0;
    }
  }

  contagemCiclosDia++;
}

// =====================================================

void exibirHistoricoAtual() {

  Serial.println(F("\n======================================================"));
  Serial.println(F("            RELATORIO DIARIO DE IRRIGACAO             "));
  Serial.print(F("Total de Ciclos: "));
  Serial.println(contagemCiclosDia);
  Serial.println(F("------------------------------------------------------"));
  Serial.println(F("VASO | MEDIA % | MEDIA ADC | AGUA (L)"));
  Serial.println(F("------------------------------------------------------"));

  if (contagemCiclosDia == 0) {
    Serial.println(F("Aguardando primeiro ciclo..."));
  } else {
    for (int i = 0; i < numSensores; i++) {

      float mediaUmi = somaUmidadeDia[i] / (float)contagemCiclosDia;
      float mediaBru = somaBrutoDia[i] / (float)contagemCiclosDia;

      Serial.print(i);
      Serial.print(F("    |   "));
      Serial.print(mediaUmi, 2);
      Serial.print(F("%   |   "));
      Serial.print(mediaBru, 1);
      Serial.print(F("    |   "));
      Serial.println(litrosPorVasoDia[i], 3);
    }
  }

  Serial.println(F("------------------------------------------------------"));
  Serial.print(F("TOTAL DE AGUA HOJE: "));
  Serial.print(litrosTotalDia, 3);
  Serial.println(F(" L"));
  Serial.println(F("======================================================\n"));
}

// =====================================================

void checarViradaDeDia() {

  if ((unsigned long)(millis() - tempoInicioDia) >= VINTE_QUATRO_HORAS) {

    for (int i = 0; i < numSensores; i++) {
      somaUmidadeDia[i] = 0;
      somaBrutoDia[i] = 0;
      tentativasRega[i] = 0;
      litrosPorVasoDia[i] = 0;
    }

    litrosTotalDia = 0;
    contagemCiclosDia = 0;
    tempoInicioDia = millis();

    Serial.println(F(">>> NOVO DIA DE MEDICOES INICIADO <<<"));
  }
}

// =====================================================

void verificarComandos() {
  if (Serial.available() > 0) {
    char comando = Serial.read();
    if (comando == 'h' || comando == 'H') {
      exibirHistoricoAtual();
    }
  }
}

// =====================================================

void checarNivelImediato() {

  while (digitalRead(sensorNivelPin)) {

    for (int i = 0; i < numSensores; i++)
      digitalWrite(releSaidas[i], HIGH);

    Serial.println(F("!!! EMERGENCIA: RESERVATORIO VAZIO !!!"));

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
    verificarComandos();
    if (digitalRead(sensorNivelPin))
      checarNivelImediato();
    yield();
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
