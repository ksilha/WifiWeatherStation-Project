//
// FILE: WifiWeatherStation.ino
// AUTHOR: Kevin Silha
// VERSION: 0.1.00
// PURPOSE: DHT22 Weather Station communicating over ESP8266 webserver
// URL:
// HISTORY:
// 0.1.00 initial version
//
// Released to the public domain
//

#include <dht.h>
#include <AltSoftSerial.h>

dht DHT;
AltSoftSerial espSerial;

#define DHT22_PIN 5

const bool printReply = true;
const char line[] = "-----\n\r";
char html[50];
char command[20];
char TempString[5];
char reply[500];
char ipAddress [20];
int led = 13;

void setup() {
      Serial.begin(9600);
      Serial.println("Start\r\n\r\n");
 
      espSerial.begin(9600); // your ESP8266 module's baud rate might be different
 
      // reset the ESP8266
      Serial.println("reset the module"); 
      espSerial.print("AT+RST\r\n");
      getReply( 2000 );
 
      // configure as a station
      Serial.println("Change to station mode"); 
      espSerial.print("AT+CWMODE=1\r\n");
      getReply( 1500 );
 
      // connect to the network. Uses DHCP. ip will be assigned by the router.
      Serial.println("Connect to a network ");
 
     // Enter the SSID and password for your own network
      espSerial.print("AT+CWJAP=\"networkname\",\"password\"\r\n");
      getReply( 6000 );
 
      // get ip address
      Serial.println("Get the ip address assigned ny the router"); 
      espSerial.print("AT+CIFSR\r\n");
      getReply( 1000 );
 
      // parse ip address.
      int len = strlen( reply ); 
      bool done=false;
      bool error = false;
      int pos = 0;
      
      while (!done)
      {
           if ( reply[pos] == 10) { done = true;} 
           pos++;
           if (pos > len) { done = true;  error = true;}
      }
 
      if (!error)
      {
            int buffpos = 0;
            done = false;
            while (!done)
            {
               if ( reply[pos] == 13 ) { done = true; }
               else { ipAddress[buffpos] = reply[pos];    buffpos++; pos++;   }
            }
            ipAddress[buffpos] = 0;
      }
      else { strcpy(ipAddress,"ERROR"); }
 
      // configure for multiple connections
      Serial.println("Set for multiple connections"); 
      espSerial.print("AT+CIPMUX=1\r\n");
      getReply( 1500 );
 
      // start server on port 80
      Serial.println("Start the server"); 
      espSerial.print("AT+CIPSERVER=1,80\r\n");
      getReply( 1500 );
 
      Serial.println("");
 
      Serial.println("Waiting for page request");
      Serial.print("Connect to "); Serial.println(ipAddress);
      Serial.println("");

      pinMode(led, OUTPUT);  
      digitalWrite(led, HIGH);
}

void loop() {
    if(espSerial.available()) // check if the ESP8266 is sending data
      {
          // this is the +IPD reply - it is quite long. 
          // normally you would not need to copy the whole message in to a variable you can copy up to "HOST" only
          // or you can just search the data character by character as you read the serial port.
          getReply( 2000 );      
 
          bool foundIPD = false;
          
          for (int i=0; i<strlen(reply); i++)
          {
               if (  (reply[i]=='I') && (reply[i+1]=='P') && (reply[i+2]=='D')   ) { foundIPD = true;    }
          }
 
          if ( foundIPD  )  
          {
              double temp = getReading('T') * 9/5 + 32;
              double humidity = getReading('H');
              
              // start sending the HTML
              strcpy(html,"<xml><data><temp>");
              strcpy(command,"AT+CIPSEND=0,17\r\n");
              espSerial.print(command);
              getReply( 2000 );          
              espSerial.print(html);
              getReply( 2000 ); 

              strcpy(html,dtostrf(temp,5,1,TempString));
              strcpy(command,"AT+CIPSEND=0,5\r\n");
              espSerial.print(command);
              getReply( 2000 );         
              espSerial.print(html);
              getReply( 2000 );

              strcpy(html,"</temp><humidity>");
              strcpy(command,"AT+CIPSEND=0,17\r\n");
              espSerial.print(command);
              getReply( 2000 );         
              espSerial.print(html);
              getReply( 2000 );

              strcpy(html,dtostrf(humidity,5,1,TempString));
              strcpy(command,"AT+CIPSEND=0,5\r\n");
              espSerial.print(command);
              getReply( 2000 );         
              espSerial.print(html);
              getReply( 2000 );
 
              strcpy(html,"</humidity></data>");
              strcpy(command,"AT+CIPSEND=0,18\r\n");
              espSerial.print(command);
              getReply( 2000 ); 
              espSerial.print(html);
              getReply( 2000 ); 
 
              // close the connection
              espSerial.print( "AT+CIPCLOSE=0\r\n" );
              getReply( 1500 );            
          }
      }
 
      delay (100);
 
      // drop to here and wait for next request.
}

double getReading(char readingType)
{
    uint32_t start = micros();
    int chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();

    if (readingType == 'H')
      return DHT.humidity;
    else if (readingType == 'T')
      return DHT.temperature;
}

void getReply(int wait)
{
    int tempPos = 0;
    long int time = millis();
    while( (time + wait) > millis())
    {
        while(espSerial.available())
        {
            char c = espSerial.read(); 
            if (tempPos < 500) { reply[tempPos] = c; tempPos++;   }
        }
        reply[tempPos] = 0;
    } 
 
    if (printReply) { Serial.println( reply );  Serial.println(line);     }
}
