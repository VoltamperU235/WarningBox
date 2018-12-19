

/* Jueves 13 de Diciembre 2018
 * Clase Bug:
 * luego de reiniciar, al momento de emviar la "Telemetry_Data"
 * habian ocaciones en las que no enviaba en mensaje, siendo grave el problema
 * se decidio implementar un sistema para verificar si el mensaje habia llegado al broker
 * 
 * Envio del mensaje
 * Espera de la confirmacion (7 intentos)
 * Continuar el programar, sino reiniciar.
 * 
 * Se añadieron las siguientes variables:
 * 
 * int JSON_CODE = 0;
 * int RST_COUNT = 0;
 * unsigned long retry_publish;
 * bool MQTT_PENDING = LOW;
 * 
 * Code modifications:
 * 
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

/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
 */
