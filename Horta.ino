#include <Arduino.h>

const int pinReleLed = 27;
const int pinReleBomba = 26;
const int pinLedVermelho = 32;
const int pinSensorLuz = 34;
const int pinSensorChuva = 35;

unsigned long tempoAnterior = 0;
const unsigned long intervaloBomba = 180000; // 3 minutos em milissegundos
const unsigned long duracaoLedLigado = 120000; // 2 minutos (120 segundos) em milissegundos
const unsigned long ciclo24Horas = 300000; // 5 minutos em milissegundos
const int limiteLuz = 2000; // Valor de referência para o sensor de luz (ajustável)
bool bombaAtivada = false;
bool nivelBaixo = false; // Variável para armazenar o estado do nível de água
bool ledLigado = false;
unsigned long tempoInicial;
unsigned long tempoLedLigado;

// Definição dos estados
#define ESTADO_0 0
#define ESTADO_1 1
#define ESTADO_2 2
#define ESTADO_3 3
#define ESTADO_4 4
#define ESTADO_5 5

int estadoAtual = ESTADO_1; // Estado inicial

void leituraSensores() {
    // Leitura do sensor de chuva
    int analogvalue = analogRead(pinSensorChuvaAnalog);
    int nivel_agua = map(analogvalue, 4095, 0, 0, 100); // Inverte a lógica: 0% fora da água, 100% dentro da água
    nivelBaixo = nivel_agua < 40; // Define o nível baixo como menos de 40%

    // Monitoramento serial das leituras
    Serial.print("Nível do reservatório: ");
    Serial.print(nivel_agua);
    Serial.println("%");
}

void controleLed() {
    int valorLuz = analogRead(pinSensorLuz); // Leitura do sensor de luz

    if (valorLuz >= limiteLuz) {
        digitalWrite(pinReleLed, LOW); // Desliga fita de LED se houver luz
        ledLigado = false;
    } else {
        unsigned long tempoAtual = millis();
        unsigned long tempoDecorrido = tempoAtual - tempoInicial;
        unsigned long tempoDecorridoDesdeLigado = tempoAtual - tempoLedLigado;

        if (tempoDecorrido >= ciclo24Horas) {
            tempoInicial = tempoAtual; // Reinicia contagem de 24 horas
        }

        if (!ledLigado && tempoDecorridoDesdeLigado >= 180000) { // 3 minutos (180000 ms) para ciclo de LED desligado
            digitalWrite(pinReleLed, HIGH); // Liga fita de LED por 2 minutos
            tempoLedLigado = tempoAtual;
            ledLigado = true;
        } else if (ledLigado && tempoDecorridoDesdeLigado >= duracaoLedLigado) {
            digitalWrite(pinReleLed, LOW); // Desliga fita de LED após 2 minutos
            ledLigado = false;
        }
    }

    Serial.print("Sensor de luz: ");
    Serial.println(valorLuz);
}

void setup() {
    Serial.begin(115200);
    pinMode(pinReleLed, OUTPUT);
    pinMode(pinReleBomba, OUTPUT);
    pinMode(pinLedVermelho, OUTPUT);
    pinMode(pinSensorLuz, INPUT);
    pinMode(pinSensorChuva, INPUT);
    

    // Inicializa relés e LED
    digitalWrite(pinReleLed, LOW); // Relé do LED desligado
    digitalWrite(pinReleBomba, LOW); // Relé da bomba desligado
    digitalWrite(pinLedVermelho, HIGH); // LED vermelho sempre aceso

    tempoInicial = millis();
    tempoLedLigado = millis();
}

void loop() {
    leituraSensores();
    controleLed();

    switch (estadoAtual) {
        case ESTADO_2:
            delay(intervaloBomba); // Tempo de execução de 3 minutos
            estadoAtual = ESTADO_3; // Próximo estado
            break;

        case ESTADO_3:
            // Monitora sensor de chuva
            if (digitalRead(pinSensorChuvaAnalog) || digitalRead(pinSensorChuvaDigital) || nivelBaixo) {
                estadoAtual = ESTADO_4; // Chuva detectada ou nível baixo
            } else {
                estadoAtual = ESTADO_5; // Nenhuma chuva e nível aceitável
            }
            delay(intervaloBomba); // Tempo de execução de 3 minutos
            break;

        case ESTADO_4:
            // Pisca LED vermelho e envia alerta se nível de água for baixo
            if (!nivelBaixo) {
                for (int i = 0; i < 6; i++) { // Pisca o LED 6 vezes (12 trocas de estado, 3 minutos no total)
                    digitalWrite(pinLedVermelho, !digitalRead(pinLedVermelho));
                    delay(15000); // Tempo de piscar (30 * 5000ms = 3 minutos)
                }
                digitalWrite(pinLedVermelho, HIGH); // Volta a deixar o LED vermelho aceso
                delay(1000); // Simula tempo de execução
            }
            estadoAtual = ESTADO_2; // Próximo estado
            break;

        case ESTADO_5:
            // Aciona a bomba de água apenas se o nível de água estiver acima de 40%
            if (!nivelBaixo) {
                digitalWrite(pinReleBomba, HIGH); // Liga a bomba
                bombaAtivada = true;
            } else {
                digitalWrite(pinReleBomba, LOW); // Desliga a bomba
                bombaAtivada = false;
            }
            tempoAnterior = millis(); // Marca o tempo atual
            delay(intervaloBomba); // Tempo de execução de 3 minutos
            estadoAtual = ESTADO_2; // Volta ao estado de monitoramento
            break;

        default:
            // Caso de estado inválido, volta ao estado 2 para continuar o monitoramento
            estadoAtual = ESTADO_2;
            break;
    }

    // Imprime o estado atual no monitor serial
    Serial.print("Estado atual: ");
    Serial.println(estadoAtual);

    delay(intervaloBomba); // Tempo de execução de 3 minutos
}
