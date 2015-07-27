
/*
 *  This sketch based on the example simple HTTP-like server.
 *  The server will perform 3 functions depending on the request
 *  1.  http://ESP8266-IP:SVRPORT/gpio/0 will set the GPIO16 low,
 *  2.  http://ESP8266-IP:SVRPORT/gpio/1 will set the GPIO16 high
 *  3.  http://ESP8266-IP:SVRPORT/?request=GetSensors will return a json string with sensor values
 *
 *  ESP8266-IP is the IP address of the ESP8266 module
 *  SVRPORT is the TCP port number the server listens for connections on
 */
#include <OneWire.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <DHT.h>

#include <Adafruit_BMP085.h>
#include <UtilityFunctions.h>

extern "C" {
#include "user_interface.h"
}

#include <sha256.h>
#include <Utils.h>
#include "keys.h" 


//Server actions
#define SET_LED_OFF 0
#define SET_LED_ON  1
#define Get_SENSORS 2

#define SERBAUD 74880
#define SVRPORT 9703
#define ONEJSON 0
#define FIRSTJSON 1
#define NEXTJSON 2
#define LASTJSON 3

//GPIO pin assignments
#define DEBUGEN 4       // Debug enable - pin 5 on ESP8266
#define DS18B20 5       // Temperature Sensor pin (DS18B20) - pin 4 on ESP8266
#define AMUXSEL0 14     // AMUX Selector 0
#define AMUXSEL1 12     // AMUX Selector 1
#define AMUXSEL2 13     // AMUX Selector 2
#define LED_IND 16      // LED used for initial code testing (not included in final hardware design)

#define DHTTYPE DHT11   // DHT 11 

const char* ssid = "YourWifiSSIDhere";
const char* password = "YourWifiPasswordHere";
const IPAddress ipadd(192,168,0,174);           //Static IP for your LAN-------------- 
const IPAddress ipgat(192,168,0,1);             //LAN router IP-----------------------
const IPAddress ipsub(255,255,255,0);           //SubnetMask--------------------------

//For the signature example for AWS
/*
const char* key = "AWS4wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
const char* dateStamp = "20120215";
const char* regionName = "us-east-1";
const char* serviceName = "iam";
*/
//For the signature for AWS
const char* regionName = "us-west-2";
const char* serviceName = "dynamodb/aws4_request";
const char* domainName = "dynamodb.us-west-2.amazonaws.com";
const char* AccessKeyId= "YourAWS_AccessKeyId";
const char* SecretAccessKey= "YourAWS_SecretAccessKey";

//For Timeserver America/Los_Angeles
//const char* timeServer = "https://script.googleusercontent.com/macros/echo?user_content_key=9an5S1jCH58z_rusYYOgfR-QW5o4fH9b9PVPpQxVx-NgAGG5atTsYuUcAVRHD_m1MANu37-Y-d21dcSYF9RlxmCeb1RzJs3am5_BxDlH2jW0nuo2oDemN9CCS2h10ox_1xSncGQajx_ryfhECjZEnJ9GRkcRevgjTvo8Dc32iw_BLJPcPfRdVKhJT5HNzQuXEeN3QFwl2n0M6ZmO-h7C6bwVq0tbM60-_IQDS8gp7-x7tfawlsXCfm-Jc7VfHX0G&lib=MwxUjRcLr2qLlnVOLh12wSNkqcO1Ikdrk";
const char* timeServer = "developer.yahooapis.com";
const char* timeServerGet = "GET /TimeService/V1/getTime?appid=YahooDemo&output=json&format=unix  HTTP/1.1";
String utctime;
String GmtDate;
char dateStamp[10];


//globals
int lc=0;
bool complete=false;
char Ain0[20],Ain1[20],Ain2[20],Ain3[20],Ain4[20],Ain5[20],Ain6[20],Ain7[20];
char key[80]; 
char kSecret[80];
char kDate[80];
char kRegion[80];
char kService[80];
char kSigning[80];
char kSignature[80];
char szT2[80];
char payloadHash[80];
char payloadStr[1000];
char canonicalRequestHash[80];
char canonicalRequestStr[500];
char toSignStr[500];
    
char* szz;
uint32_t state=0;
char szT[30];
float Ain;

SHA256 sha256;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(SVRPORT);
WiFiClient client;
WiFiClient client2;

int find_text(String needle, String haystack) {
  int foundpos = -1;
  for (int i = 0; (i < haystack.length() - needle.length()); i++) {
    if (haystack.substring(i,needle.length()+i) == needle) {
      foundpos = i;
    }
  }
  return foundpos;
}

String getMonth(String sM) {
  if(sM=="Jan") return "01";
  if(sM=="Feb") return "02";
  if(sM=="Mar") return "03";
  if(sM=="Apr") return "04";
  if(sM=="May") return "05";
  if(sM=="Jun") return "06";
  if(sM=="Jul") return "07";
  if(sM=="Aug") return "08";
  if(sM=="Sep") return "09";
  if(sM=="Oct") return "10";
  if(sM=="Nov") return "11";
  if(sM=="Dec") return "12";
  return "01";
}
void printStatus(char * status, int s) {
//    if(digitalRead(DEBUGEN)==1) {
      Serial.print(system_get_free_heap_size());
      delay(100);
      Serial.print(" ");
      delay(100);
      Serial.print(millis()/1000);
      delay(100);
      Serial.print(" ");
      delay(100);
      if(s>=0) Serial.print(s);
      else Serial.print("");
      delay(100);
      Serial.print(" ");
      delay(100);
      Serial.println(status);
//    }
    delay(100);
}
void startWIFI(void) {
  //set IP if not correct
  IPAddress ip = WiFi.localIP();
  //if( (ip[0]!=ipadd[0]) || (ip[1]!=ipadd[1]) || (ip[2]!=ipadd[2]) || (ip[3]!=ipadd[3]) ) { 
  if( ip!= ipadd) { 
      WiFi.config(ipadd, ipgat, ipsub);  //dsa added 12.04.2015
      Serial.println();
      delay(10);
      Serial.print("ESP8266 IP:");
      delay(10);
      Serial.println(ip);
      delay(10);
      Serial.print("Fixed   IP:");
      delay(10);
      Serial.println(ipadd);
      delay(10);
      Serial.print("IP now set to: ");
      delay(10);
      Serial.println(WiFi.localIP());
      delay(10);
  }
  // Connect to WiFi network
  Serial.println();
  delay(10);
  Serial.println();
  delay(10);
  Serial.print("Connecting to ");
  delay(10);
  Serial.println(ssid);
  delay(10); 
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("ESP8266 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("ESP8266 WebServer Port: ");
  Serial.println(SVRPORT);
  delay(300);

}


void readSensorIsr() {
  yield();
  switch(state++) {
    case 0:
      //Set 8-1 amux to position 0
      digitalWrite(AMUXSEL0, 0);
      digitalWrite(AMUXSEL1, 0);
      digitalWrite(AMUXSEL2, 0);
      delay(100);
      //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain0, 2);
     break;
    case 1:
      //Set 8-1 amux to position 1
      digitalWrite(AMUXSEL0, 1);
      digitalWrite(AMUXSEL1, 0);
      digitalWrite(AMUXSEL2, 0);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain1, 2);
      break;
    case 2:
      //Set 8-1 amux to position 2
      digitalWrite(AMUXSEL0, 0);
      digitalWrite(AMUXSEL1, 1);
      digitalWrite(AMUXSEL2, 0);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain2, 2);
      break;
    case 3:
      //Set 8-1 amux to position 3
      digitalWrite(AMUXSEL0, 1);
      digitalWrite(AMUXSEL1, 1);
      digitalWrite(AMUXSEL2, 0);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain3, 2);
      break;
    case 4:
      //Set 8-1 amux to position 4
      digitalWrite(AMUXSEL0, 0);
      digitalWrite(AMUXSEL1, 0);
      digitalWrite(AMUXSEL2, 1);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain4, 2);
      break;
    case 5:
      //Set 8-1 amux to position 5
      digitalWrite(AMUXSEL0, 1);
      digitalWrite(AMUXSEL1, 0);
      digitalWrite(AMUXSEL2, 1);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain5, 2);
      break;
    case 6:
      //Set 8-1 amux to position 6
      digitalWrite(AMUXSEL0, 0);
      digitalWrite(AMUXSEL1, 1);
      digitalWrite(AMUXSEL2, 1);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain6, 2);
      break;
    case 7:
      //Set 8-1 amux to position 7
      digitalWrite(AMUXSEL0, 1);
      digitalWrite(AMUXSEL1, 1);
      digitalWrite(AMUXSEL2, 1);
      delay(100);
       //Read analog input
      Ain = (float) analogRead(A0);
      ftoa(Ain,Ain7, 2);
      state = 0;
      break;
    default:
      break;
  }
  ESP.wdtFeed(); 
  yield();
}

void jsonAdd(String *s, String key,String val) {
    *s += '"' + key + '"' + ":" + '"' + val + '"';
}
void jsonEncode(int pos, String * s, String key, String val) {
    switch (pos) {
      case ONEJSON:      
      case FIRSTJSON:
        *s += "{\r\n";
        jsonAdd(s,key,val);
        *s+= (pos==ONEJSON) ? "\r\n}" : ",\r\n";
        break;
      case NEXTJSON:    
        jsonAdd(s,key,val);
        *s+= ",\r\n";
        break;
      case LASTJSON:    
        jsonAdd(s,key,val);
        *s+= "\r\n}";
        break;
    }
}
void awsRequest(String rqst, String * reply) {
    static int timeout_busy=0;    
    int ipos;
    if (client2.connect("dynamodb.us-west-2.amazonaws.com", 80)) {
        //Send Request
        client2.println(rqst);
        client2.println();
        while((!client2.available())&&(timeout_busy++<15000)){ 
          // Wait until the client sends some data
          delay(1);         
        }
        //kill client if timeout
        if(timeout_busy>=15000) {
            client2.flush();
            client2.stop();
            Serial.println("timeout receiving aws data\n");
            return;
        } 
        //Read the http GET Response
        *reply = client2.readString(); 
    }
    else {
        *reply = "Failed to connect to AWS"; 
    }
}

void updateCurTime(void) {
    static int timeout_busy=0;    
    state = 1;
    int ipos;
    timeout_busy=0; //reset
    if (client2.connect(timeServer, 80)) {
        //Send Request
        client2.println(timeServerGet);
        //client.println("GET /search?q=arduino HTTP/1.0");
        client2.println();
        while((!client2.available())&&(timeout_busy++<5000)){ 
          // Wait until the client sends some data
          delay(1);         
        }
        //kill client if timeout
        if(timeout_busy>=5000) {
            client2.flush();
            client2.stop();
            Serial.println("timeout receiving timeserver data\n");
            return;
        } 
        //Read the http GET Response
        String req2 = client2.readString();
        
        //Get the index of the Date String
        ipos = find_text((String) "Date:",req2);
        //If found, create UTC X-Amz-Date:<DATE> String 
        /*
        if(ipos>0) {
           String req3 = req2.substring(ipos+6,ipos+36);
           utctime = req3.substring(12,16) + getMonth(req3.substring(8,11)) + req3.substring(5,7) + 'T' + req3.substring(17,19) + req3.substring(20,22) + req3.substring(23,25) + 'Z';
           utctime.substring(0,8).toCharArray(dateStamp, 10);
        }
        */
        if(ipos>0) {
           GmtDate = req2.substring(ipos,ipos+35);
           utctime = GmtDate.substring(18,22) + getMonth(GmtDate.substring(14,17)) + GmtDate.substring(11,13) + 'T' + GmtDate.substring(23,25) + GmtDate.substring(26,28) + GmtDate.substring(29,31) + 'Z';
           utctime.substring(0,8).toCharArray(dateStamp, 10);
        }
        
        client2.flush();
        //Serial.print("Time String: ");
        //Serial.println(utctime);
        //delay(10);
                
        //Close connection
        delay(1);
        client2.flush();
        client2.stop();
    }
    else {
      //Serial.println("did not connect to timeserver\n");
    }
    timeout_busy=0; //reset
}

void killclient(WiFiClient client, bool *busy) {
  lc=0;
  delay(1);
  client.flush();
  client.stop();
  complete=false;
  *busy = false;  
}
void sysloop() {
  static bool busy=false;
  static int timeout_busy=0;
  int msglen;
  int amux;
  //connect wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    startWIFI();
    return;
  }
  //return if busy
  if(busy) {
    delay(1);
    if(timeout_busy++ >10000) {
      printStatus((char *)" Status: Busy timeout-resetting..",-1);
      ESP.reset(); 
      busy = false;
    }
    return;
  }
  else {
    busy = true;
    timeout_busy=0;
  }
  delay(1);
  //Read 1 sensor every 2.5 seconds
  if(lc++>500) {
    lc=0;
    ftoa(Ain,szT, 2);
    if(state==0) amux=7;
    else amux = state-1;
    printStatus((char *)szT,amux);
    readSensorIsr(); 
    busy = false;
    return;   
  }
  // Check if a client has connected
  client = server.available();
  //WiFiClient client = server.available();
  if (!client) {
      busy = false;
      return;
  } 
  // Wait until the client sends some data
  while((!client.available())&&(timeout_busy++<5000)){
    delay(1);
    if(complete==true) {
      killclient(client, &busy);
      return;
    }
  }
  //kill client if timeout
  if(timeout_busy>=5000) {
    killclient(client, &busy);
    return;
  }
  
  complete=false; 
  ESP.wdtFeed(); 
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  client.flush();
  if (req.indexOf("/favicon.ico") != -1) {
    client.stop();
    complete=true;
    busy = false;
    return;
  }
  Serial.print("Recv http: ");  
  Serial.println(req);
  delay(100);
  
    // Match the request
  int val;
  if (req.indexOf("/gpio/0") != -1)
    val = SET_LED_OFF;
  else if (req.indexOf("/gpio/1") != -1)
    val = SET_LED_ON;
  else if (req.indexOf("/?request=GetSensors") != -1) {
    val = Get_SENSORS;
    //Serial.println("Get Sensor Values Requested");
  }  
  else {
    Serial.println("invalid request");
    client.stop();
    complete=true;
    busy = false;
    return;
  }
  client.flush();
  ///////////////////////////////////////////
  // Create Request Content
  ///////////////////////////////////////////
  String msgC = "{\r\n";
  msgC += "    "; 
  msgC += '"';
  msgC += "TableName";
  msgC += '"';
  msgC += ':';
  msgC += ' ';
  msgC += '"';
  msgC += "AWSArduinoSDKDemo";
  msgC += '"';
  msgC += ',';
  msgC += "\r\n";
  msgC += "    ";   
  msgC += '"';
  msgC += "Key";
  msgC += '"';
  msgC += ':';
  msgC += ' ';
  msgC += '{';
  msgC += "\r\n";
  msgC += "        ";   
  msgC += '"';
  msgC += "DemoName";
  msgC += '"';
  msgC += ':';
  msgC += ' ';
  msgC += '{';
  msgC += '"';
  msgC += 'S';
  msgC += '"';
  msgC += ':';
  msgC += '"';
  msgC += "Colors";
  msgC += '"';
  msgC += "},";
  msgC += "\r\n";    msgC += "        ";   
  msgC += '"';
  msgC += "id";
  msgC += '"';
  msgC += ':';
  msgC += ' ';
  msgC += '{';
  msgC += '"';
  msgC += 'S';
  msgC += '"';
  msgC += ':';
  msgC += '"';
  msgC += "1";
  msgC += '"';
  msgC += '}';
  msgC += "\r\n";   
  msgC += "    }\r\n";   
  msgC += "}"; 
  msglen = msgC.length();  

  ///////////////////////////////////////////
  // Create Canonical Request String
  ///////////////////////////////////////////
  int hAry[32];
  msgC.toCharArray(payloadStr,1000);
  sha256.getHashVal((char*)&payloadHash[0], payloadStr);
  updateCurTime();
  
  String requestC = "POST\n/\n\n";
  requestC += "host:dynamodb.us-west-2.amazonaws.com\n";
  requestC += "x-amz-date:";
  requestC += utctime;
  requestC += "\n";
  requestC += "x-amz-target:DynamoDB_20120810.GetItem\n\n";
  requestC += "host;x-amz-date;x-amz-target\n";
  requestC += payloadHash;

  
  Serial.println("\r\n\r\nCanonical Request String:\r\n");
  Serial.println(requestC);

  ///////////////////////////////////////////
  // Get canonicalRequestHash
  ///////////////////////////////////////////
  requestC.toCharArray(canonicalRequestStr,500);
  sha256.getHashVal((char*)&canonicalRequestHash[0], canonicalRequestStr);
   
  ///////////////////////////////////////////
  // Create StringToSign
  ///////////////////////////////////////////
  String StringToSign = "AWS4-HMAC-SHA256\n";
  StringToSign += utctime;
  StringToSign += "\n";
  StringToSign += dateStamp;
  StringToSign += "/us-west-2/dynamodb/aws4_request\n";
  StringToSign += canonicalRequestHash;
  StringToSign.toCharArray(toSignStr,500);
  
  ///////////////////////////////////////////
  // Create Request Signature
  ///////////////////////////////////////////
  strcpy(kSecret,"AWS4");
  strcat(kSecret,SecretAccessKey);
 // hexEncode(kSecret, key, strlen(key));

  hmacSha256((char*)&kDate[0], kSecret, strlen(kSecret), dateStamp, strlen(dateStamp));  //kDate
  hmacSha256((char*)&kRegion[0], kDate, 32, regionName, strlen(regionName));  //kRegion
  hmacSha256((char*)&kService[0], kRegion, 32, serviceName, strlen(serviceName));  //kService
  hmacSha256((char*)&kSigning[0], kService, 32, "aws4_request", strlen("aws4_request"));  //kSigning
  hmacSha256((char*)&kSignature[0], kSigning, 32, toSignStr, strlen(toSignStr));  //kSignature
  hexEncode(szT2,kSignature,32);

  ///////////////////////////////////////////
  // Prepare AWS Request header
  ///////////////////////////////////////////
  String reply;
  String rqst = "POST / HTTP/1.1\r\n";
  rqst += "Host: ";
  rqst += domainName;
  rqst += "\r\nx-amz-date: ";
  rqst += utctime;
  rqst += "\r\n";
  rqst += "x-amz-target: DynamoDB_20120810.GetItem\r\n";
  rqst += "Authorization: AWS4-HMAC-SHA256 Credential=";
  rqst += AccessKeyId;
  rqst += "/";
  rqst += dateStamp;
  rqst += "/";
  rqst += regionName;
  rqst += "/";
  rqst += serviceName;
  rqst += ",SignedHeaders=host;x-amz-date;x-amz-target,Signature=";  
  rqst += szT2;
  rqst += "\r\n";
  rqst += GmtDate;
  rqst += "\r\n";
  rqst += "content-type: application/x-amz-json-1.0\r\n";
  rqst += "content-length: ";
  rqst += msglen;
  rqst += "\r\n";
  rqst += "connection: Keep-Alive\r\n";
  rqst += "\r\n";
  rqst += payloadStr;
  //rqst += msgC;

  awsRequest(rqst,&reply);
  
  Serial.println("\r\n\r\nRequest to AWS:\r\n\r\n");
  Serial.println(rqst);
  Serial.println("\r\n\r\nReply from AWS:\r\n\r\n");
  Serial.println(reply);

  ///////////////////////////////////////////
  // Prepare Response header
  ///////////////////////////////////////////
  String result = "";
  String s = "HTTP/1.1 200 OK\r\n";
         s += "Access-Control-Allow-Origin: *\r\n";
  String v ="";
  ESP.wdtFeed(); 
      
  switch (val) {
    case SET_LED_OFF:
    case SET_LED_ON:
      // Set GPIO4 according to the request
      digitalWrite(LED_IND , val);
  
      // Prepare the response for GPIO state
      s += "Content-Type: text/html\r\n\r\n";
      s += "<!DOCTYPE HTML>\r\nGPIO is now ";
      s += (val)?"high":"low";
      s += "</html>\n";
      // Send the response to the client
      client.print(s);
      break;
    case Get_SENSORS:
      //Create JSON return string
      s += "Content-Type: application/json\r\n\r\n";
      jsonEncode(FIRSTJSON,&s,"awsKeyID", awsKeyID);
      jsonEncode(NEXTJSON,&s,"awsSecKey", awsSecKey);

      //hexEncode(szT2, payloadHash, strlen(payloadHash));
      jsonEncode(NEXTJSON,&s,"payloadHash", payloadHash);
      //hexEncode(szT2, canonicalRequestHash,strlen(canonicalRequestHash));
      jsonEncode(NEXTJSON,&s,"canonicalRequestHash", canonicalRequestHash);
      jsonEncode(NEXTJSON,&s,"kSecret", kSecret);

      //hmacSha256((char*)&kDate[0], key, strlen(key), dateStamp, strlen(dateStamp));  //kDate
      hexEncode(szT2,kDate,32);
      jsonEncode(NEXTJSON,&s,"kDate", szT2);
      
      //hmacSha256((char*)&kRegion[0], kDate, 32, regionName, strlen(regionName));  //kRegion
      hexEncode(szT2,kRegion,32);
      jsonEncode(NEXTJSON,&s,"kRegion", szT2);
      
      //hmacSha256((char*)&kService[0], kRegion, 32, serviceName, strlen(serviceName));  //kService
      hexEncode(szT2,kService,32);
      jsonEncode(NEXTJSON,&s,"kService", szT2);
      
      //hmacSha256((char*)&kSigning[0], kService, 32, "aws4_request", strlen("aws4_request"));  //kSigning
      hexEncode(szT2,kSigning,32);
      jsonEncode(NEXTJSON,&s,"kSigning", szT2);

      hexEncode(szT2,kSignature,32);
      jsonEncode(NEXTJSON,&s,"kSignature", szT2);

      //Connect to time Server

      v = system_get_free_heap_size();
      jsonEncode(NEXTJSON,&s,"SYS_Heap", v);
      v = millis()/1000;
      jsonEncode(LASTJSON,&s,"SYS_Time", v);
      // Send the response to the client
      client.print(s);
      yield();
      //ESP.wdtFeed(); 
      break;
    default:
      break;
   }

    delay(1);
    v ="";
    s="";
    val=0;
    Serial.println("Ending it: Client disconnected");
    delay(150);
    complete=true;
    busy = false;
    ESP.wdtFeed(); 
  

  // The client will actually be disconnected 
  // when the function returns and 'client' object is destroyed
}

void setup() {
  //ESP.wdtEnable();
  delay(2000);
  Serial.begin(SERBAUD);
  delay(10);
  //printStatus((char *)" Greetings from this ESP8266...",-1);
  //delay(500);
  //complete=false;
  startWIFI();
  
  // Set Indicator LED as output
  pinMode(LED_IND , OUTPUT);
  digitalWrite(LED_IND, 0);
  // Set AMUX Selector pins as outputs
  pinMode(AMUXSEL0 , OUTPUT);
  pinMode(AMUXSEL1 , OUTPUT);
  pinMode(AMUXSEL2 , OUTPUT);
  // Set DEBUGEN as INPUT
  pinMode(DEBUGEN , INPUT);

  // Print Free Heap
  printStatus((char *)" Status: End of Setup",-1);
  delay(500);
  
}

void loop() {
    sysloop();
}
