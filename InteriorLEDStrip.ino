#include <Time.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h> 
#include <Adafruit_NeoPixel.h>
#include <Math.h >



 #define PIN D8

// How many NeoPixels are attached to the Arduino?
# define NUMPIXELS 18

int progress = 0;
int lastProgress = 0;
int prevLength = 0;
int meetingLength = 0;
int timeLeft = 0;

unsigned long referenceTime;
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGBW + NEO_KHZ800);

int delayval = 100; // delay for half a second

const char* ssid = "121King5_GOOD";
const char* password = "BronBronIn7";
const char* mqttServer = "192.168.0.22";
const int mqttPort = 1883;
const char * clientName = "RoomNode1Interior";
const char * topic = "11 SOUTH 14";

WiFiClient espClient;
PubSubClient client(espClient);

void ConnectWifi(const char * ssid,
  const char * password)
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to Wifi..");
  }
  Serial.println("Connected to network");
}

void ConnectBroker(PubSubClient client,
  const char * clientName)
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT: ");
    Serial.println(clientName);
    if (client.connect(clientName))
    {
      Serial.println("Connected");
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(200);
    }
  }
}

void updateStrip(int givenLength, int givenTimeLeft)
{

  //Serial.println(unit);
  progress = givenLength - givenTimeLeft;
 
  int timeDiff = 0;

  if (progress < lastProgress || prevLength < givenLength)
  {
    clearColour();
  }


  unsigned long newTime = time(nullptr);


  if (givenLength != meetingLength)
  {
    referenceTime = time(NULL);
    Serial.println("Reference: ");
    Serial.println(referenceTime);
  }
  else if (meetingLength > 0 && timeLeft > 0)
  {
    Serial.println(meetingLength);
    Serial.println("new time");
    Serial.println(newTime);
    Serial.println("reference time");
    Serial.println(referenceTime);

    timeDiff = newTime - referenceTime;
    progress += floor(((float) timeDiff) / 60);
    Serial.println("Time diff");
    Serial.println(timeDiff);
  }



  lastProgress = progress;
  prevLength = givenLength;
  meetingLength = givenLength;
  Serial.println("time left");


  timeLeft = givenTimeLeft - floor(((float) timeDiff) / 60);
  
  colourRing(timeLeft);


  if (timeDiff >= 60)
  {
    referenceTime = newTime;
  }
  Serial.println(timeLeft);
}

void colourRing(int timeLeft)
{

  if (timeLeft <= 0)
  {
    idleAnimation();
  }
  else if (timeLeft > 5)
      {
        clearColour();
        //pixels.setPixelColor(i, pixels.Color(i * 3, 25 - i, 0)); // Moderately bright green color.
      }
  else
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      Serial.println("colouring");
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      
      // les than 5 mins remaining
     
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      
      pixels.show(); // This sends the updated pixel color to the hardware.

      delay(150); // Delay for a period of time (in milliseconds).
    }
  }


}

void idleAnimation()
{
  Serial.println("idle");
  for (int i = 0; i < NUMPIXELS; i++)
  {
          pixels.setPixelColor(i, pixels.Color(255,0, 0));   
          delay(150);
          pixels.show();
  }
}

void clearColour()
{
  for (int i = NUMPIXELS; i >= 0; i--)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(150);
  }

}

void callback(char * topic, byte * payload, unsigned int length2)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);


  Serial.print("Message: ");
  for (int i = 0; i < length2; i++)
  {
    Serial.print((char) payload[i]);
  }

  Serial.println();
  Serial.println("-------------");

  StaticJsonBuffer <300> JSONbuffer;

  payload[length2] = 0;
  String inData = String((char * ) payload);
  Serial.println(inData);
  JsonObject & root = JSONbuffer.parseObject(inData);
  //  if(root["room"]=="room1"){
  int meeting_Length = root["duration"];
  int time_Left = root["elapsed"];
  time_Left = meeting_Length - time_Left;

  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.

  updateStrip(meeting_Length, time_Left);
  meetingLength = meeting_Length;
  timeLeft = time_Left;

  //}
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect(clientName))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //  client.publish("Hello World");
      // ... and resubscribe


    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void getCurrentTime()
{
  // (timezone, daylight offset in seconds, server1, server2)
  // 3*3600 as setTimezone function converts seconds to hours
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr))
  {
    Serial.print(".");
    delay(250);
  }
  delay(1000);
  Serial.println("Got Time.");
}
void setup()
{

  Serial.begin(115200);
  getCurrentTime();
  pixels.begin(); // This initializes the NeoPixel library.
  ConnectWifi(ssid, password);
  client.setServer(mqttServer, mqttPort);
  ConnectBroker(client, clientName);
  client.setCallback(callback);
  client.subscribe(topic);

  client.loop();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
    client.subscribe(topic);
  }

  //colourRing(5,15);
  updateStrip(meetingLength, timeLeft);
  //updateRing(meetingLength,timeLeft);
  client.loop();
  delay(25);
}
