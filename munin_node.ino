#include <ApplicationMonitor.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

Watchdog::CApplicationMonitor ApplicationMonitor;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

// Default Munin node port
EthernetServer server(4949);
DHT dht11(2, DHT11);
DHT dht22(3, DHT22);
boolean cmd_done;
boolean args_done;

void setup() {
  Serial.begin(9600);
  ApplicationMonitor.Dump(Serial);
  ApplicationMonitor.EnableWatchdog(Watchdog::CApplicationMonitor::Timeout_4s);
  dht11.begin();
  dht22.begin();
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Munin node started at ");
  Serial.print(Ethernet.localIP());
  Serial.println(":4949");
}

void loop() {
  ApplicationMonitor.IAmAlive();
  EthernetClient client;
  if (server.print("# munin node at arduino\n") > 0) {
    Serial.println("New connection");
    while (1) {
      ApplicationMonitor.IAmAlive();
      client = server.available();
      if (client) {
        break;
      }
    }
  } else {
      ApplicationMonitor.IAmAlive();
      delay(1000);
  }
  if (client) {
    char buffer[128];
    byte pos = 0;
    while (client.connected()) {
      ApplicationMonitor.IAmAlive();
      if (client.available()) {        
        char c = client.read();
        if (c != '\n') {
          buffer[pos] = c;
          pos = pos + 1;
        } else {
          // reset flags on new command
          cmd_done = false;
          args_done = false;
          buffer[pos] = '\0';
          pos = 0;
          String command(buffer);
          Serial.print("Command: ");
          Serial.println(command);
          if (command == String("quit")) {
            client.stop();
            break;
          }
          if (command.startsWith(String("list"))) {
            client.print("temp humid\n");
            cmd_done = true;
          }
          if (command == String("version")) {
            client.print("munin node at arduino version: 0.1\n");
            cmd_done = true;
          }
          if (command.startsWith(String("config"))) {
            if (command.endsWith(String("temp"))) {
              client.print("graph_title Temperature\n");
              client.print("graph_category Sensors\n");
              client.print("graph_scale no\n");
              client.print("graph_vlabel degrees Celsius\n");
              client.print("temp1.label DHT11\n");
              client.print("temp2.label DHT22\n");
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              client.print("graph_title Relative humidity\n");
              client.print("graph_category Sensors\n");
              client.print("graph_scale no\n");
              client.print("graph_vlabel %\n");
              client.print("humid1.label DHT11\n");
              client.print("humid2.label DHT22\n");
              args_done = true;
            }
            if (! args_done) {
              client.print("# Unknown service\n");
            }
            client.print(".\n");
            cmd_done = true;
          }
          if (command.startsWith(String("fetch"))) {
            if (command.endsWith(String("temp"))) {
              float temp1 = dht11.readTemperature();
              float temp2 = dht22.readTemperature();
              client.print("temp1.value ");
              if (isnan(temp1)) {
                client.print("U");
                Serial.println("DHT11: Failed to read temperature");
              } else {
                client.print(temp1);
                Serial.print("DHT11: Temperature ");
                Serial.print(temp1);
                Serial.println("*C");
              }
              client.print("\n");
              client.print("temp2.value ");
              if (isnan(temp2)) {
                client.print("U");
                Serial.println("DHT22: Failed to read temperature");
              } else {
                client.print(temp2);
                Serial.print("DHT22: Temperature ");
                Serial.print(temp2);
                Serial.println("*C");
              }
              client.print('\n');
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              float humid1 = dht11.readHumidity();
              float humid2 = dht22.readHumidity();
              client.print("humid1.value ");
              if (isnan(humid1)) {
                client.print("U");
                Serial.println("DHT11: Failed to read relative humidity");
              } else {
                client.print(humid1);
                Serial.print("DHT11: Relative humidity ");
                Serial.print(humid1);
                Serial.println("%");
              }
              client.print("\n");
              client.print("humid2.value ");
              if (isnan(humid2)) {
                client.print("U");
                Serial.println("DHT22: Failed to read relative humidity");
              } else {
                client.print(humid2);
                Serial.print("DHT22: Relative humidity ");
                Serial.print(humid2);
                Serial.println("%");
              }
              client.print('\n');
              args_done = true;
            }
            if (! args_done) {
              client.print("# Unknown service\n");
            }
            client.print(".\n");
            cmd_done = true;
          }
          if (! cmd_done) {
            client.print("# Unknown command. Try list, config, fetch, version or quit\n");
          }
        }
      }
    }
  }
}
