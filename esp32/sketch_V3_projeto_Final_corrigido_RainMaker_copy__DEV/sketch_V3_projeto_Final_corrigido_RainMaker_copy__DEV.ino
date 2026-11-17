/*****************************************************************************************
 * ESP32 SMART HUB - ULTIMATE VERSION
 * RECURSOS:
 * - ESP RainMaker
 * - Google Home
 * - Alexa
 * - Dashboard Web PRO
 * - Login no painel
 * - Automação configurável
 * - MQTT (Home Assistant / Node‑RED)
 * - Histórico avançado de sensores
 * - Gráficos em tempo real
 * - Gráfico 24h
 * - Scanner automático I2C
 * - Configurações salvas na FLASH
 * - Dashboard instalável como APP (PWA)
 * - Motor de automação avançado
 * - Previsão IA 2. Nível Avançado: Regressão Linear Simples
 *****************************************************************************************/
 #define CONFIG_ARDUINO_LOOP_STACK_SIZE 16384 // Adicione isso na linha 1 do seu arquivo

//#pragma GCC optimize ("Os")
//#pragma GCC optimize ("O3")

//Adicionado IR
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <LittleFS.h>

#include "RMaker.h"
#include "WiFiProv.h"
//#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>

// --- ADICIONE ESTAS LINHAS NO TOPO ---
//#include <HTTPClient.h>

//#include <ArduinoJson.h>
//unsigned long ultimaBuscaComandoMs = 0;
const unsigned long INTERVALO_BUSCA_COMANDOS = 2000; // Verifica a fila do Next.js a cada 2 segundos


// MQTT removido: o controle de dispositivos agora será feito apenas via RainMaker

#define SDA_PIN 21
#define SCL_PIN 22

// Pinos BMP280 (SPI)
#define BMP_SCK  13//SCL
#define BMP_MISO 33//SDO
#define BMP_MOSI 27//SDA
#define BMP_CS   26//CSB

const char *service_name = "PROV_ESPV8";
const char *pop = "12345678";

uint8_t relayPins[] = {23,25,14,19,18,32};

// Nomes de relés usados pelo RainMaker/Alexa. Nomes mais curtos são mais fáceis de reconhecer por voz.
const char* relayNames[] = {"Quarto","Exaustor","Ventilador","Sala","Bancada","Jardim"};

#define STATUS_LED 2
#define RESET_BUTTON 0 
//Adicionado IR
#define IR_RX 34
#define IR_TX 15//colocar resistor de 150Ω ou 220Ω
IRrecv irrecv(IR_RX, 100, 15, true); // ✅ FIXO: Limita o buffer a 100 amostras na RAM
IRsend irsend(IR_TX);
decode_results results;

// comandos aprendidos
String ac_on;
String ac_off;
String ac_23;
String tv_on;
String tv_off;
//Ate aui

//Adafruit_BME280 bme;
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);
LiquidCrystal_I2C lcd(0x3F,16,2);
WebServer server(80);
Node node;


// Variáveis para a Previsão
Preferences prefs;

const float ALTITUDE_LOCAL = 55.0;
float historico[6]; 
int amostrasContador = 0;// Contador de 1 a 6
float deltaP = 0;
float pAtual = 0;

float lastReportedPredictedTemp = 0;

// --- NO TOPO DO CÓDIGO (VARIÁVEIS GLOBAIS) ---
char statusAtual[25] = "Sincronizando...";  // ✅ FIXO: evita vazamento de String dinâmica

unsigned long ultimaLeituraMs = 0;
const unsigned long INTERVALO_HISTORICO = 3600000; // 1 hora em milisegundos//1800000; // 30 min

//Desenhos (Matrizes de Bits)
byte sol[8] = {0, 4, 21, 14, 31, 14, 21, 4};
byte nuvem[8] = {0, 0, 14, 31, 31, 31, 0, 0};
byte chuva[8] = {10, 10, 31, 31, 31, 0, 4, 4};
byte raio[8] = {2, 4, 8, 31, 4, 8, 16, 0};

/****************************************************************************
Previsão do Clima- Teste
/****************************************************************************/
void carregarDados() {
  // Abre o namespace "clima" em modo leitura (true)
  if (!prefs.begin("clima", true)) {
    Serial.println("Erro ao abrir Preferences (clima)");
    return;
  }
  amostrasContador = prefs.getInt("count", 0);
  // Carrega o array inteiro de uma vez só
  // O tamanho é calculado automaticamente por sizeof(historico)
  prefs.getBytes("hist_array", historico, sizeof(historico));

  prefs.end();
  Serial.println("Dados de clima carregados via Blob.");
}
void salvarDadosFlash() {
  // Abre o namespace "clima" em modo escrita (false)
  if (!prefs.begin("clima", false)) {
    Serial.println("Erro ao salvar no Flash");
    return;
  }
  prefs.putInt("count", amostrasContador);
  //adicionado teste-error
  //prefs.putInt("hIndex", historyIndex); // Salva a posição atual do histórico de 24h
  
  // Grava o array 'historico' inteiro como um bloco binário
  prefs.putBytes("hist_array", historico, sizeof(historico));

  prefs.end();
  Serial.println("Dados de clima salvos com sucesso.");
}

/***********************
 MQTT
***********************/
bool serverStarted=false;

Switch *relays[6];
Switch *backlightSwitch;
//Device *temp_sensor;
Device *alt_sensor;
TemperatureSensor *temp_sensor;

// Criar os objetos de TV e Ar Condicionado para RainMaker/Alexa.
// Os objetos RainMaker devem ser criados somente depois de RMaker.initNode().
Switch *minha_tv = nullptr;
Switch *meu_ar = nullptr;

//float lastTemp=-100;
//float lastAlt=-100;
float lastTemp = 0;
float lastAlt = 0;
float lastPres = 0;

unsigned long lastCloudUpdate=0;
unsigned long lastLCD=0;
unsigned long lastWiFiIPUpdate=0;

int screen=0;

// Cache do IP do WiFi para evitar interferência no I2C
char cachedWiFiIP[16] = "Aguardando...";  // ✅ FIXO: evita vazamento de String dinâmica

/*********************************************************************
CONSUMO ENERGIA
***********************/
// potência estimada dos dispositivos
float relayPower[6]={15,122,100,20,12,20};
/*********************************************************************
 CALCULAR CONSUMO
*********************************************************************/
float totalPower = 0; // Variável global que guarda o resultado

// Função 1: Apenas faz a conta e devolve o valor
float calcTotalPower() {
  float p = 0;
  for(int i = 0; i < 6; i++) {
    if(digitalRead(relayPins[i])) {
      p += relayPower[i];
    }
  }
  return p;
}

// Função 2: Chama a conta e salva na variável global
void atualizarConsumo() {
  totalPower = calcTotalPower(); 
}

/*********************************************************************
 HOME ASSISTANT DISCOVERY
*********************************************************************/
// MQTT/Home Assistant discovery removido: o fluxo agora é apenas RainMaker.
/*****************************************************************************************
 * LOGIN DASHBOARD
 *****************************************************************************************/
// Use credenciais fortes e únicas para proteger o acesso ao dashboard na rede.
String dashUser = "smartHubUser";
String dashPass = "S3nh@F0rt3_2026";


/*****************************************************************************************
 * HISTÓRICO AVANÇADO
 *****************************************************************************************/
#define HISTORY_SIZE 288   // 24h se coletar a cada 5 minutos

float tempHistory[HISTORY_SIZE];
float altHistory[HISTORY_SIZE];
float presHistory[HISTORY_SIZE];

int historyIndex=0;

void saveSensorHistory()
{
// Salva os dados globais que já foram lidos do sensor
 tempHistory[historyIndex]=lastTemp;
 altHistory[historyIndex]=lastAlt;
 presHistory[historyIndex]=lastPres;

 historyIndex++;

  // Reinicia o índice se chegar ao fim do array (Circular Buffer)
  if (historyIndex >= HISTORY_SIZE) {
    historyIndex = 0;
  }
}
/*****************************************************************************************
 * IA 2. Nível Avançado: Regressão Linear Simples (Melhorada)
 *****************************************************************************************/
 float predictedTemp=0;

 // Constantes para validação da regressão linear
 const int PREDICTION_SAMPLES = 20;          // Número de amostras para análise
 const float MAX_SLOPE_PER_SAMPLE = 0.067;   // ~2°C/hora ÷ 30 intervalos/hora ≈ 0.067°C/amostra
 const int PREDICTION_INTERVALS = 3;         // Projeta 3 intervalos à frente (15min se 5min/amostra)

 // Conta quantas amostras válidas existem no buffer
 int countValidSamples() {
   int count = 0;
   for(int i = 0; i < HISTORY_SIZE; i++) {
     float val = tempHistory[i];
     if (!isnan(val) && val > -50 && val < 80) {
       count++;
     }
   }
   return count;
 }

void predictTemperature() {
  int n = min(PREDICTION_SAMPLES, countValidSamples());
  if (n < 5) { // Aumentei para 5 para ter uma base estatística melhor
    predictedTemp = lastTemp;
    return;
  }

  float sumW = 0, sumWX = 0, sumWY = 0, sumWXY = 0, sumWX2 = 0;
  int validCount = 0;

  for(int i = 0; i < n; i++) {
    int idx = (historyIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    float y = tempHistory[idx];
    
    if (isnan(y) || y < -40 || y > 80) continue;

    // --- MELHORIA: PESO EXPONENCIAL ---
    // Amostras recentes (i pequeno) têm peso maior. 
    // Ex: i=0 (peso 1.0), i=10 (peso 0.5)
    float weight = 1.0 / (1.0 + (0.1 * i)); 

    float x = -i; 
    sumW   += weight;
    sumWX  += weight * x;
    sumWY  += weight * y;
    sumWXY += weight * x * y;
    sumWX2 += weight * x * x;
    validCount++;
  }

  if (validCount < 5) {
    predictedTemp = lastTemp;
    return;
  }

  // Cálculo da Regressão Linear Ponderada
  float denominator = (sumW * sumWX2 - (sumWX * sumWX));
  if (abs(denominator) < 0.0001) {
    predictedTemp = lastTemp;
    return;
  }

  float slope = (sumW * sumWXY - (sumWX * sumWY)) / denominator;
  
  // --- MELHORIA: FILTRO DE SLOPE (Smoothing) ---
  // Impede que a previsão mude bruscamente por um erro de leitura único
  static float filteredSlope = 0;
  filteredSlope = (filteredSlope * 0.7) + (slope * 0.3);

  // Validação de segurança
  if (abs(filteredSlope) > MAX_SLOPE_PER_SAMPLE) {
    filteredSlope = (filteredSlope > 0) ? MAX_SLOPE_PER_SAMPLE : -MAX_SLOPE_PER_SAMPLE;
  }

  // Previsão para os próximos X intervalos
  int passosNoFuturo = 2; 
  predictedTemp = lastTemp + (filteredSlope * passosNoFuturo);

  // Debug mais informativo
  Serial.printf("[IA] Real: %.2f | Previsto: %.2f | Tendência: %.4f\n", lastTemp, predictedTemp, filteredSlope);

  // Só atualiza a previsão se a mudança for relevante (ex: > 0.1°C)
  if (abs(predictedTemp - lastReportedPredictedTemp) > 0.1) {
      lastReportedPredictedTemp = predictedTemp;
      // Aqui você envia para o RainMaker
      //temp_sensor->updateAndReportParam("Temperature", predictedTemp);
      // Use o ponteiro (ptr) com a seta (->)
      temp_sensor->updateAndReportParam("Temperature", predictedTemp);
  }
}

/*****************************************************************************************
 * Função para exibir o ícone conforme o Status
 *****************************************************************************************/
void imprimirIcone(String status) {
  lcd.setCursor(15, 1); // Posiciona no ícone da direita
  
  // 1. ÍCONE DE SOL (Limpando ou Tempo Bom)
  if (status == "TEMPO BOM" || status == "BOM/LIMPO" || status == "VENTO FORTE" || status == "VENTANDO") {
    lcd.write(0); 
  } 
  // 2. ÍCONE DE NUVEM (Estável ou Nublado)
  else if (status == "TEMPO ESTAVEL" || status == "BOM ESTAVEL" || status == "MELHORANDO") {
    lcd.write(1); 
  }
  // 3. ÍCONE DE CHUVA (Chuva ou Piorando)
  else if (status == "CHANCE CHUVA" || status == "PIORANDO" || status == "NUBLADO") {
    lcd.write(2); 
  }
  // 4. ÍCONE DE RAIO (Temporal ou Crítico)
  else if (status == "CHUVA FORTE" || status == "TEMPORAL!") {
    lcd.write(3); 
  }
  // 5. ÍCONE DE CARREGAMENTO (Sincronizando)
  else {
    lcd.print("?"); // Aparece enquanto está calibrando
  }
}

/*****************************************************************************************
 * LCD
 *****************************************************************************************/
void updateLCD(){
 struct tm timeinfo;

 lcd.clear();
 delay(8); // ⚠️ CRÍTICO: PCF8574 é lento, precisa de tempo após clear()

  if(screen==0){
    lcd.setCursor(0,0);

    if(getLocalTime(&timeinfo)){lcd.printf("Data %02d/%02d/%04d",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900);
      lcd.setCursor(0,1);
      lcd.printf("Hora %02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
    } else {
    lcd.print("Sincronizando..");
    } 
  }
  else if(screen==1){
    lcd.setCursor(0,0);
    //lcd.printf("Temp: %.1f C\xDF",bmp.readTemperature());
    lcd.printf("Temp: %.1f C", lastTemp); // USE ESTA
    lcd.write(223);              // Imprime o símbolo de graus (°)

    lcd.setCursor(0,1);
    if (amostrasContador < 2) {
        lcd.print("Coletando dados");
    } else {
        //lcd.print(statusAtual);
        //imprimirIcone(statusAtual); // Adiciona o desenho no final da linha
        // E substitua temporariamente por:
        //lcd.print("Monitorando...");
        lcd.print(statusAtual);      // ✅ RESTAURADO: Mostra "TEMPO ESTAVEL", etc.
        imprimirIcone(statusAtual); // ✅ RESTAURADO: Desenha o Sol, Nuvem, etc.
    }
  }
  else if(screen==2) {
    lcd.setCursor(0,0);
    lcd.print("IP Dashboard");

    lcd.setCursor(0,1);
    if(WiFi.status()==WL_CONNECTED){
      // comente as linhas de baixo para o LCD não travar o BLE:
      lcd.print(cachedWiFiIP);  // Usa cache em vez de WiFi.localIP()
    }
    else{
      lcd.print("Esperando WiFi");
    }
  }
 else if(screen==3){
  lcd.setCursor(0,0);
  lcd.print("Temp Prox Hora:");
  lcd.setCursor(0,1);
  lcd.print(predictedTemp, 1); // Imprime o valor com 1 casa decimal
  lcd.print(" C");             // Imprime a letra C logo em seguida
  lcd.write(223);              // Imprime o símbolo de graus (°)
 }
 else if(screen==4){
  lcd.setCursor(0,0);
  lcd.print("Consumo Watts:");
  lcd.setCursor(0,1);
  lcd.print(totalPower,0);
  lcd.print(" Watts");
 }
 else if(screen==5) {
    lcd.setCursor(0, 0);
    lcd.print("STATUS RELES:");
    lcd.setCursor(0, 1);
    lcd.print("R: ");
    for(int i=0; i<6; i++) {
        lcd.print(digitalRead(relayPins[i]) == HIGH ? "L" : "D");
        lcd.print(" ");
    }
  }
  else if(screen==6) {
    lcd.setCursor(0, 0);
    lcd.print("   SMART-NODE");
    lcd.setCursor(0, 1);
    lcd.print("CASA INTELIGENTE");
  }
  else if(screen==7) {
    lcd.setCursor(0, 0);
    lcd.printf("ATM %.1f ", pAtual);//apaguei hpa
    // Indicador de progresso no canto
    lcd.setCursor(12, 0);
    lcd.printf(" %d/6", amostrasContador);

    lcd.setCursor(0, 1);
    if (amostrasContador < 2) {
        lcd.print("Coletando dados");
    } else {
        lcd.print(statusAtual);
        imprimirIcone(statusAtual); // Adiciona o desenho no final da linha
        // E substitua temporariamente por:
        //lcd.print("Monitorando1..");
    }
  }
  // Adicione isto bem na última linha, antes de fechar a chave "}" da função updateLCD():
  static unsigned long timerResetDisplay = 0;
  if (millis() - timerResetDisplay > 3600000) { // A cada 1 hora (3600000 ms)
    timerResetDisplay = millis();
     // 💡 PASSO CRÍTICO: Guarda o estado atual dos relés/backlight antes de resetar
    // Se o seu relayPins ou o estado do backlight do RainMaker estiver ativo:
    bool backlightEstavaLigado = true;
    if (backlightSwitch) {
      // Lê o estado real do parâmetro do RainMaker
      backlightEstavaLigado = backlightSwitch->getParamVal("Power").val.b; 
    }

    lcd.init(); // Soft-reset físico do display
    
    // Aplica o backlight apenas se ele realmente deveria estar ligado!
    if (backlightEstavaLigado) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
    // Recria os caracteres especiais para eles não sumirem após o reset
    lcd.createChar(0, sol);
    lcd.createChar(1, nuvem);
    lcd.createChar(2, chuva);
    lcd.createChar(3, raio);
    Serial.println("[DISPLAY] Reset preventivo executado respeitando o Backlight.");
  }
}

/*****************************************************************************************
 * CALLBACK RELÉS
 *****************************************************************************************/
// Função Auxiliar: Formata dados IR em string (consolida código duplicado)
String formatIRCode(const decode_results &results) {
  String data = "";
  
  // 1. Protocolo AC (usa State Array)
  if (hasACState(results.decode_type)) {
    data = String(results.decode_type) + "," + String(results.bits) + ",";
    for (uint16_t i = 0; i < results.bits / 8; i++) {
      if (results.state[i] < 0x10) data += "0";
      data += String(results.state[i], HEX);
    }
  }
  // 2. Protocolo RAW (desconhecido)
  else if (results.decode_type == UNKNOWN) {
    data = "RAW," + String(results.rawlen) + ",";
    for (uint16_t i = 1; i < results.rawlen; i++) {
      data += String(results.rawbuf[i] * kRawTick) + (i < results.rawlen - 1 ? "," : "");
    }
  }
  // 3. Protocolo Simples
  else {
    data = String(results.decode_type) + "," + String(results.bits) + "," + uint64ToString(results.value, 16);
  }
  
  return data;
}

// Função auxiliar: Decodifica e envia comando IR (reutiliza lógica)
void decodeAndSendIR(const String &codeStr) {
  if (codeStr.length() == 0) {
    Serial.println("IR: comando não definido");
    return;
  }

  if (codeStr.startsWith("RAW")) {
    int firstComma = codeStr.indexOf(',');
    int secondComma = codeStr.indexOf(',', firstComma + 1);
    int len = codeStr.substring(firstComma + 1, secondComma).toInt();
    if (len > 0 && len < 500) {
      uint16_t *rawBuffer = new uint16_t[len];
      int startIdx = secondComma + 1;
      for (int i = 0; i < len - 1; i++) {
        int nextComma = codeStr.indexOf(',', startIdx);
        if (nextComma == -1) nextComma = codeStr.length();
        rawBuffer[i] = codeStr.substring(startIdx, nextComma).toInt();
        startIdx = nextComma + 1;
      }
      irsend.sendRaw(rawBuffer, len - 1, 38);
      delete[] rawBuffer;
    }
    return;
  }

  int firstComma = codeStr.indexOf(',');
  int secondComma = codeStr.indexOf(',', firstComma + 1);
  if(firstComma != -1 && secondComma != -1) {
    int protocol = codeStr.substring(0, firstComma).toInt();
    int bits = codeStr.substring(firstComma + 1, secondComma).toInt();
    String valStr = codeStr.substring(secondComma + 1);

    if (hasACState((decode_type_t)protocol)) {
      uint8_t *state = new uint8_t[bits / 8];
      for (int i = 0; i < bits / 8; i++) {
        char byteStr[3] = { valStr[i*2], valStr[i*2+1], '\0' };
        state[i] = strtoul(byteStr, NULL, 16);
      }
      irsend.send((decode_type_t)protocol, state, bits / 8);
      delete[] state;
    } else {
      uint64_t code = strtoull(valStr.c_str(), NULL, 16);
      irsend.send((decode_type_t)protocol, code, bits);
    }
  } else {
    uint64_t code = strtoull(codeStr.c_str(), NULL, 16);
    irsend.sendNEC(code, 32);
  }
}

void sendIRCommand(const String &cmd) {
  String codeStr = "";
  if(cmd == "ac_on") codeStr = ac_on;
  else if(cmd == "ac_off") codeStr = ac_off;
  else if(cmd == "ac_23") codeStr = ac_23;
  else if(cmd == "tv_on") codeStr = tv_on;
  else if(cmd == "tv_off") codeStr = tv_off;

  decodeAndSendIR(codeStr);
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
    const char *name = device->getDeviceName();
    const char *param_name = param->getParamName();

    // 💡 IMPORTANTE: Verifica se o parâmetro alterado é o de ligar/desligar
    if (strcmp(param_name, "Power") == 0) {
        bool state = val.val.b;

        // Controle da TV
        if (strcmp(name, "TV") == 0) {
            if (state) sendIRCommand("tv_on");
            else sendIRCommand("tv_off");
            param->updateAndReport(val);
            return;
        }

        // Controle do Ar Condicionado
        if (strcmp(name, "Ar Condicionado") == 0) {
            if (state) sendIRCommand("ac_on");
            else sendIRCommand("ac_off");
            param->updateAndReport(val);
            return;
        }

        // Controle do Backlight do LCD
        if (strcmp(name, "Backlight") == 0) {
            if (state) lcd.backlight();
            else lcd.noBacklight();
            param->updateAndReport(val);
            return;
        }

        // Controle dos Relés de 0 a 5
        for (int i = 0; i < 6; i++) {
            if (strcmp(name, relayNames[i]) == 0) {
                digitalWrite(relayPins[i], state ? HIGH : LOW);
                param->updateAndReport(val);
                atualizarConsumo(); 
                return; // Encontrou, pode sair da função
            }
        }
    }
}
/*Função de texto********************************************************************************
String calcularStatusTempo() {
  if (amostrasContador < 2) return "Sincronizando...";
  if (deltaP <= -2.0)      return "TEMPORAL!";
  if (deltaP <= -0.7)      return "CHANCE CHUVA";
  if (deltaP >= 0.7)       return "TEMPO BOM";
  return "TEMPO ESTAVEL";
} */
  //////////TESTANDO GEMINI
  // ✅ CORRIGIDO: Retorna const char* em vez de String dinâmica
  // Passe a pressão atual e o deltaP (idealmente a diferença da última 1 hora)
const char* calcularStatusTempo( float pAtual, float deltaP) {
  if (amostrasContador < 2) return "Sincronizando";
  // 1. Condição Extrema: Queda ou Subida muito brusca
  if (deltaP <= -2.0) return "TEMPORAL!";
  if (deltaP >=  2.0) return "VENTANDO"; // Sobe rápido == vento forte limpando o tempo
  // 2. Análise combinando a Pressão Atual com a Tendência (Delta)
  // ALTA PRESSÃO (Geralmente tempo bom)
  if (pAtual >= 1015.0) {
    if (deltaP <= -0.5) return "PIORANDO";
    if (deltaP >=  0.5) return "TEMPO BOM";
    return "BOM ESTAVEL";
  }
  // BAIXA PRESSÃO (Geralmente tempo ruim)
  else if (pAtual <= 1005.0) {
    if (deltaP >= 0.5) return "MELHORANDO";
    if (deltaP <= -0.5) return "CHUVA FORTE";
    return "NUBLADO";
  }
  // PRESSÃO NORMAL (Entre 1005 e 1015 hPa - Zona de transição)
  else {
    if (deltaP <= -0.7) return "CHANCE CHUVA";
    if (deltaP >= 0.7)  return "BOM/LIMPO";
    return "TEMPO ESTAVEL";
  }
}

// =========================================================================================
// INTEGRACAO NEXT.JS FULL-STACK - ADICIONE ESTAS FUNÇÕES NO SEU CÓDIGO
// =========================================================================================

// VERSÃO INTEGRADA AO NEXT.JS (Devolve o código capturado via HTTP)
void learnCommand() {
  results.decode_type = UNKNOWN;
  results.value = 0;
  results.rawlen = 0;

  irrecv.resume();
  delay(200); 
  yield();

  Serial.println("[IR] Aguardando sinal IR por 10s...");
  unsigned long timeout = millis();
  while (!irrecv.decode(&results)) {
    if (millis() - timeout > 10000) { 
      irrecv.resume(); 
      server.send(408, "text/plain", "Timeout"); // Avisa o Next.js que estourou o tempo
      return;
    }
    delay(10);
    yield(); 
  }

  String data = formatIRCode(results);
  Serial.println("[IR] Capturado: " + data);
  irrecv.resume();

  // Envia o código capturado de volta para o Next.js salvar no PostgreSQL!
  server.send(200, "text/plain", data);
}

// enviar comando IR
// VERSÃO REAL DE DISPARO INFRAVERMELHO
void sendCommand() {
  if (server.hasArg("code")) {
    String codigoRecebido = server.arg("code");
    Serial.println("[IR] Disparando codigo recebido: " + codigoRecebido);
    
    decodeAndSendIR(codigoRecebido); // Dispara o LED físico
    
    server.send(200, "text/plain", "Sinal disparado!");
    return;
  }
  server.send(400, "text/plain", "Faltou o parametro code");
}

/*****************************************************************************************
 Salvar configuração em arquivo flash(Agora no prefs)
 *****************************************************************************************
*/
/////////////////////////////////////////////////////////////////
// ✅ VERSÃO RESTAURADA: NVS (Preferences) - SEM VAZAMENTO DE MEMÓRIA
 
// Substitua a sua função saveConfig() antiga por esta:
void saveConfig() {
  if (!prefs.begin("codigos_ir", false)) {
    Serial.println("Erro ao acessar a memória NVS");
    return;
  }
  bool ok = true;
  ok &= prefs.putString("ac_on", ac_on);
  ok &= prefs.putString("ac_off", ac_off);
  ok &= prefs.putString("ac_23", ac_23);
  ok &= prefs.putString("tv_on", tv_on);
  ok &= prefs.putString("tv_off", tv_off);
  prefs.end();

  if (!ok) {
    Serial.println("Falha ao salvar configurações de IR na NVS!");
  } else {
    Serial.println("Configurações de IR salvas com sucesso via NVS!");
  }
}

void loadConfig() {
  // Abre o namespace em modo apenas leitura (true)
  if (!prefs.begin("codigos_ir", true)) {
    Serial.println("[NVS] Erro ao acessar a memoria NVS para carregar configuracao.");
    return;
  }
  // O segundo parametro "" e o valor padrao caso a chave nao exista
  ac_on  = prefs.getString("ac_on", "");
  ac_off = prefs.getString("ac_off", "");
  ac_23  = prefs.getString("ac_23", "");
  tv_on  = prefs.getString("tv_on", "");
  tv_off = prefs.getString("tv_off", "");

  prefs.end();

  if (ac_on.length() == 0 && ac_off.length() == 0 && ac_23.length() == 0 && tv_on.length() == 0 && tv_off.length() == 0) {
    Serial.println("[NVS] Nenhuma configuracao de IR encontrada.");
  } else {
    Serial.println("[NVS] Configuracoes de IR carregadas com sucesso!");
  }
}

// Função que a Web chama para recarregar configs do NVS
// Funcao chamada internamente ou pelo sistema para forçar a recarga
void handleReload() {
  Serial.println("[SISTEMA] Solicitando recarga de comandos do disco...");
  loadConfig();
}

void handleSensorAPI() {
  char jsonBuffer[128]; // Removido o 'static' e ajustado o tamanho ideal
  snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{\"temp\":%.1f,\"pres\":%.1f,\"prediction\":%.1f,\"energy\":%.0f}",
    lastTemp, lastPres, predictedTemp, totalPower
  );
  server.send(200, "application/json", jsonBuffer);
}

void handleRelay() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt() - 1; // Ajusta de 1-6 para o índice do array (0-5)
    
    if (id >= 0 && id < 6) {
      bool state = !digitalRead(relayPins[id]); // Inverte o estado atual do relé
      digitalWrite(relayPins[id], state);
      
      // Sincroniza imediatamente com a Alexa/Google Home via RainMaker
      if(relays[id]) {
        relays[id]->updateAndReportParam("Power", state);
      }
      
      atualizarConsumo(); // Atualiza os cálculos de Watts globais
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Erro ID");
}

void SysProvEvent(arduino_event_t *sys_event) {

  switch (sys_event->event_id) {

    case ARDUINO_EVENT_PROV_START:
      Serial.println("\n[BLE] Provisionamento iniciado");
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("\n[WIFI] Credenciais recebidas pelo ESP32");
      Serial.println("[WIFI] Tentando conectar ao roteador...");
      break;

    case ARDUINO_EVENT_PROV_CRED_FAIL:

      Serial.println("\n[ERRO] Falha ao conectar no Wi-Fi!");

      if (sys_event->event_info.prov_fail_reason ==
          NETWORK_PROV_WIFI_STA_AUTH_ERROR) {

        Serial.println("[ERRO] Senha Wi-Fi incorreta ou falha de autenticacao");

      } else {

        Serial.println("[ERRO] Rede Wi-Fi nao encontrada");
      }

      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("\n[SUCESSO] Credenciais aceitas e Wi-Fi conectado!");
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.println("[BLE] Provisionamento finalizado");
      WiFiProv.endProvision();
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("\n[SUCESSO] ESP32 recebeu endereco IP!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      break;

    default:
      break;
  }
}

/*****************************************************************************************
 * SETUP
 *****************************************************************************************/ 
// Aumenta o "balde" de memória da tarefa principal para 16KB (o padrão é 8KB)
  //SET_LOOP_TASK_STACK_SIZE(16 * 1024);
void setup()
{
 Serial.begin(115200);
  delay(1000); // aguarda estabilização de energia após ligar

  Serial.println("\n[RainMaker] Inicializando Node...");

  node = RMaker.initNode("JARVIS_V3");

  // Monta sem formatacao automatica para preservar os dados existentes.
  Serial.println("\n[LittleFS] Montando particao...");
  if (!LittleFS.begin(false)) {
   Serial.println(F("[ERRO] Falha ao montar LittleFS; dados preservados. Continuando sem filesystem."));
  } else {
    Serial.println("[OK] LittleFS montado!");
  }
  // 1. Inicializa Filesystem ANTES de tudo
  //if(!LittleFS.begin(true)) { Serial.println(F("Erro LittleFS")); 
  //} else { loadConfig(); } // Carrega códigos IR salvos

  // 2. Carrega configurações de IR do NVS (Preferences)
  //carregarDados();
  
  // 3. Tenta carregar códigos IR aprendidos
  // Para não dar erro no setup, vamos tentar abrir Preferences sem server.send
  if (!prefs.begin("codigos_ir", true)) {
    Serial.println("Nenhuma config de IR encontrada (primeira inicialização).");
  } else {
    ac_on  = prefs.getString("ac_on", "");
    ac_off = prefs.getString("ac_off", "");
    ac_23  = prefs.getString("ac_23", "");
    tv_on  = prefs.getString("tv_on", "");
    tv_off = prefs.getString("tv_off", "");
    prefs.end();
    Serial.println("Códigos IR carregados do NVS com sucesso.");
  }
 // 4. Inicia o hardware básico
  pinMode(STATUS_LED,OUTPUT);
  pinMode(RESET_BUTTON,INPUT_PULLUP);
  for(int i=0;i<6;i++){
    pinMode(relayPins[i],OUTPUT);
    digitalWrite(relayPins[i],LOW);
  }
 // 5. CONFIGURA O NÓ DO RAINMAKER (AINDA NÃO DÁ START)
  // Todos os dispositivos abaixo são registrados no RainMaker para Alexa/Google.
  // 5. CONFIGURA O NÓ DO RAINMAKER
  Serial.println("\n[DIAGNOSTICO DE MEMORIA]");
  Serial.printf("Heap livre: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap minimo: %u bytes\n", ESP.getMinFreeHeap());
  Serial.printf("PSRAM livre: %u bytes\n", ESP.getFreePsram());

  //Serial.println("\n[RainMaker] Inicializando Node...");

  //node = RMaker.initNode("JARVIS_V3");

  Serial.printf("[MEMORIA] Heap livre apos Node: %u bytes\n",
                ESP.getFreeHeap());

  if (node.getNodeInfo() == nullptr) {
    Serial.println("[ERRO] Node RainMaker nao foi criado!");
  } else {
    Serial.println("[OK] Node RainMaker criado!");
  }

  for(int i=0;i<6;i++){
    relays[i]=new Switch(relayNames[i],&relayPins[i]);
    relays[i]->addCb(write_callback);
    node.addDevice(*relays[i]);
  }

  backlightSwitch = new Switch("Backlight");
  backlightSwitch->addCb(write_callback);
  node.addDevice(*backlightSwitch);

  temp_sensor = new TemperatureSensor("Sensor");
  Param temp_status("Status", "esp.param.name", value("Iniciando..."), PROP_FLAG_READ);
  temp_sensor->addParam(temp_status);
  node.addDevice(*temp_sensor);
  delay(50);

  alt_sensor = new Device("Altimetro","esp.device.light_brightness_ctrl");
  alt_sensor->addParam(Param("Altitude","esp.param.altitude",value(0.0f),PROP_FLAG_READ));
  node.addDevice(*alt_sensor);
  //alt_sensor = new TemperatureSensor("Altimetro");
    //alt_sensor->addTemperatureParam(0.0f);
    //Param alt_status("Status", "esp.param.name", value("Calibrando..."), PROP_FLAG_READ);
    //alt_sensor->addParam(alt_status); // Corrigido para ->
    //node.addDevice(*alt_sensor);
    //delay(50);

  // Criar os Switch somente depois de RMaker.initNode().
  // A classe Switch já cria automaticamente o parâmetro "Power".
  minha_tv = new Switch("TV");
  minha_tv->addCb(write_callback);
  node.addDevice(*minha_tv);

  meu_ar = new Switch("Ar Condicionado");
  meu_ar->addCb(write_callback);
  node.addDevice(*meu_ar);

  // TV e Ar Condicionado já possuem o parâmetro "Power" criado pela classe Switch.
  // 6. Inicializa o provisionamento BLE antes de iniciar o agente RainMaker.
  // Esta é a sequência usada no exemplo oficial da Espressif.
  //WiFiProv.initProvision(
    //NETWORK_PROV_SCHEME_BLE,
    //NETWORK_PROV_SCHEME_HANDLER_FREE_BLE
  //);
  // Registra os eventos antes de começar o provisionamento.
  WiFi.onEvent(SysProvEvent);


  //RMaker.start();
  Serial.println("✓ Conectando ao WiFi AND RainMaker...");
  // 7. Inicia o agente RainMaker depois de registrar Node e dispositivos.
  esp_err_t rmaker_err = RMaker.start();
  Serial.printf("[RainMaker] RMaker.start() retorno: %d\n",
               (int)rmaker_err);

  Serial.println("RainMaker inicializado; iniciando provisionamento BLE...");

  /// 7. INICIA O WI-FI PROVISIONING VIA BLE (Versão corrigida para o core 3.3.12)
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE,NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,NETWORK_PROV_SECURITY_1,pop,service_name);

  Serial.println("\n--- QR CODE ESPRESSIF BLE PROVISIONING ---");

  // Monta o JSON exigido pelo aplicativo Espressif
  String qrText = "{\"ver\":\"v1\",\"name\":\"" +
                  String(service_name) +
                  "\",\"pop\":\"" +
                  String(pop) +
                  "\",\"transport\":\"ble\"}";

  // Codificação URL completa
  String encoded = "";
  const char hex[] = "0123456789ABCDEF";

  for (int i = 0; i < qrText.length(); i++) {
    uint8_t c = (uint8_t)qrText[i];

    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' ||
        c == '.' || c == '~') {

      encoded += (char)c;

    } else {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 0x0F];
    }
  }

  // GERADOR OFICIAL DA ESPRESSIF
  String qrLink =
    "https://espressif.github.io/esp-jumpstart/qrcode.html?data=" +
    encoded;

  Serial.println("\nJSON DO PROVISIONAMENTO:");
  Serial.println(qrText);

  Serial.println("\nABRA ESTE LINK NO NAVEGADOR:");
  Serial.println(qrLink);

  Serial.println("\n------------------------------------------");

  delay(70000); // Desativa delay longo; use temporizador não bloqueante se precisar aguardar.
  
  lcd.init();
  Wire.setClock(100000); // ✅ ADICIONE ESTA LINHA: Garante estabilidade contra ruídos de rede
  lcd.backlight();

  //lcd.print("SMART V1 START");
  // Registra os caracteres (máximo 8)
  lcd.createChar(0, sol);
  lcd.createChar(1, nuvem);
  lcd.createChar(2, chuva);
  lcd.createChar(3, raio);

  if(!bmp.begin()){
    lcd.print("Erro BMP280 SPI");
    Serial.println(F("BMP280 nao encontrado"));
    //while(1);// Se travar aqui, verifique a fiação
  }

 configTzTime("BRT3","pool.ntp.org","time.google.com");
 // MQTT removido: o RainMaker cuidará da integração com Alexa/Google sem MQTT local.
  // Rotas do Servidor
 //server.on("/", handleDashboard);         // Sua rota principal
 // Descomente esta linha no setup() para o Next.js conseguir ler os sensores:
 //server.on("/sensor", handleSensorAPI);   // A rota que o JavaScript chama
 //server.on("/relay", handleRelay);        // Alterado para bater com o nome da função
 //server.on("/sw.js", handleServiceWorker);
 //server.on("/manifest.json", handleManifest);// Registra a rota 
 // Rota para o Next.js ler a temperatura e energia do banco
  server.on("/sensor", handleSensorAPI);   
  // Rota para o Next.js mandar ligar/desligar os relés na bancada
  server.on("/relay", handleRelay);        
  // Rota para o Next.js mandar disparar os comandos de Ar/TV
  server.on("/send", sendCommand);     
  server.on("/learn", learnCommand); // ✅ Garanta que esta linha está ativa!    
  server.begin(); // Inicializa o servidor web de forma enxuta e leve
  Serial.println("✓ Servidor Web do Hub ativado para comandos do Next.js!");

  carregarDados();

  irrecv.enableIRIn();
  irsend.begin();
  // Rotas de configuração e IR
  //server.on("/root", handleRoot);
  //server.on("/learn", learnCommand);
  //server.on("/send", sendCommand);
  //server.on("/save", saveConfig);
  // REGISTRO DA ROTA Carregar cod IR
  //server.on("/reload", HTTP_GET, handleReload); 
  // ----------------------------------
  // Agora que está montado com segurança, carrega as configs
  //carregarDados();
}
/*****************************************************************************************
 * LOOP
 *****************************************************************************************/
void loop()
{
  // --- 1. CONEXÃO E INTEGRAÇÃO NEXT.JS ---
  if(WiFi.status() == WL_CONNECTED) {
    if(!serverStarted) {
      Serial.print("WiFi Conectado! IP: ");
      Serial.println(WiFi.localIP());
      server.begin(); // Mantém o servidor web ativo e leve
      serverStarted = true;
    }
    server.handleClient(); // Processa as rotas locais de forma leve
  }
  // --- 2. PISCA LED DE STATUS (A cada 2 segundos) ---
  static unsigned long ledTimer = 0;
  if(millis() - ledTimer > 2000) {
    ledTimer = millis();
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
  }
  // --- 2.5. ATUALIZA CACHE DO IP WIFI (A cada 60 segundos) ---
  if(WiFi.status() == WL_CONNECTED && millis() - lastWiFiIPUpdate > 60000) {
    lastWiFiIPUpdate = millis();
    snprintf(cachedWiFiIP, sizeof(cachedWiFiIP), "%s", WiFi.localIP().toString().c_str());
  }
  // --- 3. ATUALIZAÇÃO DO LCD (A cada 5 segundos) ---
  if(millis() - lastLCD > 5000) {
    lastLCD = millis();
    screen = (screen + 1) % 8;
    updateLCD(); 
  }
  // ... O restante do seu loop (leitura do BMP280, RainMaker, etc.) continua igual abaixo!
  if(WiFi.status() == WL_CONNECTED) {
        // --- 4. LEITURA DOS SENSORES E NUVEM (A cada 1 minuto) ---
        // Importante: Só lê o sensor de tempos em tempos para não travar o I2C
        if(millis() - lastCloudUpdate > 60000) {
          lastCloudUpdate = millis();
          // ⭐ PROTEÇÃO: Lê sensor apenas se o I2C for válido
          if(bmp.readTemperature() > -50 && bmp.readTemperature() < 150) {
            float t = bmp.readTemperature();
            //float t = lastTemp;
            float a = bmp.readAltitude(1012.90);
            //float a = lastAlt;
            //float pressaoBruta = bmp.readPressure(); // Pressão no local em Pa
            
            float p = bmp.readPressure() / 100.0F; // Pressão no local em hPa;
            //float p = bmp.readSeaLevelPressure(55.0) / 100.0F;// Pressão ao nível do mar em hPa*class Adafruit_BMP280' has no member named 'readSeaLevelPressure'
            /**************Atenção****************************/
            // Atualização da Pressão e Histórico
            if(WiFi.status() == WL_CONNECTED) {
            if (millis() - ultimaLeituraMs >= INTERVALO_HISTORICO || ultimaLeituraMs == 0) {
              //pAtual = bmp.readSeaLevelPressure(55.0) / 100.0F;
              pAtual = p;
              for (int i = 5; i > 0; i--) historico[i] = historico[i-1];
              historico[0] = pAtual;// A leitura mais nova fica na posição 0
              if (amostrasContador < 6) amostrasContador++;
              // Chame sua função de salvar na Flash aqui se desejar
              salvarDadosFlash(); 
              //salvarDados
              
              ultimaLeituraMs = millis();
            }
            
            // Calcula a tendência continuamente para a tela usar
            float pAntiga = (amostrasContador > 0) ? historico[amostrasContador - 1] : pAtual;
            deltaP = pAtual - pAntiga;

            // ✅ CORRIGIDO: Copia resultado const char* para array fixo (evita vazamento de memória)
            // 1. CHAMA A FUNÇÃO E GUARDA O TEXTO (o "return")
            // ✅ DESCOMENTADO: Atualiza a previsão e joga para o LCD de hora em hora!
            strncpy(statusAtual, calcularStatusTempo(pAtual, deltaP), 24);
            statusAtual[24] = '\0';  // Garante null terminator

            }
            /*************************************************/
            // Só envia para o RainMaker se houver mudança significativa e Wi-Fi OK
            if(abs(t - lastTemp) > 0.2 || abs(a - lastAlt) > 1.0) {
              if(WiFi.status() == WL_CONNECTED) {
                temp_sensor->updateAndReportParam("Temperature", t);
                if(alt_sensor) alt_sensor->updateAndReportParam("Altitude", p);
                
                // MQTT removido: não usamos mais publicação local.
                // LÊ O SENSOR APENAS AQUI!
                lastTemp = t;     // Atualiza a variável global que o LCD usa
                lastPres = p;     // Atualiza a pressão
                lastAlt = a;      // Atualiza a altitude
                saveSensorHistory();
                predictTemperature();

                // ============================================================================
                // NOVA INSERÇÃO: ENVIA DADOS PARA O POSTGRESQL SEMPRE QUE O SENSOR ATUALIZAR
                // ============================================================================
                //enviarDadosParaNextJS();
                // ============================================================================

                Serial.printf("Sensor Update - Temp: %.2f C | Alt: %.2f m\n", t, a);
              }
            }
          } else {
            Serial.println("⚠️ Erro ao ler BMP280 - I2C pode estar travado!");
          }
        }
    // --- 5. HISTÓRICO E PREVISÃO (A cada 5 minutos) ---
    static unsigned long lastHistoryUpdate = 0;
    if (millis() - lastHistoryUpdate > 300000) {
      lastHistoryUpdate = millis();
      // Salva histórico e atualiza previsão a cada 5 minutos
      saveSensorHistory();
      predictTemperature();
    }
  }
  // --- 6. BOTÃO DE RESET DE FÁBRICA ---
  // Se segurar por 10 segundos, limpa as configs do RainMaker
  if(digitalRead(RESET_BUTTON) == LOW) {
    delay(100); // Debounce
    int sec = 0;
    while(digitalRead(RESET_BUTTON) == LOW) {
      delay(1000);
      yield(); // ⭐ ALIMENTA O WATCHDOG - CRÍTICO!
      sec++;
      Serial.printf("Reset pressionado: %d seg\n", sec);
      if(sec >= 10) {
        Serial.println("Efetuando Factory Reset...");
        RMakerFactoryReset(2); // Reseta e reinicia em 2 segundos
      }
    }
  }
  //--- 5.5 SINCRONIZAÇÃO PERIÓDICA DO RAINMAKER ---
     //⚠️ COMENTADO TEMPORARIAMENTE - Será ativado após RMaker.start() estar funcionando
     static unsigned long lastSensorSync = 0;
     if(WiFi.status() == WL_CONNECTED && millis() - lastSensorSync > 120000) {
       lastSensorSync = millis();
       if(temp_sensor) {
         temp_sensor->updateAndReportParam("Temperature", lastTemp);
         temp_sensor->updateAndReportParam("Status", "Conectado");
       }
       if(alt_sensor) {
         alt_sensor->updateAndReportParam("Temperature", lastPres);
         //alt_sensor->updateAndReportParam("Status", statusAtual.c_str());
       }
       Serial.println("✓ RainMaker Sync Completo (Valores + Status)");
     }
}