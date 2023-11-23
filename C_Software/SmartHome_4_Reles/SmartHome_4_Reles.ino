/**********************************************************************************
 Código para un dispositivo de domótica para el control de iluminación LED.
 Utiliza ESP RainMaker en la nube + Bluetooth + IR + pulsadores manuales con 4 relés
 Placa de desarrollo ESP32
 **********************************************************************************/

#include "RMaker.h"      // Incluye la biblioteca ESP RainMaker
#include "WiFi.h"         // Incluye la biblioteca WiFi
#include "WiFiProv.h"     // Incluye la biblioteca para la provisión de WiFi
#include <IRremote.h>     // Incluye la biblioteca para el control remoto IR
#include <AceButton.h>    // Incluye la biblioteca AceButton para el manejo de botones
using namespace ace_button;

const char *service_name = "2.4G-D160_EXT";   // Nombre del servicio WiFi
const char *pop = "8E0245D170";               // Clave de la red WiFi

// Define el ID del chip ESP32
uint32_t espChipId = 0;

// Define el nombre del nodo
char nodeName[] = "ESP32 Smart-Home";

// Define los nombres de los dispositivos
char deviceName_1[] = "Switch1";
char deviceName_2[] = "Switch2";
char deviceName_3[] = "Switch3";
char deviceName_4[] = "Switch4";

// Actualiza el código HEX de los botones del control remoto IR (0x<HEX CODE>)
#define IR_Button_1 0x80BF49B6
#define IR_Button_2 0x80BFC936
#define IR_Button_3 0x80BF33CC
#define IR_Button_4 0x80BF718E
#define IR_All_Off  0x80BF3BC4

// Define los pines GPIO conectados a relés y interruptores
static uint8_t RelayPin1 = 23;  // D23
static uint8_t RelayPin2 = 22;  // D22
static uint8_t RelayPin3 = 21;  // D21
static uint8_t RelayPin4 = 19;  // D19

static uint8_t SwitchPin1 = 13;  // D13
static uint8_t SwitchPin2 = 12;  // D12
static uint8_t SwitchPin3 = 14;  // D14
static uint8_t SwitchPin4 = 27;  // D27

static uint8_t wifiLed = 2;      // D2
static uint8_t gpio_reset = 0;   // Presiona BOOT para resetear el WiFi
static uint8_t IR_RECV_PIN = 35; // D35 (Pin del receptor IR)

static uint8_t RX2Pin = 16;      // RX2
static uint8_t TX2Pin = 17;      // TX2

/* Variables para leer el estado de los pines */
bool toggleState_1 = LOW; // Define una variable booleana para recordar el estado de alternancia para el relé 1
bool toggleState_2 = LOW; // Define una variable booleana para recordar el estado de alternancia para el relé 2
bool toggleState_3 = LOW; // Define una variable booleana para recordar el estado de alternancia para el relé 3
bool toggleState_4 = LOW; // Define una variable booleana para recordar el estado de alternancia para el relé 4

String bt_data = ""; // Variable para almacenar datos de Bluetooth

IRrecv irrecv(IR_RECV_PIN); // Objeto para recibir señales infrarrojas
decode_results results;    // Almacena los resultados de la decodificación IR

ButtonConfig config1; // Configuración del botón 1
AceButton button1(&config1); // Objeto para el botón 1
ButtonConfig config2; // Configuración del botón 2
AceButton button2(&config2); // Objeto para el botón 2
ButtonConfig config3; // Configuración del botón 3
AceButton button3(&config3); // Objeto para el botón 3
ButtonConfig config4; // Configuración del botón 4
AceButton button4(&config4); // Objeto para el botón 4

void handleEvent1(AceButton*, uint8_t, uint8_t); // Función de manejo de eventos para el botón 1
void handleEvent2(AceButton*, uint8_t, uint8_t); // Función de manejo de eventos para el botón 2
void handleEvent3(AceButton*, uint8_t, uint8_t); // Función de manejo de eventos para el botón 3
void handleEvent4(AceButton*, uint8_t, uint8_t); // Función de manejo de eventos para el botón 4

// El framework proporciona algunos tipos de dispositivos estándar como interruptor, bombilla, ventilador, sensor de temperatura.
static Switch my_switch1(deviceName_1, &RelayPin1); // Objeto para el interruptor 1
static Switch my_switch2(deviceName_2, &RelayPin2); // Objeto para el interruptor 2
static Switch my_switch3(deviceName_3, &RelayPin3); // Objeto para el interruptor 3
static Switch my_switch4(deviceName_4, &RelayPin4); // Objeto para el interruptor 4

void sysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
            Serial.printf("\nInicio de aprovisionamiento con nombre \"%s\" y PoP \"%s\" en BLE\n", service_name, pop);
            printQR(service_name, pop, "ble");
#else
            Serial.printf("\nInicio de aprovisionamiento con nombre \"%s\" y PoP \"%s\" en SoftAP\n", service_name, pop);
            printQR(service_name, pop, "softap");
#endif
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.printf("\n¡Conectado a Wi-Fi!\n");
            digitalWrite(wifiLed, true);
            break;
    }
}

// Callback de escritura para dispositivos
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
    // Obtiene el nombre del dispositivo y el nombre del parámetro
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    // Comprueba el nombre del dispositivo para determinar la acción a realizar
    if(strcmp(device_name, deviceName_1) == 0) {
      
      // Imprime el estado de la bombilla
      Serial.printf("Bombilla = %s\n", val.val.b? "true" : "false");
      
      // Comprueba el nombre del parámetro y actualiza el estado del relé correspondiente
      if(strcmp(param_name, "Power") == 0) {
          Serial.printf("Valor recibido = %s para %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
        toggleState_1 = val.val.b;
        (toggleState_1 == false) ? digitalWrite(RelayPin1, HIGH) : digitalWrite(RelayPin1, LOW);
        param->updateAndReport(val);
      }
      
    } else if(strcmp(device_name, deviceName_2) == 0) {
      
      // Imprime el valor del interruptor
      Serial.printf("Valor del interruptor = %s\n", val.val.b? "true" : "false");

      // Comprueba el nombre del parámetro y actualiza el estado del relé correspondiente
      if(strcmp(param_name, "Power") == 0) {
        Serial.printf("Valor recibido = %s para %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
        toggleState_2 = val.val.b;
        (toggleState_2 == false) ? digitalWrite(RelayPin2, HIGH) : digitalWrite(RelayPin2, LOW);
        param->updateAndReport(val);
      }
  
    } else if(strcmp(device_name, deviceName_3) == 0) {
      
      // Imprime el valor del interruptor
      Serial.printf("Valor del interruptor = %s\n", val.val.b? "true" : "false");

      // Comprueba el nombre del parámetro y actualiza el estado del relé correspondiente
      if(strcmp(param_name, "Power") == 0) {
        Serial.printf("Valor recibido = %s para %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
        toggleState_3 = val.val.b;
        (toggleState_3 == false) ? digitalWrite(RelayPin3, HIGH) : digitalWrite(RelayPin3, LOW);
        param->updateAndReport(val);
      }
  
    } else if(strcmp(device_name, deviceName_4) == 0) {
      
      // Imprime el valor del interruptor
      Serial.printf("Valor del interruptor = %s\n", val.val.b? "true" : "false");

      // Comprueba el nombre del parámetro y actualiza el estado del relé correspondiente
      if(strcmp(param_name, "Power") == 0) {
        Serial.printf("Valor recibido = %s para %s - %s\n", val.val.b? "true" : "false", device_name, param_name);
        toggleState_4 = val.val.b;
        (toggleState_4 == false) ? digitalWrite(RelayPin4, HIGH) : digitalWrite(RelayPin4, LOW);
        param->updateAndReport(val);
      }  
    } 
}

// Función para el control remoto IR
void ir_remote(){
  if (irrecv.decode(&results)) {
      switch(results.value){
          case IR_Button_1:  
            digitalWrite(RelayPin1, toggleState_1);
            toggleState_1 = !toggleState_1;
            my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
            delay(100);            
            break;
          case IR_Button_2:  
            digitalWrite(RelayPin2, toggleState_2);
            toggleState_2 = !toggleState_2;
            my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
            delay(100);            
            break;
          case IR_Button_3:  
            digitalWrite(RelayPin3, toggleState_3);
            toggleState_3 = !toggleState_3;
            my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
            delay(100);            
            break;
          case IR_Button_4:  
            digitalWrite(RelayPin4, toggleState_4);
            toggleState_4 = !toggleState_4;
            my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);
            delay(100);            
            break;
          case IR_All_Off:
            all_SwitchOff();  
            break;
          default : break;         
        }   
        //Serial.println(results.value, HEX);    
        irrecv.resume();   
  } 
}

// Función para controlar dispositivos mediante Bluetooth
void bluetooth_control()
{
  // Verifica si hay datos disponibles en el puerto serie Bluetooth
  if(Serial2.available()) {
    // Lee los datos disponibles en el puerto serie Bluetooth
    bt_data = Serial2.readString();
    Serial.println(bt_data.substring(bt_data.lastIndexOf(",") + 1));
    
    // Realiza acciones basadas en los datos recibidos
    if (bt_data.substring(bt_data.lastIndexOf(",") + 1) == "A1"){
      // Si se recibe "A1", enciende el Relé1
      digitalWrite(RelayPin1, LOW);  
      toggleState_1 = 1; 
      my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "A0"){
      // Si se recibe "A0", apaga el Relé1
      digitalWrite(RelayPin1, HIGH);  
      toggleState_1 = 0; 
      my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "B1"){
      // Si se recibe "B1", enciende el Relé2
      digitalWrite(RelayPin2, LOW);  
      toggleState_2 = 1; 
      my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "B0"){
      // Si se recibe "B0", apaga el Relé2
      digitalWrite(RelayPin2, HIGH);  
      toggleState_2 = 0; 
      my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "C1"){
      // Si se recibe "C1", enciende el Relé3
      digitalWrite(RelayPin3, LOW);  
      toggleState_3 = 1; 
      my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "C0"){
      // Si se recibe "C0", apaga el Relé3
      digitalWrite(RelayPin3, HIGH);  
      toggleState_3 = 0; 
      my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "D1"){
      // Si se recibe "D1", enciende el Relé4
      digitalWrite(RelayPin4, LOW);  
      toggleState_4 = 1; 
      my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "D0"){
      // Si se recibe "D0", apaga el Relé4
      digitalWrite(RelayPin4, HIGH);  
      toggleState_4 = 0; 
      my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "Z1"){    
      // Si se recibe "Z1", enciende todos los Relés
      all_SwitchOn();
    }
    else if(bt_data.substring(bt_data.lastIndexOf(",") + 1) == "Z0"){    
      // Si se recibe "Z0", apaga todos los Relés
      all_SwitchOff();
    }
  } 
}

// Función para apagar todos los relés
void all_SwitchOff(){
  toggleState_1 = 0; digitalWrite(RelayPin1, LOW); my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1); delay(100);
  toggleState_2 = 0; digitalWrite(RelayPin2, LOW); my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2); delay(100);
  toggleState_3 = 0; digitalWrite(RelayPin3, LOW); my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3); delay(100);
  toggleState_4 = 0; digitalWrite(RelayPin4, LOW); my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4); delay(100);
}

// Función para encender todos los relés
void all_SwitchOn(){
  toggleState_1 = 1; digitalWrite(RelayPin1, HIGH); my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1); delay(100);
  toggleState_2 = 1; digitalWrite(RelayPin2, HIGH); my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2); delay(100);
  toggleState_3 = 1; digitalWrite(RelayPin3, HIGH); my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3); delay(100);
  toggleState_4 = 1; digitalWrite(RelayPin4, HIGH); my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4); delay(100);
}

void setup()
{
    // Inicialización de la comunicación serial a una velocidad de 115200 bps
    Serial.begin(115200);
    
    // Inicialización de la comunicación serial 2 a una velocidad de 9600 bps con configuración de 8 bits de datos, sin paridad y 1 bit de parada
    Serial2.begin(9600, SERIAL_8N1, RX2Pin, TX2Pin);

    // Configuración de los pines de los relés como salida
    pinMode(RelayPin1, OUTPUT);
    pinMode(RelayPin2, OUTPUT);
    pinMode(RelayPin3, OUTPUT);
    pinMode(RelayPin4, OUTPUT);
    pinMode(wifiLed, OUTPUT);

    // Configuración de los pines de los interruptores como entrada con resistencia de pull-up
    pinMode(SwitchPin1, INPUT_PULLUP);
    pinMode(SwitchPin2, INPUT_PULLUP);
    pinMode(SwitchPin3, INPUT_PULLUP);
    pinMode(SwitchPin4, INPUT_PULLUP);
    pinMode(gpio_reset, INPUT);

    // Establecimiento de los estados iniciales de los relés al arrancar
    digitalWrite(RelayPin1, !toggleState_1);
    digitalWrite(RelayPin2, !toggleState_2);
    digitalWrite(RelayPin3, !toggleState_3);
    digitalWrite(RelayPin4, !toggleState_4);
    digitalWrite(wifiLed, LOW);

    // Habilitación del receptor de infrarrojos (IR)
    irrecv.enableIRIn();

    // Configuración de manejadores de eventos para los botones
    config1.setEventHandler(button1Handler);
    config2.setEventHandler(button2Handler);
    config3.setEventHandler(button3Handler);
    config4.setEventHandler(button4Handler);

    // Inicialización de objetos Button con los pines de los interruptores
    button1.init(SwitchPin1);
    button2.init(SwitchPin2);
    button3.init(SwitchPin3);
    button4.init(SwitchPin4);

    // Inicialización del nodo y asignación del nombre
    Node my_node;
    my_node = RMaker.initNode(nodeName);

    // Configuración de devoluciones de llamada para los dispositivos de interruptores
    my_switch1.addCb(write_callback);
    my_switch2.addCb(write_callback);
    my_switch3.addCb(write_callback);
    my_switch4.addCb(write_callback);

    // Agregar dispositivos de interruptores al nodo
    my_node.addDevice(my_switch1);
    my_node.addDevice(my_switch2);
    my_node.addDevice(my_switch3);
    my_node.addDevice(my_switch4);

    // Opcional: habilitar OTA (Over-The-Air) utilizando parámetros
    RMaker.enableOTA(OTA_USING_PARAMS);

    // Opcional: habilitar el servicio de zona horaria
    RMaker.enableTZService();

    // Opcional: habilitar programación
    RMaker.enableSchedule();

    // Obtención del identificador único del chip y del nombre del servicio
    for (int i = 0; i < 17; i = i + 4)
    {
        espChipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    // Impresión del identificador del chip y el nombre del servicio
    Serial.printf("\nChip ID: %d Service Name: %s\n", espChipId, service_name);

    // Mensaje de inicio del programa
    Serial.printf("\nStarting ESP-RainMaker\n");
    RMaker.start();

    // Configuración del evento de WiFi
    WiFi.onEvent(sysProvEvent);

    // Inicio del proceso de aprovisionamiento WiFi
#if CONFIG_IDF_TARGET_ESP32
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#else
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#endif

    // Actualización y reporte de parámetros de los interruptores
    my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
}

void loop()
{
    // Llamada a las funciones de control remoto infrarrojo y control Bluetooth
    ir_remote();
    bluetooth_control();
}

// Manejador del botón 1
void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Imprimir mensaje de evento 1 en el puerto serie
  Serial.println("EVENT1");

  // Manejar el evento según el tipo de evento
  switch (eventType) {
    // Caso en que el botón es liberado
    case AceButton::kEventReleased:
      // Cambiar el estado del relé 1
      digitalWrite(RelayPin1, toggleState_1);
      // Invertir el estado del interruptor 1
      toggleState_1 = !toggleState_1;
      // Actualizar y reportar el parámetro del interruptor 1
      my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
      break;
  }
}

// Manejador del botón 2
void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Imprimir mensaje de evento 2 en el puerto serie
  Serial.println("EVENT2");

  // Manejar el evento según el tipo de evento
  switch (eventType) {
    // Caso en que el botón es liberado
    case AceButton::kEventReleased:
      // Cambiar el estado del relé 2
      digitalWrite(RelayPin2, toggleState_2);
      // Invertir el estado del interruptor 2
      toggleState_2 = !toggleState_2;
      // Actualizar y reportar el parámetro del interruptor 2
      my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
      break;
  }
}

// Manejador del botón 3
void button3Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Imprimir mensaje de evento 3 en el puerto serie
  Serial.println("EVENT3");

  // Manejar el evento según el tipo de evento
  switch (eventType) {
    // Caso en que el botón es liberado
    case AceButton::kEventReleased:
      // Cambiar el estado del relé 3
      digitalWrite(RelayPin3, toggleState_3);
      // Invertir el estado del interruptor 3
      toggleState_3 = !toggleState_3;
      // Actualizar y reportar el parámetro del interruptor 3
      my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
      break;
  }
}

// Manejador del botón 4
void button4Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Imprimir mensaje de evento 4 en el puerto serie
  Serial.println("EVENT4");

  // Manejar el evento según el tipo de evento
  switch (eventType) {
    // Caso en que el botón es liberado
    case AceButton::kEventReleased:
      // Cambiar el estado del relé 4
      digitalWrite(RelayPin4, toggleState_4);
      // Invertir el estado del interruptor 4
      toggleState_4 = !toggleState_4;
      // Actualizar y reportar el parámetro del interruptor 4
      my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);
      break;
  }
}