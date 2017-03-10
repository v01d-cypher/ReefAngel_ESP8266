#include <SPI.h>
#include <ESP8266WiFi.h>


//Configure wifi_ip to match your network (gateway, subnet and dns declared in setup())
IPAddress wifi_ip(192, 168, 8, 244);
WiFiServer web_server(wifi_ip, 80);

char client_request[200] = {0};
char client_get_query[50] = {0};
char RA_response[800] = {0};
char portal_status[400] = {0};


void handle_portal() {
  while (Serial.available()) {
    //The RA portal status submission ends with "\n\n"
    int b = Serial.readBytesUntil('\n', portal_status, sizeof(portal_status) - 1);
    portal_status[b] = '\0';

    //Don't know why b > 10, copied from example code.
    //TODO: Check what RA does on serial that could interfere here.
    if (b > 10) {
      WiFiClient http_client_out;
      IPAddress reefangel_portal(104, 36, 18, 155); // forum.reefangel.com

      if (http_client_out.connect(reefangel_portal, 80)) {
        http_client_out.print(portal_status);
        http_client_out.println(" HTTP/1.0");
        http_client_out.println();
      }

      //Give client time to recieve. This matters even though it's short
      delay(1);
      http_client_out.stop();
    }
  }
}


void handle_client() {
  WiFiClient client = web_server.available();

  if (client) {
    while (client.connected()) {
      while (client.available()) {
        //TODO: The way handle_client is implemented means we can't handle basic auth, as there's no back and forth communication

        int b = client.readBytesUntil('\r', client_request, sizeof(client_request) - 1);
        client_request[b] = '\0';

        //We care only about the GET part of the client's request
        if (strstr(client_request, "GET /")) {
          strcpy(client_get_query, client_request);
        }

      }

      //Send to RA if we have a "GET /" part in the request
      if (*client_get_query) {
        //Clear serial buffer just in case there's already stuff in it.
        Serial.flush();
        while (Serial.available()) {
          Serial.read();
          delay(1);
        }

        Serial.write(client_get_query, strlen(client_get_query));

        //Flush outgoing serial, to make sure all data is sent to RA
        Serial.flush();

        //Clear client_get_query for next client
        memset(client_get_query, '\0', sizeof(client_get_query));

        //Wait for RA response because we want to complete the client handling here.
        while (!Serial.available()) {
          delay(1);
        }

        char serial_chunk[33] = {0};
        while (Serial.available() > 0) {
          //Read up to 32 bytes from serial at a time.
          int b = Serial.readBytes(serial_chunk, sizeof(serial_chunk) - 1);
          serial_chunk[b] = '\0';

          //Add it to RA response
          strcat(RA_response, serial_chunk);

          //At 57600 baud, it should take about 11 miliseconds to fill up the 64 byte serial buffer.
          //delay(1) should therefore be fine to give the buffer a bit of time to start filling.
          //We do this so that Serial.available() is true until it's done, again to make sure we handle
          //the RA response to the client here.
          delay(1);
        }

        //DEBUG: Send RA response to client as text
        //client.println("HTTP/1.1 200 OK");
        //client.println("Content-Type: text/plain");
        //client.println("Connection: close");
        //client.println();

        client.print(RA_response);

        //Give client time to receive. This matters even though it's short
        delay(1);
        client.stop();

        //Clear RA response for next client
        memset(RA_response, '\0', sizeof(RA_response));
      }
    }

    delay(1);
    client.stop();
  }
}

void setup() {
  Serial.begin(57600);
  //Serial.setTimeout(100);

  while (!Serial) {
    delay(100);
  }

  //Configure these to match your network
  IPAddress gateway(192, 168, 8, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 8, 1);

  WiFi.mode(WIFI_STA);
  
  //Assign static IP to Wifi (wifi_ip and web_server declared globally)
  WiFi.config(wifi_ip, gateway, subnet, dns);

  const char* ssid = "yourSSID";
  const char* password = "yourPassword";  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  web_server.begin();
}


void loop() {
  handle_portal();
  handle_client();
}
