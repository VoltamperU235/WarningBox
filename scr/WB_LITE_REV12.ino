#include <ArduinoJson.h>
#include "LowPower.h"                                 //Fue nesesaria para Reudcir el consumo de energia mientras la arduino no se esta utilizando (que es aproximadamente un 95% del tiempo :v )
#include <SoftwareSerial.h>                           //Se utilizo SoftwareSerial para el Debuging (En el terminal) debido a que el SIM utiliza el UART0 .
#define TINY_GSM_MODEM_SIM800                         //Como la libreria tinyGSM es generica hay que especificar que Chip se utiliza para la conexion a la red GSM.
#include <TinyGsmCommon.h>
#include <PubSubClient.h>                             //Librerias nesesarias para el correcto funcionamiento del SIM800.
#include <TinyGsmClient.h>
SoftwareSerial mySerial(11, 12);                      //Declaramos los pines por los cuales se comunicara el Arduino con el PC (utilizar conversor USB - TTL)
TinyGsm modem(Serial);                                //Creamos un objeto del tipo modem en el cual indicaremos el UART que se comunicara con el SIM800 en este caso UART0 (Serial) si fuera otra arduino mas poderosa podria ser Serial1, Serial2, etc. :v
TinyGsmClient client(modem);
PubSubClient mqtt(client);
#include "RTClib.h"
#include <DS3232RTC.h>
#include "config.h"
#include <EEPROM.h>

#define RED   NO_CONECTION_LED
#define BLUE  LOW_POWER_LED
#define GREEN ACTIVE_LED

volatile int ESTATE = 0;

long lastReconnectAttempt = 0;
volatile bool LOW_LEVEL = LOW;
int Y;
int M;
int D;
int H;
int MM;
unsigned long previousMillis = 0;        // will store last time LED was updated

unsigned long currentMillis = 0;
char ID[16];
String IMEI = "";
bool ALARM_FLAG = LOW;
bool LOW_FLAG = LOW;
bool SLEEP_LOCK = LOW;
int JSON_CODE = 0;
int RST_COUNT = 0;
unsigned long retry_publish;
unsigned long retry_request;
bool TELEMETRY_PENDING = LOW;
bool MANAGEMENT_PENDING = LOW;
bool DATE_PENDING = LOW;
bool DATE_REQUEST = LOW;
char JSON_SMS[105];

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);                                  //Establecemos la conexion con el SIM800 utilizando el UART0 (Lo utilizamos porque existe la Interupcion Serial nesesaria para sacar a la arduino del modo sleep en el momento que el SIM800 reciba la indicacion de alarma);
  mySerial.begin(115200);
  attachInterrupt(1, WATER_DETECT, FALLING);
  attachInterrupt(0, ALARM, FALLING);
  rtc.begin();
  pinMode(OPEN_COLECTOR_CAPACITIVE_SENSOR, INPUT_PULLUP);
  pinMode(RTC_ALARM, INPUT_PULLUP);
  pinMode(LOW_PIN, OUTPUT);
  digitalWrite(LOW_PIN, LOW);
  pinMode(NO_CONECTION_LED, OUTPUT);                   //El LED permanecera encendido mientras carga el setup y detecta conexion;
  pinMode(ACTIVE_LED, OUTPUT);
  pinMode(LOW_POWER_LED, OUTPUT);
  pinMode(RST, OUTPUT);
  SIM800_RESET();
  delay(555);
  mySerial.println(F("WBREV12"));
  int error = 0;
  while (IMEI == "")
  {
    IMEI = modem.getIMEI();
    error++;
    if (error > 10)
    {
      software_reset();
    }
  }
  ENABLE_COMUNICATION();
  mqtt.subscribe(topic2);
  mqtt.subscribe(topic4);
  if (EEPROM.read(0) == 0)
  {
    JSON(1);
    JSON_CODE = 1;
    MANAGEMENT_PENDING = HIGH;
  }
  else
  {
    JSON(2);
    JSON_CODE = 2;
    TELEMETRY_PENDING = HIGH;
  }
  ESTATE = 3;
}

void loop() {
  switch (ESTATE)
  {
    case 0:
      if (ALARM_FLAG)
      {
        mySerial.println(F("AF"));
        ALARM_FLAG = LOW;
        SLEEP_LOCK = HIGH;
        ESTATE = 4;
      }
      if (LOW_FLAG)
      {
        mySerial.println(F("LF"));
        LOW_FLAG = LOW;
        SLEEP_LOCK = HIGH;
        ESTATE = 2;
      }
      if (!SLEEP_LOCK)
      {
        mySerial.println(F("ZZz"));
        Serial.flush();
        mySerial.flush();
        attachInterrupt(1, WATER_DETECT, FALLING);
        attachInterrupt(0, ALARM, FALLING);
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);
      }
      mySerial.println(F("O-S"));
      break;
    case 1:
      mqtt.disconnect();
      while (!modem.gprsDisconnect())
      {
        delay(55);
      }
      mqtt.disconnect();
      sendData(ATCFUN0, 7555, 1);
      sendData(ATCSLK, 7555, 1);
      delay(555);
      ESTATE = 0;
      break;
    case 2:
      delay(5);
      if (!digitalRead(3))
      {
        EEPROM.write(0, 1);
        DEBUG_BLINK(8, 1, 1, GREEN);
        software_reset();
        ESTATE = 3;
      } else {
        goto_sleep();
      }
      break;
    case 3:
      if (TELEMETRY_PENDING || MANAGEMENT_PENDING)
      {
        if (millis() - retry_publish > 1000) {
          JSON(JSON_CODE);
          RST_COUNT++;
          if (RST_COUNT > 7)
          {
            software_reset();
          }
        }
      }
      if (DATE_PENDING)
      {
        if (millis() - retry_request > 1000) {
          DATE_REQUEST = mqtt.publish(topic5, "ND");
          if (!DATE_REQUEST)
          {
            software_reset();
          } RST_COUNT++;
          if (RST_COUNT > 7)
          {
            software_reset();
          }
        }
      }

      DEBUG_BLINK(2, 1, 250, BLUE);
      mqtt.loop();
      break;
    case 4:
      if (!digitalRead(2))
      {
        if (check())
        {
          mySerial.println(F("A!"));
          software_reset();
        } else
        {
          mySerial.println(F("PA"));
          SET_ALARM();
          goto_sleep();
        }
      } else {
        goto_sleep();
      }

      break;
  }
}

void SIM800_RESET()
{
  digitalWrite(RST, LOW);
  delay(555);
  digitalWrite(RST, HIGH);
}

void ENABLE_COMUNICATION()
{
  mySerial.println(F("W!"));

  if (!modem.waitForNetwork())
  {
    mySerial.println(F("."));
    software_reset();
  }
  if (modem.isNetworkConnected())
  {
    mySerial.println(F("GSM"));      //Mostrara un mensaje cuando de haya conectado a la red.

  }
  DEBUG_BLINK(1, 1, 250, GREEN);
  if (!modem.gprsConnect(apn, user, pass))
  {
    mySerial.println("F");
    software_reset();
  } else
  {
    mySerial.println(F("2G!"));
  }
  DEBUG_BLINK(1, 2, 250, GREEN);
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
  long int time = millis();
  int reset_time = 30000;
  while ( ((time + reset_time) > millis()) && (!mqtt.connected()))
  {
    if (!mqtt.connected())
    {
      unsigned long t = millis();
      if (t - lastReconnectAttempt > 2000L)
      {
        lastReconnectAttempt = t;
        if (mqttConnect())
        {
          lastReconnectAttempt = 0;
        }
      }
      delay(100);
    }
  }
  if (!mqtt.connected())
  {
    software_reset();
  }
  DEBUG_BLINK(1, 3, 250, GREEN);
}

void software_reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  mySerial.print("SMS [");
  mySerial.print(topic);
  mySerial.print("]: ");
  mySerial.write(payload, len);
  mySerial.println();

  String Data = "";
  for (int y = 0; y < len; y ++)
  {
    Data += char(payload[y]);
  }

  if (String(topic) == "/in/WaterSense/R")
  {
    if (Data == IMEI)
    {
      switch (EEPROM.read(0))
      {
        case 0:

          RST_COUNT = 0;
          MANAGEMENT_PENDING = LOW;
          DATE_REQUEST = mqtt.publish(topic5, "ND");
          if (!DATE_REQUEST)
          {
            software_reset();
          }
          DATE_PENDING = HIGH;
          retry_request = millis();

          break;
        case 1:

          EEPROM.write(0, 0);
          RST_COUNT = 0;
          ESTATE = 1;
          TELEMETRY_PENDING = LOW;

          break;

      }
    }

  }

  if (String(topic) == "/in/WaterSense/D")
  {
    int pos1;
    int pos2;
    String Build;
    pos2 = Data.indexOf("-");
    for (int j = 0; j < pos2; j++)
    {
      Build += Data[j];
      Y = Build.toInt();
    }
    Build = "";
    pos1 = pos2;
    pos2 = Data.indexOf("-", pos1 + 1);
    for (int j = pos1 + 1; j < pos2; j++)
    {
      Build += Data[j];
      M = Build.toInt();
    }
    Build = "";
    pos1 = pos2;
    pos2 = Data.indexOf("T");
    for (int j = pos1 + 1; j < pos2; j++)
    {
      Build += Data[j];
      D = Build.toInt();
    }
    Build = "";
    pos1 = pos2;
    pos2 = Data.indexOf(":");
    for (int j = pos1 + 1; j < pos2; j++)
    {
      Build += Data[j];
      H = Build.toInt();
    }
    Build = "";
    pos1 = pos2;
    pos2 = Data.indexOf(":", pos1 + 1);
    for (int j = pos1 + 1; j < pos2; j++)
    {
      Build += Data[j];
      MM = Build.toInt();
    }

    H = H - 6;
    if (H < 0) {
      H = 24 + H;
    }

    rtc.adjust(DateTime(Y, M, D, H, MM, 0));
    mySerial.println(F("TN!"));
    ESTATE = 1;
    DATE_PENDING = LOW;
    RST_COUNT = 0;
    DEBUG_BLINK(1, 4, 250, GREEN);
    ALARMS();
  }
}



boolean mqttConnect()
{
  mySerial.print(F("-> "));
  mySerial.print(broker);

  IMEI.toCharArray(ID, sizeof(IMEI));
  boolean status = mqtt.connect(ID, username, password);

  if (status == false)
  {
    mySerial.println(F(" F"));
    return false;
  }
  mySerial.println(F("!"));
  return mqtt.connected();
}

void ALARMS()
{
  mySerial.println(F("StA"));
  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
  RTC.setAlarm(ALM1_MATCH_MINUTES, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  mySerial.println(F("AS"));
}

String sendData(String command, const int timeout, boolean debug) {     //Funcion que permite enviar comandos AT y esperar su respuesta por determinado tiempo (si Debug esta en HIGH)
  String response = "";                                                 //Reiniciamos nuestra variable.
  mySerial.println(command);
  Serial.println(command);                                              //Mostramos en la PC el comando que enviamos.
  long int time = millis();                                             //Tomamos el valor acutal de millis();
  while ( (time + timeout) > millis()) {                                //Miestras no se sobrepase el timeout esperamos datos y los mostramos en el PC
    while (Serial.available()) {
      char c = Serial.read();
      response += c;
    }
  }
  if (debug) {
    mySerial.print(response);
  }
  return response;
}

bool check()
{
  DateTime now = rtc.now();
  int hora = now.hour();
  if ((hora == 6) || (hora == 12) || (hora == 18) || (hora == 0))
  {
    return true;
  } else
  {
    return false;
  }
}

void SET_ALARM()
{
  // DateTime now = rtc.now();
  //int minute = now.minute();
  /*minute = minute + 3;
    if (minute > 59)
    {
    minute = minute - 59;
    }*/
  mySerial.println(F("AWU"));
  //mySerial.println(minute);
  if ( RTC.alarm(ALARM_1) )
  {
    //RTC.setAlarm(ALM1_MATCH_MINUTES, 0, minute, 0, 1);
    RTC.setAlarm(ALM1_MATCH_MINUTES, 0, 0, 0, 1);
    mySerial.println(F("AS"));
  }
}

void JSON(int code)
{
  StaticJsonBuffer<105> jsonBuffer;                           //Generar un buffer para la String Json que contrendra los datos.
  JsonObject& root = jsonBuffer.createObject();               //Se crea un Objeto del tipo JsomObjet para ingresar los datos.

  root["I"] = IMEI;

  if (code == 1)
  {
    root["P"] = modem.getGsmLocation();
    root["L"] = LOCATION;
    root["B"] = modem.getBattPercent();
  }
  if (code == 2)
  {
    root["LE"] = "0";
  }
  root.printTo(JSON_SMS, sizeof(JSON_SMS));                   //Ingresamos en un array de Char nuestro JSON con los datos ya ingresados.
  mySerial.println("J");
  mySerial.println(String(JSON_SMS));
  mySerial.println("F");

  if (code == 1)
  {
    bool ok = mqtt.publish(topic1, JSON_SMS);
    if (!ok)
    {
      mySerial.println(F("SR"));
      software_reset();
    }
    retry_publish = millis();
    mySerial.println(ok);
  }

  if (code == 2)
  {
    bool ok = mqtt.publish(topic3, JSON_SMS);

    if (!ok)
    {
      mySerial.println(F("SR"));
      software_reset();
    }
    retry_publish = millis();
    mySerial.println(ok);

  }
}

void WATER_DETECT()
{

  if (ESTATE == 0)
  {
    ESTATE = 2;
  } else
  {
    LOW_FLAG = HIGH;
  }

}

void ALARM()
{

  if (ESTATE == 0)
  {
    ESTATE = 4;
  } else
  {
    ALARM_FLAG = HIGH;
  }

}

void goto_sleep()
{
  ESTATE = 0;
  SLEEP_LOCK = LOW;
}

void DEBUG_BLINK(int blinks, int number_of_blinks, int delay_between_blinks, int color)
{
  for (int P = 0; P < number_of_blinks; P++)
  {
    for (int R = 0; R < blinks; R++)
    {
      digitalWrite(color, HIGH);
      delay(80);
      digitalWrite(color, LOW);
      delay(80);
    }
    delay(delay_between_blinks);
  }
}
