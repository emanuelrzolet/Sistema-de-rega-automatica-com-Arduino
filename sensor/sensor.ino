const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};
const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// --- MONITORAMENTO ---
int historicoUmidade[numSensores];
int tentativasRega[numSensores]; 
const int LIMITE_ALERTA_FALHA = 3;

// --- ESTATÍSTICAS DIÁRIAS (ACUMULADORES) ---
long somaUmidadeDia[numSensores];
long somaBrutoDia[numSensores];   // Novo: Acumulador para valor bruto
long contagemCiclosDia = 0;
unsigned long tempoInicioDia = 0;
const unsigned long VINTE_QUATRO_HORAS = 86400000; 

// Estrutura para o histórico semanal (Médias Finais)
int histSemanUmidade[7][numSensores]; 
int histSemanBruto[7][numSensores];
int diaAtualIndice = 0; 

// --- CONFIGURAÇÃO TÉCNICA ---
const int amostras = 15;
const int VALOR_SECO = 635; 
const int VALOR_MOLHADO = 215; 
int LIMITE_REGA = 90; 

// --- TEMPOS ---
const unsigned long tempoRega = 10000; 
const unsigned long tempoEspera = 3000; 
const unsigned long intervaloCiclo = 10000; 

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numSensores; i++) {
    pinMode(releSaidas[i], OUTPUT);
    digitalWrite(releSaidas[i], HIGH); 
    tentativasRega[i] = 0;
    somaUmidadeDia[i] = 0;
    somaBrutoDia[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);
  tempoInicioDia = millis();
  
  Serial.println(F("================================================"));
  Serial.println(F("   SISTEMA DE MONITORAMENTO COM MEDIA BRUTA    "));
  Serial.println(F("================================================"));
  Serial.println(F(" >> Digite 'H' para ver as MEDIAS ATUAIS do dia "));
  Serial.println(F("================================================"));
}

void loop() {
  verificarComandos();
  checarNivelImediato();
  processarRega();
  checarViradaDeDia(); // Apenas para rotacionar o índice do histórico semanal
  delayInteligente(intervaloCiclo);
}

void processarRega() {
  Serial.println(F("\n--- Iniciando Varredura ---"));
  for (int i = 0; i < numSensores; i++) {
    checarNivelImediato();
    
    Serial.print(F("VASO ")); Serial.print(i); Serial.print(F(": "));
    
    long somaLeiturasLocal = 0;
    for (int j = 0; j < amostras; j++) {
      somaLeiturasLocal += analogRead(sensoresAnalogicos[i]);
      delayInteligente(133); 
    }
    
    int leituraMediaBruta = somaLeiturasLocal / amostras;
    int umidade = map(leituraMediaBruta, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    Serial.print(F("Bruto: ")); Serial.print(leituraMediaBruta);
    Serial.print(F(" | Umidade: ")); Serial.print(umidade); Serial.print(F("% "));

    // Acumula para o histórico
    historicoUmidade[i] = umidade;
    somaUmidadeDia[i] += umidade; 
    somaBrutoDia[i] += leituraMediaBruta;

    if (umidade < LIMITE_REGA) {
      Serial.println(F("-> [SECO]"));
      tentativasRega[i]++; 
      if (tentativasRega[i] > LIMITE_ALERTA_FALHA) {
        Serial.print(F("!!! FALHA NA BOMBA ")); Serial.print(i); Serial.println(F(" !!!"));
        alertaErroBomba(); 
      }
      digitalWrite(releSaidas[i], LOW);
      delayInteligente(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delayInteligente(tempoEspera);
    } else {
      Serial.println(F("-> [OK]"));
      tentativasRega[i] = 0;
    }
  }
  contagemCiclosDia++;
}

void exibirHistoricoAtual() {
  Serial.println(F("\n\n################################################"));
  Serial.println(F("      RELATORIO DE MEDIAS ACUMULADAS HOJE       "));
  Serial.print(F("      (Baseado em ")); Serial.print(contagemCiclosDia); Serial.println(F(" leituras realizadas)"));
  Serial.println(F("################################################"));
  Serial.println(F(" VASO | MEDIA UMIDADE | MEDIA VALOR BRUTO (ADC) "));
  Serial.println(F("------|---------------|-------------------------"));

  if (contagemCiclosDia == 0) {
    Serial.println(F("      Aguarde a primeira leitura...             "));
  } else {
    for (int i = 0; i < numSensores; i++) {
      int mediaUmi = somaUmidadeDia[i] / contagemCiclosDia;
      int mediaBru = somaBrutoDia[i] / contagemCiclosDia;
      
      Serial.print(F("  ")); Serial.print(i); Serial.print(F("   |      "));
      Serial.print(mediaUmi); Serial.print(F("%      |         "));
      Serial.println(mediaBru); 
    }
  }
  Serial.println(F("################################################\n"));
}

void checarViradaDeDia() {
  if (millis() - tempoInicioDia >= VINTE_QUATRO_HORAS) {
    // Salva a média final do dia no histórico semanal antes de zerar
    for (int i = 0; i < numSensores; i++) {
      if(contagemCiclosDia > 0) {
        histSemanUmidade[diaAtualIndice][i] = somaUmidadeDia[i] / contagemCiclosDia;
        histSemanBruto[diaAtualIndice][i] = somaBrutoDia[i] / contagemCiclosDia;
      }
      somaUmidadeDia[i] = 0;
      somaBrutoDia[i] = 0;
    }
    diaAtualIndice = (diaAtualIndice + 1) % 7;
    contagemCiclosDia = 0;
    tempoInicioDia = millis();
    Serial.println(F("\n>>> CICLO DE 24H FINALIZADO E REINICIADO <<<"));
  }
}

void verificarComandos() {
  if (Serial.available() > 0) {
    char comando = Serial.read();
    if (comando == 'h' || comando == 'H') {
      exibirHistoricoAtual();
    }
  }
}

void checarNivelImediato() {
  while (digitalRead(sensorNivelPin)) {
    for (int i = 0; i < numSensores; i++) digitalWrite(releSaidas[i], HIGH);
    Serial.println(F("!!! ALERTA: RESERVATORIO SECO !!!"));
    tone(buzzerPin, 1000); delay(300); noTone(buzzerPin); delay(300);
  }
}

void delayInteligente(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    verificarComandos();
    if (digitalRead(sensorNivelPin)) checarNivelImediato();
    yield();
  }
}

void alertaErroBomba() {
  for (int r = 0; r < 5; r++) {
    tone(buzzerPin, 2000); delay(150); noTone(buzzerPin); delay(150);
  }
}