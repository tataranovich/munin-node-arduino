#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

// Default Munin node port
EthernetServer server(4949);
boolean cmd_done;
boolean args_done;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Munin node started at ");
  Serial.print(Ethernet.localIP());
  Serial.println(":4949");
}

void loop() {
  server.write("# munin node at arduino\n");
  delay(1000);
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New connection");
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
              client.print("temp.label DHT22\n");
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              client.print("graph_title Relative humidity\n");
              client.print("graph_category Sensors\n");
              client.print("graph_scale no\n");
              client.print("graph_vlabel %\n");
              client.print("humid.label DHT22\n");
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
              byte rand_t = random(20, 30);
              client.print("temp.value ");
              client.print(rand_t);
              client.print('\n');
              args_done = true;
            }
            if (command.endsWith(String("humid"))) {
              byte rand_h = random(30, 50);
              client.print("humid.value ");
              client.print(rand_h);
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
