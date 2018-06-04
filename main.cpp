//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include "DHT.h"
// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define DHTTYPE DHT22
#define DHTPIN 2
#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
DHT dht(DHTPIN, DHTTYPE);
const size_t bufferSize = JSON_OBJECT_SIZE(3) + 60;
DynamicJsonBuffer jsonBuffer(bufferSize);
JsonObject& mymsg = jsonBuffer.createObject();
bool calc_delay = false;
SimpleList<uint32_t> nodes;

float values[5][3]={0};
void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 5, TASK_FOREVER, &sendMessage ); // start with a one second interval
String messages[5];

void setup() {
  Serial.begin(115200);
  dht.begin();

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();


  randomSeed(analogRead(A0));

  
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}

void sendMessage() {

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
int t=0;
while (values[t][0]!=0 && t<6){t++;}
int node_id = mesh.getNodeId();

values[t][0]=(float)node_id;
values[t][1]=temperature;
values[t][2]=humidity;

  
JsonArray& idd = mymsg.createNestedArray("id");
JsonArray& tempp = mymsg.createNestedArray("temperature");
JsonArray& humm = mymsg.createNestedArray("humidity");
for (int i=0;i<5;i++){
  idd.add(values[i][0]);
  tempp.add(values[i][1]);
  humm.add(values[i][2]);
  }

  String msg;
  mymsg.printTo(msg);
  mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }
  
  Serial.printf("Sending message: %s\n", msg.c_str());

  taskSendMessage.setInterval(TASK_SECOND * 5);  // between 1 and 5 seconds
}


void receivedCallback(uint32_t from, String & msg) {
  JsonObject& msgparse = jsonBuffer.parseObject(msg);
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  int id_;
  float temperature_;
  float humidity_;
  float id_f;
  
  for (int i=0;i<5;i++){
    id_ = msgparse["id"][i];
    id_f = (float)id_;
   temperature_ = msgparse["temperature"][i];
   humidity_ = msgparse["humidity"][i];
    values[i][0] = id_f;
    values[i][1] = temperature_;
    values[i][2] = humidity_;
  }
  Serial.printf("testing receiving id:%d temp: %.2f humid: %.2f \n", id_, temperature_, humidity_);
}



void newConnectionCallback(uint32_t nodeId) {

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
