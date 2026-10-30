/*
  ROBO DE SUMÔ AUTÔNOMO
  Controlador: ESP32 DevKit
  Sensores: 2 ultrassônicos + 3 infravermelhos
  Motores: 2 motores DC com driver H-bridge

  Estratégia:
  1. Evitar a borda da arena.
  2. Atacar o adversário detectado.
  3. Procurar o adversário quando ele não for detectado.
*/

// ==================================================
// 1. DEFINIÇÃO DOS PINOS
// ==================================================

// Sensor ultrassônico esquerdo
const int TRIG_ESQ = 32;
const int ECHO_ESQ = 33;

// Sensor ultrassônico direito
const int TRIG_DIR = 18;
const int ECHO_DIR = 5;

// Sensores infravermelhos
const int IR_ESQ = 4;
const int IR_CENTRO = 16;
const int IR_DIR = 22;

// Driver dos motores
const int IN1 = 19;
const int IN2 = 13;
const int IN3 = 12;
const int IN4 = 21;

const int ENA = 9;  // PWM motor esquerdo
const int ENB = 14;  // PWM motor direito

// ==================================================
// 2. CONFIGURAÇÕES
// ==================================================

// Velocidades entre 0 e 255
const int VELOCIDADE_ATAQUE = 230;
const int VELOCIDADE_BUSCA = 145;
const int VELOCIDADE_RECUO = 180;
const int VELOCIDADE_GIRO = 175;

// Distância para identificar o adversário
const float DISTANCIA_ATAQUE = 70.0;

// Distância mínima válida dos ultrassônicos
const float DISTANCIA_MINIMA = 2.0;

// Tempo de recuo ao detectar a borda
const int TEMPO_RECUO = 300;

// Tempo para girar e escapar da borda
const int TEMPO_ESCAPE = 350;

// Intervalo antes de iniciar a competição
const int TEMPO_INICIAL = 3000;

// ==================================================
// 3. CONFIGURAÇÃO DOS SENSORES INFRAVERMELHOS
// ==================================================

// Muitos módulos IR digitais apresentam LOW quando
// detectam uma superfície clara e HIGH quando detectam
// uma superfície escura.
//
// ATENÇÃO: isso depende do módulo e da calibração.
// Altere para HIGH se o seu sensor funcionar ao contrário.

const int BORDA_DETECTADA = LOW;

// ==================================================
// 4. FUNÇÕES DOS MOTORES
// ==================================================

void motorEsquerdo(int velocidade) {
  velocidade = constrain(velocidade, -255, 255);

  if (velocidade > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } 
  else if (velocidade < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } 
  else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  ledcWrite(ENA, abs(velocidade));
}

void motorDireito(int velocidade) {
  velocidade = constrain(velocidade, -255, 255);

  if (velocidade > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } 
  else if (velocidade < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } 
  else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  ledcWrite(ENB, abs(velocidade));
}

// Comando para os dois motores
void mover(int esquerdo, int direito) {
  motorEsquerdo(esquerdo);
  motorDireito(direito);
}

// Parar os motores
void parar() {
  mover(0, 0);
}

// Avançar
void avancar(int velocidade) {
  mover(velocidade, velocidade);
}

// Recuar
void recuar(int velocidade) {
  mover(-velocidade, -velocidade);
}

// Girar para a esquerda
void girarEsquerda(int velocidade) {
  mover(-velocidade, velocidade);
}

// Girar para a direita
void girarDireita(int velocidade) {
  mover(velocidade, -velocidade);
}

// ==================================================
// 5. LEITURA DOS SENSORES ULTRASSÔNICOS
// ==================================================

float medirDistancia(int trig, int echo) {
  // Garante um pulso de disparo limpo
  digitalWrite(trig, LOW);
  delayMicroseconds(3);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Tempo máximo de espera: 12 milissegundos
  unsigned long duracao = pulseIn(echo, HIGH, 12000);

  // Sem eco: distância desconhecida
  if (duracao == 0) {
    return -1.0;
  }

  float distancia = duracao * 0.0343 / 2.0;

  if (distancia < DISTANCIA_MINIMA) {
    return -1.0;
  }

  return distancia;
}

// ==================================================
// 6. DETECÇÃO DA BORDA DA ARENA
// ==================================================

bool detectarBordaEsquerda() {
  return digitalRead(IR_ESQ) == BORDA_DETECTADA;
}

bool detectarBordaCentro() {
  return digitalRead(IR_CENTRO) == BORDA_DETECTADA;
}

bool detectarBordaDireita() {
  return digitalRead(IR_DIR) == BORDA_DETECTADA;
}

// ==================================================
// 7. ESCAPE DA BORDA
// ==================================================

void escaparBorda() {
  bool esquerda = detectarBordaEsquerda();
  bool centro = detectarBordaCentro();
  bool direita = detectarBordaDireita();

  parar();
  delay(40);

  // Se o centro detectar a borda, recua em linha reta.
  if (centro) {
    recuar(VELOCIDADE_RECUO);
    delay(TEMPO_RECUO);

    // Gira para mudar a direção.
    if (esquerda && !direita) {
      girarDireita(VELOCIDADE_GIRO);
    } 
    else if (direita && !esquerda) {
      girarEsquerda(VELOCIDADE_GIRO);
    } 
    else {
      girarDireita(VELOCIDADE_GIRO);
    }

    delay(TEMPO_ESCAPE);
  }

  // Borda somente à esquerda: recua e vira à direita.
  else if (esquerda && !direita) {
    recuar(VELOCIDADE_RECUO);
    delay(TEMPO_RECUO);

    girarDireita(VELOCIDADE_GIRO);
    delay(TEMPO_ESCAPE);
  }

  // Borda somente à direita: recua e vira à esquerda.
  else if (direita && !esquerda) {
    recuar(VELOCIDADE_RECUO);
    delay(TEMPO_RECUO);

    girarEsquerda(VELOCIDADE_GIRO);
    delay(TEMPO_ESCAPE);
  }

  // Os dois lados detectaram a borda.
  else {
    recuar(VELOCIDADE_RECUO);
    delay(TEMPO_RECUO);

    girarDireita(VELOCIDADE_GIRO);
    delay(TEMPO_ESCAPE);
  }

  parar();
  delay(50);
}

// ==================================================
// 8. CONFIGURAÇÃO INICIAL
// ==================================================

void setup() {
  Serial.begin(115200);

  // Ultrassônicos
  pinMode(TRIG_ESQ, OUTPUT);
  pinMode(ECHO_ESQ, INPUT);

  pinMode(TRIG_DIR, OUTPUT);
  pinMode(ECHO_DIR, INPUT);

  digitalWrite(TRIG_ESQ, LOW);
  digitalWrite(TRIG_DIR, LOW);

  // Infravermelhos
  pinMode(IR_ESQ, INPUT);
  pinMode(IR_CENTRO, INPUT);
  pinMode(IR_DIR, INPUT);

  // Driver dos motores
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // PWM para ESP32 Arduino Core 3.x
  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);

  parar();

  Serial.println("ROBO DE SUMO INICIADO!");
  Serial.println("Inicio da contagem...");

  for (int i = 3; i > 0; i--) {
    Serial.println(i);
    delay(1000);
  }

  Serial.println("MODO AUTONOMO ATIVADO!");
}

// ==================================================
// 9. LOOP PRINCIPAL
// ==================================================

void loop() {

  // PRIORIDADE 1: EVITAR A BORDA

  if (detectarBordaEsquerda() ||
      detectarBordaCentro() ||
      detectarBordaDireita()) {

    Serial.println("BORDA DETECTADA! ESCAPANDO...");
    escaparBorda();

    return;
  }

  // PRIORIDADE 2: PROCURAR O ADVERSÁRIO

  float distanciaEsquerda =
      medirDistancia(TRIG_ESQ, ECHO_ESQ);

  delay(5);

  float distanciaDireita =
      medirDistancia(TRIG_DIR, ECHO_DIR);

  Serial.print("Ultrassonico esquerdo: ");
  Serial.print(distanciaEsquerda);

  Serial.print(" cm | Direito: ");
  Serial.print(distanciaDireita);

  Serial.println(" cm");

  bool adversarioEsquerda =
      distanciaEsquerda >= DISTANCIA_MINIMA &&
      distanciaEsquerda <= DISTANCIA_ATAQUE;

  bool adversarioDireita =
      distanciaDireita >= DISTANCIA_MINIMA &&
      distanciaDireita <= DISTANCIA_ATAQUE;

  // PRIORIDADE 3: ATACAR

  if (adversarioEsquerda && adversarioDireita) {

    Serial.println("ADVERSARIO A FRENTE! ATAQUE TOTAL!");

    avancar(VELOCIDADE_ATAQUE);
  }

  else if (adversarioEsquerda) {

    Serial.println("ADVERSARIO A ESQUERDA!");

    // Avança fazendo uma curva para a esquerda.
    mover(
      VELOCIDADE_ATAQUE * 0.65,
      VELOCIDADE_ATAQUE
    );
  }

  else if (adversarioDireita) {

    Serial.println("ADVERSARIO A DIREITA!");

    // Avança fazendo uma curva para a direita.
    mover(
      VELOCIDADE_ATAQUE,
      VELOCIDADE_ATAQUE * 0.65
    );
  }

  // PRIORIDADE 4: PROCURAR O ADVERSÁRIO

  else {

    Serial.println("PROCURANDO ADVERSARIO...");

    girarDireita(VELOCIDADE_BUSCA);
  }

  delay(20);
}
