// Library needed in Arduino 0019 or later
#include <SPI.h> 

// Include need libraries
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Twitter.h>
#include <dht11.h>
#include <Time.h>

// Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
	return 1.8 * celsius + 32;
}

// Fast integer version with rounding
// int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}


// Ethernet shield settings, devices MAC address
byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

// Local port to listen for UDP packets
unsigned int localPort = 8888;

// NTP server: time-a.timefreq.bldrdoc.gov
IPAddress timeServer(132, 163, 4, 101);

// NTP local offset (hours in seconds) set to EDT by default
const long timeZoneOffset = -14400L; 

// NYTP Sync Interval
unsigned int ntpSyncTime = 15;  

// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE= 48;

// Buffer to hold incoming and outgoing packets 
byte packetBuffer[ NTP_PACKET_SIZE];

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Keep track of how long ago we updated the NTP time
unsigned long ntpLastUpdate = 0;

// Your token to tweet (get it from http://arduino-tweet.appspot.com/)
Twitter twitter("");

// Set the sensor up
dht11 DHT11;
#define DHT11PIN 2

void setup()
{
  int i = 0;
  // Open a serial port and wait for it to be ready  
  Serial.begin(9600);
  delay(5000);
  // Setup ethernet access  
  Ethernet.begin(mac);
   // Try to get the date and time
   int trys=0;
   while(!getTimeAndDate() && trys<10) {
     trys++;
   }
}

// Do not alter this function, it is used by the system
int getTimeAndDate() {
   int flag=0;
   Udp.begin(localPort);
   sendNTPpacket(timeServer);
   delay(1000);
   if (Udp.parsePacket()){
     // Read the packet into the buffer
     Udp.read(packetBuffer,NTP_PACKET_SIZE);
     unsigned long highWord, lowWord, epoch;
     highWord = word(packetBuffer[40], packetBuffer[41]);
     lowWord = word(packetBuffer[42], packetBuffer[43]);  
     epoch = highWord << 16 | lowWord;
     epoch = epoch - 2208988800 + timeZoneOffset;
     flag=1;
     setTime(epoch);
     ntpLastUpdate = now();
   }
   return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;                  
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

// Clock display of the time and date (Basic). Format: MM/DD/YY - H:MM:SS
void clockDisplay(){
  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print("/");
  Serial.print(year());
  Serial.print(" - ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.println();
}

void loop()
{
  Serial.println("\n");

  // Update the time via NTP server as often as the time you set at the top
  if(now()-ntpLastUpdate > ntpSyncTime) {
    int trys=0;
    while(!getTimeAndDate() && trys<10){
      trys++;
    }
    if(trys<10){
      Serial.println("NTP server update: Success.");
    }
    else{
      Serial.println("NTP server update: Failed.");
    }
  }
   
  // Display the date & time
  clockDisplay();  

  // Verify the sensor is reading correctly
  int chk = DHT11.read(DHT11PIN);
  Serial.print("Reading sensor: ");
  switch (chk)
  {
    case DHTLIB_OK: 
		Serial.println("Success."); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Checksum error."); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Time out error."); 
		break;
    default: 
		Serial.println("Unknown error."); 
		break;
  }
  
  Serial.print("Temperature (oF): ");
  Serial.println(Fahrenheit(DHT11.temperature), 2);
  
  Serial.print("Humidity (%): ");
  Serial.println((float)DHT11.humidity, 2);
  
  String message;
  int temp;
  int months;
  int dates;
  int years;
  int hours;
  int minutes;
  int seconds;
  String ampm;
  
  temp = Fahrenheit(DHT11.temperature);
  months = month();
  dates = day();
  years = year();
  minutes = minute();
  seconds = second();
  
  // Adjust hours to 12h format
  if(hour() > 12){
    hours = hour()-12;
  }
  else{
    hours = hour();
  }
 
  // Set AM or PM
  if (hour() - 12 >= 0){
    ampm = "pm";
  }
  else{
    ampm = "am";
  }
  
  // Compose the message
  message = "The Gelman Library sixth floor is currently: " + String(temp); 
  message += " degrees Fahrenheit with " + String(DHT11.humidity);
  message += " percent humidity. -- ";
  
  // Add leading "0" to all hours between midnight and noon
  if (hour() - 12 < 12){
    message += String("0") + String(hours) + String(":");
  }
  else{
    message += String(hours) + String(":");
  }
  // Add leading "0" to all minutes between 0-10
  if (minute() < 10){
    message += String("0") + String(minutes);
  }
  else{
    message += String(minutes);
  }
  
  message += String(ampm) + " " + String(months) + "/" + String(dates) + "/" + String(years);
    
  Serial.println(message);
 
tweet(message);
  //Specify time between tweets (Default 1 hour)
  delay(3600000);
}

// Send a tweet
void tweet(String message)
{
  char msg[140] = "";
  message.toCharArray(msg, 140);
  Serial.println("Connecting to Twitter ...");
  if (twitter.post(msg)) {
    int status = twitter.wait();
    if (status == 200) {
      Serial.println("Post Tweet: Success.");
    } else {
      Serial.print("Post Tweet: Failed - code ");
      Serial.println(status);
    }
  } else {
    Serial.println("Connect to Twitter: Failed.");
    // Added a retry if connection fails (Default 3 mintues)
    Serial.println("Connect to Twitter: Retrying in 3 minutes.");
    delay(180000);
    if (twitter.post(msg)) {
      int status = twitter.wait();
      if (status == 200) {
        Serial.println("Post Tweet: Success.");
      } else {
        Serial.print("Post Tweet: Failed - code ");
        Serial.println(status);
      }
    }  
  }
}
