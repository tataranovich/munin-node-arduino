#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

// Default Munin node port
EthernetServer server(4949);
DHT dht(2, DHT11);
boolean cmd_done;
boolean args_done;

void setup() {
  Serial.begin(9600);
  dht.begin();
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure networking with DHCP");
    Serial.println("Using fallback IP address");
    Ethernet.begin(mac, ip);
  }
  server.begin();
  Serial.print("Munin node started at ");
  Serial.print(Ethernet.localIP());
  Serial.println(":4949");
}

void loop() {
  EthernetClient client;
  if (server.print("# munin node at arduino\n") > 0) {
    Serial.println("New connection");
    while (1) {
      client = server.available();
      if (client) {
        break;
      }
    }
  } else {
      Ethernet.maintain();
      delay(1000);
  }
  if (client) {
    char buffer[128];
    byte pos = 0;
    while (client.connected()) {
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
              client.print("temp.label DHT11\n");
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              client.print("graph_title Relative humidity\n");
              client.print("graph_category Sensors\n");
              client.print("graph_scale no\n");
              client.print("graph_vlabel %\n");
              client.print("humid.label DHT11\n");
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
              float t = dht.readTemperature();
              client.print("temp.value ");
              if (isnan(t)) {
                client.print("U");
                Serial.println("Failed to read temperature");
              } else {
                client.print(t);
                Serial.print("Temperature ");
                Serial.print(t);
                Serial.println("*C");
              }
              client.print('\n');
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              float h = dht.readHumidity();
              client.print("humid.value ");
              if (isnan(h)) {
                client.print("U");
                Serial.println("Failed to read relative humidity");
              } else {
                client.print(h);
                Serial.print("Relative humidity ");
                Serial.print(h);
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
