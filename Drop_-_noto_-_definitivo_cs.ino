
/***********************************************************************
***********************************************************************
 *           
 *           **********      **********     **********
 *           **********      **********     **********
 *           **                  **         **
 *           **                  **         **
 *           **   *****          **         **********
 *           **   *****          **         **********
 *           **      **          **         ** 
 *           **      **          **         **
 *           **********  **  ********** **  **********
 *           **********  **  ********** **  **********
 *               GRUPO DE INNOVACIÓN EDUCATIVA
************************************************************************ 
************************************************************************
                    ESTACIÓN METEOROLÓGICA
     ESTE PROYECTO HA SIDO REALIZADO PARA EL PROYECTO GLOBE - CLIMA
                        MARZO DE 2017
*/
#include <ThingSpeak.h> // Esta librería nos va a permitir enviar datos a Thinkspeak
#include <DHT.h> // Librería encargada de los sensores de temperatura y humedad
#include <math.h> // La math.h nos permite hacer potencias usadas al calcular el punto de rocío (dew point)
#include <Adafruit_BMP085.h> //Esta librería da cobertura a los BMP 180 (sensores de presión)
#include <ESP8266WiFi.h> //Esta otra habilita los comandos de la wifi
#define DHTPIN D3 //conectamos el pin de datos del DHT22 en el pin D4
#define DHTTYPE DHT22 //Especificamos el tipo de sensor de temperatura y humedad. En nuestro caso un DHT22


// Inicializamos el DHT22
DHT dht(DHTPIN, DHTTYPE);
int EntradaUV = A0; //Puerto donde conectamos la entrada del sensor ml8511
Adafruit_BMP085 bmp;
WiFiClient client; // Inicializamos el servicio de Wifi
// HABILITAMOS Y CONFIGURAMOS EL SERVICIO DE THINKSPEAK
unsigned long myChannelNumber = Nº de canal; //Aquí es donde anotamos el número de nuestro canal en thinkspeak
const char * myWriteAPIKey = "API del canal"; //Y esta es nuestra API que nos la dan ellos al contratar el servicio
// HABILITAMOS NUESTRA RED WIFI
const char* ssid = "SSID de nuestra red";
const char* password = "PASS de nuestra red";
const char* server = "api.thingspeak.com";

void setup()
{
  ThingSpeak.begin(client); // Abrimos el servidor web de thinkspeak
  Serial.begin(115200); // Establecemos la velocidad de la comunicación en 115200 bps
  pinMode(EntradaUV, INPUT); //Decimos al sensor ml8511 (Rayos uva) cuáles son sus entradas a través de arduino
  WiFi.begin(ssid, password); //Entramos en nuestra wifi con nuestra ssid y contraseña
  //Establecemos un  mensaje a través del puerto serie por si algo fallase y también para cerciorarnos de que usamos
  //nuestra red wifi
  Serial.println();
  Serial.println();
  Serial.print("Conectando con ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) // Si  no entrásemos por alghuna razón nos dará un mensaje
  {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  if (!bmp.begin()) 
  {
    Serial.println("Ojo que el sensor no está conectado");
    while (1) {}
  }
  // Comienza la comunicación con el sensor DHT 22 (Temperatura y humedad)
  dht.begin(); //Iniciamos el sensor
  // Colocamos un descanso mientras el sensor se inicia
  delay(10); // 10 ms son más que suficientes en este caso concreto

}


void loop(){
  
  int Nivel_UV = ValorMedio(EntradaUV); //Leemos el nivel del sensor UV
  // Leemos la temperatura y humedad actuales provistas por el sensor
  float temp = dht.readTemperature(); //Almacenamos en una variable tipo float el valos de la temperatura
  float hum = dht.readHumidity(); //En hum, también del tipo anterior, metemos el valor de la humedad
  float pR = puntoRocio(temp, hum); // pR es el punto de rocío que hemos calculado con una función de tipo float que está más abajo
  float presion = bmp.readPressure(); // almacenamos en presion, el valor de esta magnitud
  float altura = bmp.readAltitude(); // lo propio con la altura
  float Vsalida = 3.3 * Nivel_UV/1024; //Comprobamos la fiabilidad del voltaje de salida de la lectura del sensor UV ml8511
  float IntensidadUV = mapfloat(Vsalida, 0.99, 2.9, 0.0, 15.0); // Hacemos un mapeado de la función en coma flotante
  //Visualizamos en local y mandamos datos vía wifi al servisor de Thinkspeak
  Serial.print("T = ");
  Serial.print(temp); //Mostramos temperatura a través de la pantalla del PC con el puerto serie
      ThingSpeak.setField(1,float(temp)); // Enviamos T a Thinkspeak
 // Hacemos lo mismo con todas y cada un ade las variables que hemos dispuesto
  Serial.print(" C - H = ");
  Serial.print(hum); //Humedad
      ThingSpeak.setField(2,float(hum)); //a Thinkspeak
 // Mucho cuidado con la definición de las variables. Si no le ponemos el tipo no 
 // nos saldrá en la web. HAY QUE DEFINIR Y MARCAR EL TIPO DE VARIABLE PARA QUE
 // EL SERVIDOR WEB LO ENTIENDA COMO TAL, de lo contrario no representa nada de nada.
  Serial.print(" % - Pto. Rocio = ");
  Serial.print(pR); // Punto de rocío
      ThingSpeak.setField(3,float(pR)); //Enviamos a Thinkspeak
  Serial.print(" C - Presion = ");
  Serial.print(presion); // Presentamos la presión
      ThingSpeak.setField(4,float(presion)); // La enviamos a ThinkSpeak
  Serial.print(" Pa - Altura = ");
  Serial.print(altura);
      ThingSpeak.setField(5,float(altura));
  Serial.print(" m - Rad. UV = ");
  Serial.print(IntensidadUV);
      ThingSpeak.setField(6,float(IntensidadUV));
      Serial.print(" mW/cm^2");
  Serial.println();
    

  // Enviamos ahora todo el paquete en un único mensaje, dando a esta función como
  // valores de entrada el número de canal y el API que nos dan al registrarnos
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);  

  delay(20000);  // Esperamos 20 segundos para enviar medidas de thingspeak pues
  // dada nuestra modalidad (free registration) esa es la limitación que se nos
  // impone. En todo caso, para una estación meteorológica es más que suficiente.
}

//FUNCIÓN QUE CALCULA EL PUNTO DE ROCÍO. Este parámetro es vital en aeronaútica y, en general,
// en el mundo del transporte porque su proximidad al valor de la temperatura y su tendencia
// nos da una idea de la probabilidad de formación de bancos de niebla.
float puntoRocio(double t, double h)
{
  float x = 1-0.01*h;
  float a = (14.55+0.114*t)*x;
  float b = (2.5+0.007*t)*x;
  float c = pow (b,3);
  float d = 15.9+0.117*t;
  float e = pow(x,14);
  float f = d*e;
  float Td = t-a-c-f;
return Td;
}

//Función que calcula el valor medio de la radiación recibida en el sensor UV ml8511
int ValorMedio(int pin)
{
  byte numeroLlamadas = 8;
  unsigned int Valor = 0; 
 
  for(int x = 0 ; x < numeroLlamadas ; x++)
    Valor += analogRead(pin);
  Valor /= numeroLlamadas;
 
  return(Valor);  
 
}
 
// Mapeo en coma flotante

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}




