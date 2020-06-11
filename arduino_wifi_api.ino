#include "Time/Time.h"
#include <Wire.h>
#include "DS1307RTC/DS1307RTC.h"  // a basic DS1307 library that returns time as a time_t
#include <SoftwareSerial.h>
SoftwareSerial Wifi(3, 2); // RX | TX

String WifiSSID = "YOUR-WIFI-SSID";
String WifiCLAVE = "YOUR-WIFI-KEY";
String direccionIp  = "";
int wifiBaudRate = 19200;
int serialBaudRate = 19200;
long initialWifiBaudRate = 115200;
int ledPin = 13;
int wifiResetPin = 7;
String metodoHttp = "GET";
String urlHttp = "/hora";
#define PrintDebug true

#define contentType "Content-Type: application/json\r\n"
#define corsHeader "Access-Control-Allow-Origin: *\r\n"
#define httpHeader "HTTP/1.1 200 OK\r\n"

void debugLog(String s)
{
  if(PrintDebug) Serial.println(s);
}

class ComandoAt
{
  public:
    ComandoAt(String comando, String respuesta,long wifiBaudRate = 0)
    {
        m_comando = comando;
        m_respuesta = respuesta;
        m_wifiBaudRate = wifiBaudRate;
    }
    
    String getComando()
    {
      return m_comando;
    }
  
    String getEspera()
    {
      return m_respuesta;
    }
  
    long getBaudRate()
    {
      return m_wifiBaudRate;
    }

private:
    String m_comando;
    String m_respuesta;
    long m_wifiBaudRate;
};

ComandoAt inicializacion[] = {
            //ComandoAt("AT", "OK"),
          //  ComandoAt("AT+RST","boot"),
         //   ComandoAt("AT+CIOBAUD=9600", "OK", 9600),
            ComandoAt("AT", "OK"),
            //ComandoAt("AT+CWMODE=3", "OK"),
            ComandoAt("AT+CWJAP=\"" + WifiSSID + "\",\"" + WifiCLAVE + "\"", "WIFI GOT IP"),
            ComandoAt("AT+CIPMUX=1", "OK"),
            ComandoAt("AT+CIPSERVER=1,80", "OK")
};

bool contains(String busqueda, String inString)
{
  return inString.indexOf(busqueda) != -1;
}

void resetWifi()
{
    pinMode(wifiResetPin, INPUT_PULLUP);
    digitalWrite(wifiResetPin, HIGH);
    delay(250);
    pinMode(wifiResetPin, OUTPUT);
    delay(10);
    digitalWrite(wifiResetPin, HIGH);
    delay(5);
    digitalWrite(wifiResetPin, LOW);
    delay(250);
    pinMode(wifiResetPin, INPUT);
    delay(10);
    digitalWrite(wifiResetPin, HIGH);
    delay(250);
  
}

void initializeWifi()
{
  
    resetWifi();

    Wifi.println("AT");
    
    delay(250);
    Wifi.println("AT+CIOBAUD=" + String(wifiBaudRate));
    delay(250);
}



bool EjecutarComandoAt(ComandoAt comando, int  i = 0)
{
        String comandoStr = comando.getComando();
        debugLog("EJECUTANDO: " + comandoStr);
        
        Wifi.println(comandoStr);
        delay(25);
        
        unsigned long T = millis();
        while ( true)
        {   
            String salidaWifi = flushBuffer();
            if ( salidaWifi!= "") debugLog("> " + salidaWifi);
            
            if ( contains(comando.getEspera(), salidaWifi))  
            {
              debugLog("\nOK PASO: ");
              debugLog(String(i+1));
              /*
              if(comando.getBaudRate()!= 0)
              {
                Wifi.begin(comando.getBaudRate());  
              }
              */
              return true;
            }
            if(contains("busy", salidaWifi))
            {
              delay(2000);
              EjecutarComandoAt(comando);
              return false;
              break;
            }
            if ( (millis()-T) >10000 || contains("ERROR", salidaWifi)) 
            {    
              
              debugLog("Timeout PASO: " + String(i+1));
              
              debugLog(" Comando: "+ comandoStr + " \n");
              debugLog(" Datos: " + salidaWifi + "\n");
              return false;
              break;    
            }
        }
        return true;
}

void connectWifi()
{
  int i = 0;
     for(i = 0; i< sizeof(inicializacion)/sizeof(ComandoAt); i++)
     {
        ComandoAt comando = inicializacion[i];
        if( !EjecutarComandoAt(comando,i))
        {
          i--;
        }
     }

}
String GetCurrentIp()
{
  Wifi.println("AT+CIPSTA?");
  delay(25);
  String salida = "";
  char c = Wifi.read();
  while(Wifi.available())
  {

    salida = salida + c;

    delay(10);

    c = Wifi.read();
  }

  
  return getValue(salida, '\"',1) ;//"192.168.1.104";
}

String GetLineWifi()
{
  String S = "" ;
  if (Wifi.available())
  {
    char c = Wifi.read(); ;
    while ( c != '\n' && c != '\r') //Hasta que el caracter sea intro
    {
      S.concat(c) ;
      delay(25) ;
      c = Wifi.read();
     }
     return( S ) ;
   }
}


void setup()
{  
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    Serial.begin(serialBaudRate);
    
    Wifi.begin(initialWifiBaudRate);
    initializeWifi();
    delay(250);
    Wifi.begin(wifiBaudRate);
 
    
    connectWifi();
    direccionIp = GetCurrentIp();
    setSyncProvider(RTC.get);      // Vamos a usar el RTC
      
    String ultimoNumeroIp = getValue(direccionIp, '.',3);
    debugLog("Representare el ultimo de la direccion IP (" + ultimoNumeroIp + ") parpadeando el LED.");
    debugLog("HORA: " + digitalClockDisplay());
   // mostrarNumeroPorLed(ultimoNumeroIp);
}

void mostrarNumeroPorLed(String ultimoNumeroIp)
{
    for(int x = 0; x < ultimoNumeroIp.length() ; x++)  //ultimoNumeroIp.length() - 1 ; x > 0 ; x--)
    {
        int valorDigito = String(ultimoNumeroIp.charAt(x)).toInt() ;
        if(valorDigito == 0) valorDigito = 10;
        delay(1000);
        for(int m = 0 ; m < valorDigito; m++)
        {
            digitalWrite(ledPin, LOW);
            delay(250);
            digitalWrite(ledPin, HIGH);
            delay(250);
            digitalWrite(ledPin, LOW);
        }

        delay(1000);
    }
  
}

String printDigits(int digits)
{     
         String salida = ":";
         if(digits < 10)
            salida = salida + "0";
         
         salida = salida + String(digits);

         return salida;
}

String digitalClockDisplay()
{   
         String salida = String(hour()) +
         printDigits(minute()) +
         printDigits(second()) +
         " " +
         String(day())+
         "/" +
         String(month())+
         "/"+
         String(year());

         return salida;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String flushBuffer()
{
  String outBuff = "";
  char c = Wifi.read();
  while(Wifi.available())
  {
    outBuff = outBuff + String(c);
    delay(25);
    c = Wifi.read();
  }  
  return outBuff;
}

void sendHttpData(String datos, int channel)
{
      unsigned long sendingBytes = datos.length();
      
      debugLog("BYTES A ENVIAR: " + String(sendingBytes));

      
      EjecutarComandoAt(ComandoAt("AT+CIPSEND="+String(channel)+"," + String(sendingBytes), "OK"));
      Wifi.print(datos);  
      
      delay(50);
      while(true)
      {
        String resultado = flushBuffer();
        if(contains("SEND OK" , resultado))
        {
          break;
        }
      }
}

void CloseIPConnection(int channel)
{
  EjecutarComandoAt(ComandoAt("AT+CIPCLOSE=" + String(channel), String(channel)+",CLOSED")); 
}

void loop()
{  

  
  debugLog("Listo para recibir comandos por HTTP: " + direccionIp);
  
  while(true)
  {
    String salidaWifi = GetLineWifi();

    if(contains(":" + metodoHttp + " " + urlHttp + " HTTP/1.1", salidaWifi))
    {
      

      int channel = getValue(salidaWifi,',',1).toInt();
      int recvBytes = getValue(salidaWifi,',',2).toInt();
      



       Wifi.flush();

       

      debugLog("Canal: " + String(channel) + " N de bytes rcv: "+ String(recvBytes) );
      
      
      sendHttpData(String(httpHeader), channel);
      sendHttpData(String(contentType), channel);
      sendHttpData(String(corsHeader), channel);
      sendHttpData("\r\n", channel);
      
      String datos = String("{ \"msg\": \"desde el Arduino?: \", \"datos\": \"" +  digitalClockDisplay() + "\" }");
      sendHttpData(datos, channel);
      CloseIPConnection(channel);

      break;
    }
  } 
/*
       if (Wifi.available())
         { char c = Wifi.read() ;
           Serial.print(c);
         }

         
     if (Serial.available())
         {  char c = Serial.read();
            Wifi.print(c);
         }
*/
}
