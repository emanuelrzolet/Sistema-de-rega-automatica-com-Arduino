const int numSensores = 6;
const int sensoresAnalogicos[numSensores] = {A0, A1, A2, A3, A4, A5};
const int releSaidas[numSensores] = {30, 31, 32, 33, 34, 35};
const int buzzerPin = 44; 
const int sensorNivelPin = 53;

// --- ESTATÍSTICAS (Usando float para evitar erros de soma grande) ---
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
    somaUmidadeDia[i] = 0;
    somaBrutoDia[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorNivelPin, INPUT_PULLUP);
  tempoInicioDia = millis();
  Serial.println(F("SISTEMA REINICIADO - AGUARDANDO PRIMEIRA LEITURA"));
}

void loop() {
  verificarComandos();
  checarNivelImediato();
  processarRega();
  checarViradaDeDia(); 
  delayInteligente(intervaloCiclo);
}

void processarRega() {
  Serial.println(F("\n--- Iniciando Varredura ---"));
  for (int i = 0; i < numSensores; i++) {
    checarNivelImediato();
    
    long somaLeiturasLocal = 0;
    for (int j = 0; j < amostras; j++) {
      somaLeiturasLocal += analogRead(sensoresAnalogicos[i]);
      delayInteligente(133); 
    }
    
    int leituraMediaBruta = somaLeiturasLocal / amostras;
    int umidade = map(leituraMediaBruta, VALOR_SECO, VALOR_MOLHADO, 0, 100);
    umidade = constrain(umidade, 0, 100);

    // Print em tempo real
    Serial.print(F("Vaso ")); Serial.print(i);
    Serial.print(F(": ")); Serial.print(umidade);
    Serial.print(F("% (Bruto: ")); Serial.print(leituraMediaBruta); Serial.println(F(")"));

    // ACUMULADORES (Soma os valores)
    somaUmidadeDia[i] += (float)umidade; 
    somaBrutoDia[i] += (float)leituraMediaBruta;

    if (umidade < LIMITE_REGA) {
      digitalWrite(releSaidas[i], LOW);
      delayInteligente(tempoRega);
      digitalWrite(releSaidas[i], HIGH);
      delayInteligente(tempoEspera);
    }
  }
  contagemCiclosDia++; // Incrementa o divisor global após ler todos
}

void exibirHistoricoAtual() {
  Serial.println(F("\n################################################"));
  Serial.println(F("      RELATORIO DE MEDIAS ACUMULADAS HOJE       "));
  Serial.print(F("      Leituras realizadas: ")); Serial.println(contagemCiclosDia);
  Serial.println(F("################################################"));
  Serial.println(F(" VASO | MEDIA UMIDADE | MEDIA VALOR BRUTO "));
  
  if (contagemCiclosDia == 0) {
    Serial.println(F("      Nenhum dado coletado ainda."));
  } else {
    for (int i = 0; i < numSensores; i++) {
      // Cálculo da média na hora de exibir
      int mediaUmi = (int)(somaUmidadeDia[i] / contagemCiclosDia);
      int mediaBru = (int)(somaBrutoDia[i] / contagemCiclosDia);
      
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
    }
    contagemCiclosDia = 0;
    tempoInicioDia = millis();
    Serial.println(F(">>> NOVO DIA INICIADO - MEDIAS ZERADAS <<<"));
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
    Serial.println(F("!!! RESERVATORIO VAZIO !!!"));
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