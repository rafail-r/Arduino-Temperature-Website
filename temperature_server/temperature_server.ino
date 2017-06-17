#include <ESP8266WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>

//first sensor input pin
#define ONE_WIRE_BUS D1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor_1(&oneWire);

// WIFI credentials
const char* ssid = "*****";
const char* password = "*****";

// Stores the HTTP request
String HTTP_req;            

// The temperature in .xx string format
char temperatureString[6];

// Default port 80
WiFiServer server(80);

void setup() {
  
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  
  Serial.print("\nAttempting to connect to SSID: ");
  Serial.println(ssid);   
  WiFi.begin(ssid, password);

  // Wait for connection
  delay(10000);

  // Start the server
  server.begin();
  
  // Print IP address
  printWifiStatus();
}


void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

    Serial.println("new client");
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        HTTP_req += c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // If it was an ajax request, send back the xml
          if (HTTP_req.indexOf("ajax_temp") > -1) {
            // Send xml with temperatures
            SendXML(client);
          }
          else {
            // Else send the HTML
            SendHTML(client);
          }
          // End of request -> reset
          HTTP_req = "";
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(100);
   
    // close the connection:
    client.stop();
  }
}


float getTemperature() {
  float temperature;
  // Read Temperature until valid value
  do {
    sensor_1.requestTemperatures(); 
    temperature = sensor_1.getTempCByIndex(0);
    delay(1000);
  } while (temperature == 85.0 || temperature == (-127.0));
  
  return temperature;
}

String SendHTML(WiFiClient cl){
  float temperature = getTemperature();
  dtostrf(temperature, 2, 2, temperatureString);
  
  // Depending on temperature, color the webpage appropriately
  String cssClass = "mediumhot";
  if (temperature < 15)
    cssClass = "cold";
  else if (temperature > 30)
    cssClass = "hot";  
  cl.println("<!DOCTYPE html>");
  cl.println("<html>");
  cl.println("<head>");
  cl.println("<title>Arduino Web Page</title>");
  cl.println("<meta charset=\"utf-8\" /><meta name=\"viewport\" content=\"width=device-width\" /><link href='https://fonts.googleapis.com/css?family=Advent+Pro' rel=\"stylesheet\" type=\"text/css\"><style>");
  cl.println("html {height: 100%;}");
  cl.println("div {color: #fff;font-family: 'Advent Pro';font-weight: 400;left: 50%;position: absolute;text-align: center;top: 50%;transform: translateX(-50%) translateY(-50%);}");
  cl.println("h2 {font-size: 90px;font-weight: 400; margin: 0}");
  cl.println("body {margin: 0; padding: 0; height: 100%;}");
  cl.println(".cold {background: linear-gradient(to bottom, #7abcff, #0665e0 );}");
  cl.println(".mediumhot {background: linear-gradient(to bottom, #81ef85,#057003);}");
  cl.println(".hot {background: linear-gradient(to bottom, #fcdb88,#d32106);}");
  cl.println("</style>");
  cl.println("<script>");
  cl.println("function GetSwitchState() {");
  cl.println("nocache = \"&nocache=\"+ Math.random() * 1000000;");
  cl.println("var request = new XMLHttpRequest();");
  cl.println("request.onreadystatechange = function() {");
  cl.println("if (this.readyState == 4) {");
  cl.println("if (this.status == 200) {");
  cl.println("if (this.responseXML != null) {");
  cl.println("document.getElementById(\"temp1\").innerHTML = \"Sensor 1 : \" + this.responseXML.getElementsByTagName('temperature1')[0].childNodes[0].nodeValue + \"<small>&deg;C</small>\";");
  cl.println("}}}}");
  cl.println("request.open(\"GET\", \"ajax_temp\" + nocache, true);");
  cl.println("request.send(null);");
  cl.println("setTimeout('GetSwitchState()', 2000);");
  cl.println("}");
  cl.println("</script>");
  cl.println("</head>");
  cl.println("<body onload=\"GetSwitchState()\" class=\"" + cssClass + "\">");
  cl.println("<div><h2 id=\"temp1\">Sensor 1 : " + String(temperatureString) + "&nbsp;<small>&deg;C</small></h2></div>");
  cl.println("</body>");
  cl.println("</html>");
}

// send the temp
void SendXML(WiFiClient cl)
{
  dtostrf(getTemperature(), 2, 2, temperatureString);
  cl.print("<?xml version = \"1.0\" ?>");
  cl.print("<temperatures>");
  cl.print("<temperature1>");
  cl.println(temperatureString);
  cl.print("</temperature1>");
  cl.print("</temperatures>");
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

}
