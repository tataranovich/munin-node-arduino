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
            client.write("load\n");
            cmd_done = true;
          }
          if (command == String("version")) {
            client.write("munin node at arduino version: 0.1\n");
            cmd_done = true;
          }
          
          if (command.startsWith(String("config"))) {
            if (command.endsWith(String("load"))) {
              client.write("graph_title Load average\n");
              client.write("graph_args --base 1000 -l 0\n");
              client.write("graph_vlabel load\n");
              client.write("graph_scale no\n");
              client.write("graph_category system\n");
              client.write("load.label load\n");
              client.write("graph_info The load average of the machine describes how many processes are in the run-queue\n");
              args_done = true;
            }
            if (! args_done) {
              client.write("# Unknown service\n");
            }
            client.write(".\n");
            cmd_done = true;
          }
          if (command.startsWith(String("fetch"))) {
            if (command.endsWith(String("load"))) {
              client.write("load.value 0.10\n");
              args_done = true;
            }
            if (! args_done) {
              client.write("# Unknown service\n");
            }
            client.write(".\n");
            cmd_done = true;
          }
          if (! cmd_done) {
            client.write("# Unknown command. Try list, config, fetch, version or quit\n");
          }
        }
      }
    }
  }
}
