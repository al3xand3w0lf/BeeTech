



void requestHiveData(int num_beehive){


  if( num_beehive == 0)
      getLocalSensorData(0); 
  else
      getNetworkSensorData(num_beehive);    // call for sensor data     
}



void getNetworkSensorData(int num_beehive){

  //Serial.println("");
  //Serial.println("-----------------------------");
  //Serial.print("Request Data from Hive: ");
  //Serial.println(num_beehive);

  CAN.beginPacket(num_beehive);
  CAN.write(rtc.getDay());
  CAN.write(rtc.getMonth());
  CAN.write(rtc.getYear());
  CAN.write(rtc.getHours());
  CAN.write(rtc.getMinutes());
  CAN.write(rtc.getSeconds());
  CAN.endPacket();

  //Serial.println("Sending CAN packet success");
  //Serial.println("-----------------------------");
}

/*
  float tempC = sensor2.getTempC(deviceAddress);
  Serial.print("Temp Sensor_2 C: ");
  Serial.println(tempC);

 */


void getLocalSensorData(int num_beehive){
  float WEIGHT_SENSOR_float = 0.0, TempIntern_float = 0.0;

  if( TEMP1_CON == SENSOR_CONNECTED){
      sensor1.requestTemperatures(); // Send the command to get temperatures
      TempIntern_float = getTemperature_sensor1(insideThermometer1); // Use a simple function to print out the data
      //Serial.print("Temp Sensor_1 in getLocalSensorData A C: ");
      //Serial.println(TempIntern_float);
      BEEHIVE[num_beehive].temp1_float = TempIntern_float; // Use a simple function to print out the data
      //Serial.print("Temp Sensor_1 in getLocalSensorData B C: ");
      //Serial.println(BEEHIVE[num_beehive].temp1_float);
      }
  if( TEMP2_CON == SENSOR_CONNECTED){
      sensor2.requestTemperatures(); // Send the command to get temperatures
      TempIntern_float = getTemperature_sensor2(insideThermometer2);
      //Serial.print("Temp Sensor_2 in getLocalSensorData A C: ");
      //Serial.println(TempIntern_float);
      BEEHIVE[num_beehive].temp2_float = TempIntern_float; // Use a simple function to print out the data
      //Serial.print("Temp Sensor_2 in getLocalSensorData B C: ");
      //Serial.println(BEEHIVE[num_beehive].temp2_float);
      }
  if( SCALE_CON == SENSOR_CONNECTED){
      WEIGHT_SENSOR_float = scale_readings();
      //BEEHIVE[num_beehive].weight_float = scale_readings();
      }
  BEEHIVE[num_beehive].weight_float = WEIGHT_OFFSET_float + WEIGHT_SENSOR_float;
  /*
  if(BEEHIVE[num_beehive].weight_float < 0.0)
     BEEHIVE[num_beehive].weight_float = 0.0;        // in case correction factor from SD CArd is slightly different
  */
  
  BEEHIVE[num_beehive].sound_int = getSound(analogSoundPin) ; // analogRead(analogSoundPin);      
}
