#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

// Default Munin node port
EthernetServer server(4949);

#define SENSORS_MAX 5

DHT sensors[] = {
  DHT(2, DHT11),
  DHT(3, DHT22),
  DHT(5, DHT22),
  DHT(6, DHT22),
  DHT(7, DHT22)
};

boolean cmd_done;
boolean args_done;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < SENSORS_MAX; i++) {
    sensors[i].begin();
  }
  Ethernet.begin(mac, ip);
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
              client.print("temp0.label DHT11\n");
              for (int i = 1; i < SENSORS_MAX; i++) {
                client.print("temp");
                client.print(i);
                client.print(".label DHT22\n");
              }
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              client.print("graph_title Relative humidity\n");
              client.print("graph_category Sensors\n");
              client.print("graph_scale no\n");
              client.print("graph_vlabel %\n");
              client.print("humid0.label DHT11\n");
              for (int i = 1; i < SENSORS_MAX; i++) {
                client.print("humid");
                client.print(i);
                client.print(".label DHT22\n");
              }
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
              for (int i = 0; i < SENSORS_MAX; i++) {
                float temp = sensors[i].readTemperature();
                client.print("temp");
                client.print(i);
                client.print(".value ");
                if (isnan(temp)) {
                  client.print("U");
                  Serial.println("Failed to read temperature");
                } else {
                  client.print(temp);
                  Serial.print("Temperature ");
                  Serial.print(temp);
                  Serial.println("*C");
                }
                client.print("\n");
              }
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              for (int i = 0; i < SENSORS_MAX; i++) {
                float humid = sensors[i].readHumidity();
                client.print("humid");
                client.print(i);
                client.print(".value ");
                if (isnan(humid)) {
                  client.print("U");
                  Serial.println("Failed to read relative humidity");
                } else {
                  client.print(humid);
                  Serial.print("Relative humidity ");
                  Serial.print(humid);
                  Serial.println("%");
                }
                client.print("\n");
              }
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
