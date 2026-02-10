const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};
const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// --- MONITORAMENTO E FALHAS ---
int tentativasRega[numSensores]; 
const int LIMITE_ALERTA_FALHA = 3;

// --- ESTATÍSTICAS (FLOAT PARA PRECISÃO) ---
float somaUmidadeDia[numSensores];
float somaBrutoDia[numSensores];
unsigned long contagemCiclosDia = 0; 
unsigned long tempoInicioDia = 0;
const unsigned long VINTE_QUATRO_HORAS = 86400000; 

// --- CONFIGURAÇÃO ---
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
    tentativasRega[i] = 0; // Inicializa tentativas
    somaUmidadeDia[i] = 0;
    somaBrutoDia[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);
  tempoInicioDia = millis();
  Serial.println(F("SISTEMA REINICIADO - AGUARDANDO LEITURAS"));
  Serial.println(F("Pressione 'H' para ver o relatorio de medias."));
}

void loop() {
  verificarComandos();
  checarNivelImediato();
  processarRega();
  checarViradaDeDia(); 
  delayInteligente(intervaloCiclo);
}

void processarRega() {
  Serial.println(F("\n--- Iniciando Varredura dos Vasos ---"));
  for (int i = 0; i < numSensores; i++) {
    checarNivelImediato();
    
    long somaLeiturasLocal = 0;
    // Realiza as 15 amostras ao longo de 2 segundos
    for (int j = 0; j < amostras; j++) {
      somaLeiturasLocal += analogRead(sensoresAnalogicos[i]);
      delayInteligente(133); 
    }
    
    int leituraMediaBruta = somaLeiturasLocal / amostras;
    int umidade = map(leituraMediaBruta, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    // Print em tempo real para calibração
    Serial.print(F("Vaso ")); Serial.print(i);
    Serial.print(F(": ")); Serial.print(umidade);
    Serial.print(F("% (ADC: ")); Serial.print(leituraMediaBruta); Serial.print(F(") "));

    // ACUMULADORES PARA O HISTÓRICO
    somaUmidadeDia[i] += (float)umidade; 
    somaBrutoDia[i] += (float)leituraMediaBruta;

    // LÓGICA DE REGA E ALERTA DE FALHA
    if (umidade < LIMITE_REGA) {
      Serial.println(F("-> [SOLO SECO]"));
      tentativasRega[i]++; // Incrementa tentativa de rega
      
      // Se já tentou regar mais que o limite e a umidade não subiu
      if (tentativasRega[i] >= LIMITE_ALERTA_FALHA) {
        Serial.print(F("!!! ALERTA CRITICO: FALHA NA BOMBA ")); 
        Serial.print(i); 
        Serial.println(F(" (Umidade persistente) !!!"));
        dispararAlertaSonoro();
      }

      digitalWrite(releSaidas[i], LOW); // Liga bomba
      delayInteligente(tempoRega);
      digitalWrite(releSaidas[i], HIGH); // Desliga bomba
      delayInteligente(tempoEspera);
    } else {
      Serial.println(F("-> [OK]"));
      tentativasRega[i] = 0; // Reseta tentativas se o solo estiver úmido
    }
  }
  contagemCiclosDia++; // Incrementa ciclo diário após passar pelos 6 sensores
}

void exibirHistoricoAtual() {
  Serial.println(F("\n################################################"));
  Serial.println(F("      RELATORIO DE MEDIAS ACUMULADAS HOJE       "));
  Serial.print(F("      Total de Ciclos Medidos: ")); Serial.println(contagemCiclosDia);
  Serial.println(F("################################################"));
  Serial.println(F(" VASO | MEDIA UMIDADE | MEDIA VALOR BRUTO (ADC) "));
  Serial.println(F("------|---------------|-------------------------"));

  if (contagemCiclosDia == 0) {
    Serial.println(F("      Aguarde o primeiro ciclo completar...     "));
  } else {
    for (int i = 0; i < numSensores; i++) {
      // Cálculo da média exata baseada nos acumuladores float
      int mediaUmi = (int)(somaUmidadeDia[i] / (float)contagemCiclosDia);
      int mediaBru = (int)(somaBrutoDia[i] / (float)contagemCiclosDia);
      
      Serial.print(F("  ")); Serial.print(i); 
      Serial.print(F("   |      ")); Serial.print(mediaUmi); 
      Serial.print(F("%      |         ")); Serial.println(mediaBru);
    }
  }
  Serial.println(F("################################################\n"));
}

void checarViradaDeDia() {
  if (millis() - tempoInicioDia >= VINTE_QUATRO_HORAS) {
    for (int i = 0; i < numSensores; i++) {
      somaUmidadeDia[i] = 0;
      somaBrutoDia[i] = 0;
      tentativasRega[i] = 0; // Opcional: resetar alertas de falha no novo dia
    }
    contagemCiclosDia = 0;
    tempoInicioDia = millis();
    Serial.println(F(">>> SISTEMA: INICIANDO NOVO DIA DE MEDICOES <<<"));
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
    Serial.println(F("!!! EMERGENCIA: RESERVATORIO VAZIO !!!"));
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

void dispararAlertaSonoro() {
  for (int r = 0; r < 5; r++) {
    tone(buzzerPin, 2000); delay(150); noTone(buzzerPin); delay(150);
  }
}