#define NO_CONECTION_LED 14                           //Definimos el pin que nos mostrara en una LED la ausencia de conexion.
#define ACTIVE_LED 15                                //Definimos el pin que nos mostrara en una LED que el sistema esta busy.
#define LOW_POWER_LED 16
#define LOW_PIN 17
#define OPEN_COLECTOR_CAPACITIVE_SENSOR 3
#define RTC_ALARM 2
#define LOCATION "TEC"
#define RST 9

#define FIRMWARE "1.0 ALPHA"

const char* apn = "Internet.tuenti.gt";
const char* user = "";
const char* pass = "";
const char*  broker = "klimato.tk";
const String ATCFUN0 = "AT+CFUN=0";
const String ATCSLK = "AT+CSCLK=2";

const char* topic1 = "/out/WaterSense/M";
const char* topic2 = "/in/WaterSense/D";
const char* topic3 = "/out/WaterSense/T";
const char* topic4 = "/in/WaterSense/R";
const char* topic5 = "/out/WaterSense/Dr";

const char* username = "DENNIS";
const char* password = "FBx_admin2012";
