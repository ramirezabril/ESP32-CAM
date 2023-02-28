/* Código para accionar la cámara de la ESP32-CAM
 * Inicia el web server la de ESP32-CAM
 * Envia y recibe datos de firebase
 * Fecha: 28/02/2023
 * Autor: Abril Ramírez Gómez
*/
#include <Arduino.h> //Librerias a utilizar
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h> //Librería para conectar con firebase

#include "esp_camera.h"

#include "addons/TokenHelper.h" //Proporciona información sobre el proceso de generación de tokens
#include "addons/RTDBHelper.h" //Proporciona la información de impresión de la carga útil RTDB y otras funciones de ayuda

#define WIFI_SSID "CGA2121_G5ezGL2" //Nombre de la red wifi
#define WIFI_PASSWORD "Macc2023@?76" //Credenciales de la red

#define API_KEY "AIzaSyCH_TBm-YjcVNmn1orCPsN3ADdoJjgxU0E" // ApiKey de Firebase - Clave secreta

#define USER_EMAIL "controlacc767@gmail.com" //Email con el que se estará logueando
#define USER_PASSWORD "Accmex767" //Contraseña para iiciar sesión

#define DATABASE_URL "https://camaras-8a244-default-rtdb.firebaseio.com/" //URL de Firebase RTDB

#define CAMERA_MODEL_AI_THINKER // Has PSRAM // Definir la cámara a utilizar

#include "camera_pins.h"

//Definir objeto de datos Firebase
FirebaseData fbdo;
FirebaseAuth auth;  
FirebaseConfig configu;


const char* ssid = "CGA2121_G5ezGL2";
const char* password = "Macc2023@?76";

  
String uid;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

int count;
int contador;
int camara3;
int camara4;
int camara5;
int camara6;
int valorLDR;
int Led = 2;

void startCameraServer();

void setup() { //Inicio del setup
  Serial.begin(115200);
  pinMode(33,INPUT); //----FOTORESISTENCIA-----
  pinMode(Led , OUTPUT); // LED
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

  #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  #endif

  WiFi.begin(ssid, password); //Conexión a WiFi

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://"); //WebServer
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  //Conexión a Firebase
  configu.api_key = API_KEY; //Asignando la ApiKey que se requiere a la configuración

  auth.user.email = USER_EMAIL; //Asignando las credenciales del usuario para la autentificación
  auth.user.password = USER_PASSWORD;

  configu.database_url = DATABASE_URL; //Asignando la RTDB URL de Firebase

  configu.token_status_callback = tokenStatusCallback;//Asignar la función de devolución de llamada para la tarea de generación de fichas de larga duración 
  
  Firebase.begin(&configu, &auth); //Iniciando Firebase a tráves de la configuración y autenticación
  Firebase.reconnectWiFi(true); //Reconexión de WiFi para Firebase
  fbdo.setResponseSize(4096); //Asignación al Firebase Data

  // Obtener el UID del usuario puede tardar unos segundos
  Serial.println("Obtener el UID del usuario");
  while ((auth.token.uid) == "") { //A tráves del token
    Serial.print('.');
    delay(1000);
  }
  
}//Fin del setup

void loop() { // Inicio del loop
   
// -------------------------------COMPROBAR TOKEN -----------------------------  
   if (Firebase.isTokenExpired()){ //Verificación de si ha expirado el token o no
      Firebase.refreshToken(&configu); 
      Serial.println("Refrescar token"); //Iniciar de nuevo el token
   }
// -------------------------------COMPROBAR TOKEN ----------------------------- 




//--------------------------------FOTORESISTENCIA------------------------------
   valorLDR = analogRead(33);                
   //Serial.println(valorLDR);
   delay(500);

  //Establecer rangos de tempratura de cauerdo al valor de la fotoresistencia
   if ((valorLDR<2460)){
    count = random(15, 33);
   }
   if ((valorLDR>2460) && (valorLDR<2464)){
    count = random(15, 33);
   }
   if ((valorLDR>2464) && (valorLDR<2468)){
    count = random(15, 33);
   }
   if ((valorLDR>2468) && (valorLDR<2472)){
    count = random(15, 33);
   }
   if ((valorLDR>2472) && (valorLDR<2476)){
    count = random(15, 33);
   }
   if ((valorLDR>2476)){
    count = random(15, 33);
   }
  Serial.println(count);
//--------------------------------FOTORESISTENCIA------------------------------






//--------------------------------ENVIAR A FIREBASE -------------------------------------
//CAMARA1--------------

if (Firebase.RTDB.setInt(&fbdo, "CAMARA1/Temperatura/Sensor1/Valor", count)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 1"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 1");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }

//CAMARA2-------------
contador = random (-50,-10);
Serial.println(contador);
if (Firebase.RTDB.setInt(&fbdo, "CAMARA2/Temperatura/Sensor1/Valor", contador)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 2"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 2");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }


//CAMARA3-------------
camara3 = random (-50,-10);
Serial.println(camara3);
if (Firebase.RTDB.setInt(&fbdo, "CAMARA3/Temperatura/Sensor1/Valor", camara3)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 2"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 2");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }


//CAMARA4-------------
camara4 = random (-50,-10);
Serial.println(camara4);
if (Firebase.RTDB.setInt(&fbdo, "CAMARA4/Temperatura/Sensor1/Valor", camara4)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 2"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 2");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }


//CAMARA5-------------
camara5 = random (-50,-10);
Serial.println(camara5);
if (Firebase.RTDB.setInt(&fbdo, "CAMARA5/Temperatura/Sensor1/Valor", camara5)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 2"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 2");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }


//CAMARA6-------------
camara6 = random (-50,-10);
Serial.println(camara6);
if (Firebase.RTDB.setInt(&fbdo, "CAMARA6/Temperatura/Sensor1/Valor", camara6)){ //Escribe un número Int en la ruta de la base de datos - test/int
      Serial.println("PASSED 2"); //Se realizó correctamente
     // Serial.println("PATH: " + fbdo.dataPath()); //Da la dirección del PATH
     // Serial.println("TYPE: " + fbdo.dataType()); //Indica el tipo de dato ingresado con Type
   }
else { //Da un error 
     Serial.println("FAILED 2");
      //Serial.println("REASON: " + fbdo.errorReason()); //Muestra el error correspondiente
    }
 //--------------------------------ENVIAR A FIREBASE -------------------------------------






//-------------------------------LED--------------------------------
   int Value;
   if (Firebase.RTDB.getInt(&fbdo, "/CAMARA1/Equipos/Led")) {
      if (fbdo.dataType() == "int") {
        Value = fbdo.intData();
        Serial.println(Value);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }

   if (Value==1) {
      digitalWrite(Led , HIGH);   // poner el Pin en HIGH 
      delay(500);
    } 
    if (Value==0){
      digitalWrite(Led , LOW);   // poner el Pin en HIGH
    }
//------------------------------LED----------------------------------

} //Fin del loop
