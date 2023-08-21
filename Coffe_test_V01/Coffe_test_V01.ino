/////----------mejoras-------------//////
/*
''''''''' organizacion y comentacion de codigo
.......... gpio_hold_dis(GPIO_NUM_14); el valor de intensidad no se actualizaba por falta de este comando
.........Reparación de conexion a mysql
.........nueva base de datos para probar multiples sensores UV incluyendo los sensores de Co2


---------pendiente: wifi on request para inicializar el sensor




****************** Conexiones ********************

------------------pin 14: enable sensor UV
------------------Pin 13: Out Sensor UV
------------------pin 12: Sensor de Bateria>
                            GND----[ 20k ]----{ }---[ 20k ]-----Bateria
                                               |
                                               |
                                            Pin 27
----------------- puerto: pin 21(SDA-Verde) 22(SCL-Amarillo)


nota: El circuito se alimenta mediante el pin 3v3, NO USAR *VIN*
**************************************************
*/

#include "time.h"



#include <WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "esp_adc_cal.h"
#include <HTTPClient.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <IPAddress.h>


/// ########################    variables   ###########################//////
/////--------------------------------hora-----------------------////

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8*60*60;
const int   daylightOffset_sec = 3600;
char tim[]= "0000-00-00 00:00:00.000000";
char timG[]= "0000-00-00 00:00:00.000000";
//----------------------Parametros Wifi--------------------////////
const char* ssid = "gamer";
///YX4fchdK2F
const char* passwordWifi = "gvTtqYfg";
WiFiClient client;   

//----------------------Parametros Google Sheets--------------------////////
String GOOGLE_SCRIPT_ID = "AKfycbyLDLUOB4vZO9xH6DltMdqAAFOq79i8OAOg6ee8LddagK6shmJr__2hi_e3ToNjXGLY9A";

//----------------------Parametros Servidor SQL--------------------////////
//IPAddress server_addr(162,241,62,55);  // IP of the MySQL *server* here
char user[] = "terialab_esp32_1";              // MySQL user login username
char password[] = "espectra2017";        // MySQL user login password
char buffer[250]="0.00";
/*
INSERT INTO `coffe_test` (`id`, `time`, `espT`, `temp`, `hum`, `co2_1`, `co2_2`, `temp_co2`, `hum_co2`, `uv1`, `uv2`, `uv3`, `bat`) VALUES (NULL, CURRENT_TIMESTAMP, '2023-07-14 16:40:21', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10');*/
///INSERT INTO `coffe_test` (`id`, `time`, `espT`, `temp`, `hum`, `co2_1`, `uv1`, `uv2`, `uv3`, `bat`) VALUES (NULL, CURRENT_TIMESTAMP, '0000-00-00 00:00:00.000000', '1', '2', '3', '7', '8', '9', '10');
//char INSERT_SQL[] = "INSERT INTO  terialab_coffe.coffe_test (id, time,espT,temp,hum,co2_1,co2_2,temp_co2,hum_co2,uv1,uv2,uv3,bat) VALUES (NULL,CURRENT_TIMESTAMP,%s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f)";
char INSERT_SQL[] = "INSERT INTO  terialab_coffe.coffe_test (id, time,espT,temp,hum,co2_1,uv1,uv2,uv3,bat) VALUES (NULL,CURRENT_TIMESTAMP,%s, %f, %f, %d,%f, %f, %f,%f)";
char query[300];
MySQL_Connection conn(&client);
MySQL_Cursor* cursor;



//----------------------Parametros Deep-sleep--------------------////////
#define minu 15
#define Time_To_Sleep 60   //Time ESP32 will go to sleep (in seconds)
#define S_To_uS_Factor 1000000ULL      //Conversion factor for micro seconds to seconds

//---------------------- Sensores y puertos --------------------////////
          
///>>>>sensor UV
double intensidadUv_1=0; /// variable para obtenener indice UV
double intensidadUv_2=0; /// variable para obtenener indice UV
double intensidadUv_3=0; /// variable para obtenener indice UV
#define enable 26 /// puerto de activación sensor UV [1=enable]
const int UVOUT1 = 13; ///puerto Analogo sensor UV 1
const int UVOUT2 = 12; ///puerto Analogo sensor UV 2
const int UVOUT3 = 14; ///puerto Analogo sensor UV 3

///>>>medidor de bateria
float carga=0;
int pinpow=27;   ///Voltaje sensor pin
///>>>sht30
Adafruit_SHT31 sht30 = Adafruit_SHT31();
float t ;
float h ;

/// ####################################   Funciones  ##################################//////

void googlesheet(String a,String b,String c,String d,String e,String f,String g,String h){   
  /*
  a--espt
  b--temp
  c--hum
  d--co2
  e--uv1
  f--uv2
  g--uv3
  h--battery 
    https://script.google.com/macros/s/AKfycbwP7nAI9-XLGicBHVLYrMAzHE-Vn5txhe2x6GavWWsxhyuVnpktazm92RzTk_yEqTJ-5A/exec?espT=0&temp=1&hum=2&co2_1=3&co2_2=4&co2_t=5&co2_h=6&uv1=7&uv2=8&uv3=9&bat=10
 https://script.google.com/macros/s/AKfycbyQZY3p4NtaK3D_SYzIfePeabxCtHfTA5DTcyEVu3lYSHxNp-yVNNmtEXXYP8t7TMfzkg/exec?espT='2023-07-14 16:40:21'&temp=1&hum=2&co2_1=3&uv1=7&uv2=8&uv3=9&bat=10
  
  
  https://script.google.com/macros/s/AKfycbyLDLUOB4vZO9xH6DltMdqAAFOq79i8OAOg6ee8LddagK6shmJr__2hi_e3ToNjXGLY9A/exec?espT=2023-07-14%2016:40:21&temp=1&hum=2&co2_1=3&uv1=7&uv2=8&uv3=9&bat=10
  
  */ 
    String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"espT=" + a+ "&temp=" + b+"&hum=" + c+"&co2_1=" + d+"&uv1="+e+"&uv2="+f+"&uv3="+g+"&bat="+h;
    Serial.print("POST data to spreadsheet:");
    Serial.println(urlFinal);
    HTTPClient http;
    http.begin(urlFinal.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    //---------------------------------------------------------------------
    //getting response from google sheet
    String payload;
    if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload: "+payload);    
    }
    //---------------------------------------------------------------------
    http.end();
}

double ReadVoltage(int ADC_Pin, int offset){
  //double calibration  = cal;
  double calibration  = 1.201; // Adjust for ultimate accuracy when input is measured using an accurate DVM, if reading too high then use e.g. 0.99, too low use 1.01
  float vref = 1100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  vref = adc_chars.vref; // Obtain the device ADC reference voltage
  return (analogRead(ADC_Pin) / 4095.0) * 3.3 * offset * (1100 / vref) * calibration;

}

double uvIntens(int x){
    ////////// toma el valor promedio de 10 medidas del sensor uv 
    double average_sen=0;
    for (int i = 0; i < 10; i++) {
      
      double sensor = ReadVoltage(x,1); ////conversion Valor-->Voltaje, ya que 1042 [valor Standby del sensor]= ~1004mV y el valor maximo de alimentacion es 3.404V
      average_sen += ((sensor-1.00233)/ 0.119767); //// desplazamiento del valor offset [1.004v ~= 0mW/cm2] | se divide el valor de la pendiente m para convertir a mW/cm2 (2200[mv]-1004[mv])/(10[mw/cm2]-0[mw/cm2])| [despejado de la funcion de recta pendiente del datasheet ML8511_3]
      //delay(50);   
    }
    
    average_sen = (average_sen / 10);  ///obtencion del promedio
    //average_sen= floor(m * 10) / 10.0; /// funcion para despreciar las centesimas [ruido]
    if ( average_sen <= 0) average_sen = 0;////es probable obtener valores negativos durante el set Zero, esta fija a cero todos los valores negativos 
    return  average_sen; ////devuelve a uvIntens el valor de average_sens
    ///////////////////////////////////////////////////
}

double battery(){
  double batt=0;
  for(int i=0;i<10;i++){
    batt+=ReadVoltage(pinpow,2);
  } 
  return ((batt/10));

}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y/%m/%d %H:%M:%S");
  strftime(tim, sizeof(tim),"\'%Y/%m/%d %H:%M:%S\'", &timeinfo);
  
  strftime(timG, sizeof(timG),"%Y/%m/%d   %H:%M:%S", &timeinfo);
  for (int i = 0; timG[i]; ++i) {
    if (timG[i] == ' ') {
        timG[i] = '%';
        timG[i + 1] = '2';
        timG[i + 2] = '0';
    }
}
}



void setup() {

    Serial.begin(115200);
    pinMode(enable,OUTPUT);
    delay(50);
    sht30.begin(0x44);
    sht30.heater(0);

    ////// activa el sensor uv antes de tomar la medida
    gpio_hold_dis(GPIO_NUM_14);
    digitalWrite(enable,HIGH);

    delay(200);

///////////////////Obtencion de variables///////////////////////////
    intensidadUv_1=uvIntens(UVOUT1);
    intensidadUv_2=uvIntens(UVOUT2);
    intensidadUv_3=uvIntens(UVOUT3);
    carga=battery();
    h = sht30.readHumidity();
    t = sht30.readTemperature();


//////////////////////////////////////////////////////////////////////
    if (! isnan(t)) {  // check if 'is not a number'
      Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
    } else { 
      t =0;
      Serial.println("Failed to read temperature");
    }
  
    if (! isnan(h)) {  // check if 'is not a number'
      Serial.print("Hum. % = "); Serial.println(h);
    } else { 
      h =0;
      Serial.println("Failed to read humidity");
    }

    Serial.print("intensidadUv1 = ");
    Serial.println(intensidadUv_1);
    Serial.print("intensidadUv2 = ");
    Serial.println(intensidadUv_2);
    Serial.print("intensidadUv3= ");
    Serial.println(intensidadUv_3);
    Serial.print("Bateria = ");
    Serial.println(carga);
    

    // >>>>>>>>>>>>>> Sleep config <<<<<<<<<<<<<<<////
    digitalWrite(enable,LOW);
    gpio_hold_en(GPIO_NUM_14);
    esp_sleep_enable_timer_wakeup(minu * Time_To_Sleep * S_To_uS_Factor); /// establece los parametros para despertar del modo Sleep

    // >>>>>>>>>>>>>> Conexion WiFi <<<<<<<<<<<<<<<////

    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, passwordWifi);
    Serial.println("\nConnecting");
    unsigned long i=millis();
    while(WiFi.status() != WL_CONNECTED){
        if(millis()>=i+10000){
          Serial.println("Failed to connect");
          esp_deep_sleep_start();
        }
    }
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

  ////////////////----------actualizacion de hora------////////////////
    //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();


  /// >>>>>>>>>>>>>> Acondicionamiento de Variables y metodos <<<<<<<<<<<<<<<////
  /*  
  a--espt
  b--temp
  c--hum
  d--co2
  e--uv1
  f--uv2
  g--uv3
  h--battery 
  */  
  googlesheet(timG,String(t),String(h),"300",String(intensidadUv_1),String(intensidadUv_2),String(intensidadUv_3),String(carga)); //// metodo insert Google Sheets

  sprintf(query,INSERT_SQL,tim,t,h,300,intensidadUv_1,intensidadUv_2,intensidadUv_3,carga);  ///// cadena para SQL
  Serial.println(query);
  Serial.print("Connecting to SQL...  ");
  
  IPAddress server_addr(162,241,62,55);  // IP of the MySQL *server* here

  if (conn.connect(server_addr, 3306, "terialab_esp32_1",  "espectra2017")){
      // create MySQL cursor object
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

    cur_mem->execute(query);
    delete cur_mem;
    Serial.println("OK."); 
  } 
  else Serial.println("FAILED.");




  /// >>>>>>>>>>>>>> Inicio de Deep Sleep <<<<<<<<<<<<<<<////
  esp_deep_sleep_start();
 
}




void loop() {
  ///// ------------calibracion------------///
    /*double sensor1=0;
    for(int i=0;i<10;i++){
     sensor1 += ReadVoltage(UVOUT,1); ////conversion Valor-->Voltaje, ya que 1042 [valor Standby del sensor]= ~1004mV y el valor maximo de alimentacion es 3.404V  
    }
    sensor1=sensor1/10;

    

    Serial.println("   Adjusted voltage = " + String(sensor1,4) + "v");
    delay(5000);*/
}
