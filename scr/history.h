//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////777
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Jueves 13 de Diciembre 2018
   Clase BUG
   luego de reiniciar, al momento de emviar la "Telemetry_Data"
   habian ocaciones en las que no enviaba en mensaje, siendo grave el problema
   se decidio implementar un sistema para verificar si el mensaje habia llegado al broker

   Envio del mensaje
   Espera de la confirmacion (7 intentos)
   Continuar el programar, sino reiniciar.

   Se añadieron las siguientes variables:

   int JSON_CODE = 0;
   int RST_COUNT = 0;
   unsigned long retry_publish;
   bool MQTT_PENDING = LOW;

   Code modifications:

  //////////////////////////////////////////////////////////////
  from:
  mySerial.println(F("WBREV11"));
  ENABLE_COMUNICATION();
  mqtt.subscribe(topic2);
  --------------------------------------------------------------
  to:
  mySerial.println(F("WBREV12"));
  while (IMEI == "")
  {
    IMEI = modem.getIMEI();
  }
  ENABLE_COMUNICATION();
  mqtt.subscribe(topic2);
  mqtt.subscribe(topic4);

  //////////////////////////////////////////////////////////////

  from:
    else
  {
    JSON(2);
    EEPROM.write(0, 0);
    ESTATE = 1;
  }
  ---------------------------------------------------------------
  to:
  else
  {
    JSON(2);
    JSON_CODE = 2;
    MQTT_PENDING = HIGH;
  }

  ESTATE = 3;

  /////////////////////////////////////////////////////////////

  from:
   case 3:
      flash();
      mqtt.loop();
     break;
  --------------------------------------------------------------
  to:
  case 3:
      if (MQTT_PENDING)
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
      flash();
      mqtt.loop();
      break;

  //////////////////////////////////////////////////////////////////
  //Condicion añadida en MQTTcallback

  if (String(topic) == "/in/WaterSense/R")
  {
    if(EEPROM.read(0) == 1)
    {
    if (Data == IMEI)
    {
      EEPROM.write(0, 0);
      RST_COUNT = 0;
      ESTATE = 1;
      MQTT_PENDING = LOW;
    }
    }

  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////777
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  /* Miercoles 19 de Diciembre 2018 3:28 AM
   Clase BUG
   Cuando llegue a mi casa ( :v ) me di cuenta que el sistema estaba loopeado, bug que surgio debido a que yo asumia que el publish si llegaba al broker
   cosa que mi sensei Edwin Kestler dijo que habia probabilidad de que hubiera un envio exitoso pero que el mensaje nunca hubiera llegado al broker
   Entonces WarningBox hacia una solicitud de HORA al NodeRed y se quedaba esperando en un loop la llegada del timestamp, pero que me aseguraba a mi que si habia llegado la solicitud a node
   Por lo que se implemento el mismo sistema del bug anterior (13 de diciembre) esperar en un loop la confirmacion del mensaje y si no, reintentar reenvio...

   Se añadieron las siguientes variables:

   unsigned long retry_request;
   bool MANAGEMENT_PENDING = LOW;
   bool DATE_PENDING = LOW;
   bool DATE_REQUEST = LOW;


   Code modifications:

  ///////////////////////////////////////////////////////////////////////////////////
  Funcion modicicada en void SETUP()
  Permite que el sistema no se quede loopeado si no logra obtener el IMEI ( 10 intentos )

  from:
   while (IMEI == "")
  {
    IMEI = modem.getIMEI();
  }
  -----------------------------------------------------------------------
  to:
   while (IMEI == "")
  {
    IMEI = modem.getIMEI();
    error++;
    if (error > 10)
    {
      software_reset();
    }
  }
  ///////////////////////////////////////////////////////////////////////////////////
  MQTT_LOOP modificado
  Nuevas funciones añadida en MQTT_LOOP     DATE_PENDING = LOW;para evitar bucles infinitos
  from:

  case 3:
      if (MQTT_PENDING)
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
      flash();
      mqtt.loop();
      break;

  ----------------------------------------------------------------------
  to:
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
  ////////////////////////////////////////////////////////////////////////
  Variables agregadas para la habilitacion de escucha y reenvio en MQTT_LOOP
  from:
  if (EEPROM.read(0) == 0)
  {
    JSON(1);
    JSON_CODE = 1;
  }
  else
  {
    JSON(2);
    JSON_CODE = 2;
    TELEMETRY_PENDING = HIGH;
  }
  ESTATE = 3;
  -----------------------------------------------------------------------
  to:
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
  ///////////////////////////////////////////////////////////////////////
  Funcion dentro de MQTT_CALLBACK moficada

  from:
  olvide como era antes :v
  -----------------------------------------------------------------------
  to:
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
  ////////////////////////////////////////////////////////////////////////////
  mofificacion de variable añadida a la condicion if (String(topic) == "/in/WaterSense/D") dentro de MQTT_CALLBACK
  from:
   rtc.adjust(DateTime(Y, M, D, H, MM, 0));
    mySerial.println(F("TN!"));
    ESTATE = 1;
    RST_COUNT = 0;
    DEBUG_BLINK(1, 4, 250, GREEN);
    ALARMS();
  ----------------------------------------------------------------------
  to:
    rtc.adjust(DateTime(Y, M, D, H, MM, 0));
    mySerial.println(F("TN!"));
    ESTATE = 1;
    DATE_PENDING = LOW;
    RST_COUNT = 0;
    DEBUG_BLINK(1, 4, 250, GREEN);
    ALARMS();
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////777
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Miercoles 19 de Diciembre 2018
  Clase UPGRADE
  Se supone que mi progracion ya estaba genial, pero Edwin me dijo que creara una funcion debug led
  en la cual por medio del led RGB se supiera el estado de la progracion y se pudieran identicar errores

  No se añadieron nuevas variables globales
  Nueva funcion añadida

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////777
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
