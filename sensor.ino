const int sensorPin = A0; // Define o pino analógico A0 para o sensor

void setup() {
  Serial.begin(9600); // Inicia a comunicação serial a 9600 bits por segundo
}

void loop() {
  int valorSensor = analogRead(sensorPin); // Lê o valor do sensor no pino analógico A0
  Serial.print("Valor do sensor: "); // Imprime um texto descritivo
  Serial.println(valorSensor); // Imprime o valor lido pelo sensor

  delay(3000); // Espera 3000 milissegundos (3 segundos) antes da próxima leitura
}