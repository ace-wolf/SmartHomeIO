#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <ESP8266HTTPClient.h>
#include <EEPROM.h>


//########## CONFIG ##########

int BUTTON_PIN = 0;
int RELAIS_PIN = 12;
int LED_PIN = 13;

//API Key (Internal one, the External key will be this String encrypted by the MAC and ChipIP)
String apiKey = "J5scc8te";

//Wifi SSID/PW for AP
String _ssid = "SmartHome Switch";
String _pass = "espconfig";

//IP Config
IPAddress ip(10, 10, 10, 10);
IPAddress mask(255, 0, 0, 0);

//Servers
ESP8266WebServer server(80); //Web Server Port

int memoryAddresses[8] = {
	0,   //ssid(String)						=> 64  =>   0-64	0
	65,  //password(String)					=> 32  =>  65-97	1
	98,  //ip(IP)							=> 15  =>  98-113	2
	114, //subnet(IP)						=> 15  => 114-129	3
	130, //gateway(IP)						=> 15  => 130-145	4

	146, //apikey(String)					=> 32  => 146-178	5

	179, //EEPROM data valid ("valid")		=> 5   => 179-184	6

	185  //Was active						=> 1   => 185-186	7
};

//########## END CONFIG ##########


bool allUp = false;
bool lastWebserverMode = false;

bool relaisOn = false;
int lastButtonState = false;

//blink
int ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 400;



void setup(void)
{
	/** START BOOT SEQUENCE **/

	//Debug Verbindung
	Serial.begin(115200);
	Serial.println("\n\nBOOTING...\n");

	//EEPROM init
	EEPROM.begin(2048); // up to 4096

	//clearEEPROM(); //INITIAL EEPROM SET

	//Pins Initialisieren
	pinMode(BUTTON_PIN, INPUT);
	pinMode(RELAIS_PIN, OUTPUT);
	pinMode(LED_PIN, OUTPUT);
	//Defaults
	digitalWrite(LED_PIN, LOW);
	digitalWrite(RELAIS_PIN, LOW);

	//Boot Delay
	delay(1000);

	//EEPROM DATEN VALIDIEREN
	String dataValidation = memGetString(memoryAddresses[6]);
	if (dataValidation != "valid")
	{
		Serial.println("Invalid EEPROM data, running firsttime setup...");
		clearEEPROM();
	}


	//PWM Frequenz ändern
	analogWriteFreq(100);

	//PWM Range ändern
	analogWriteRange(255);

	//Versuchen zu gespeichertem WLAN zu verbinden
	String ssid = memGetString(memoryAddresses[0]);
	String pass = memGetString(memoryAddresses[1]);
	if (ssid != "" && pass != "") {
		Serial.println("######################################");
		Serial.println("Found Configuration");
		Serial.println("SSID: " + ssid);
		//Serial.println("PASSWD: "+pass);
		Serial.println("IP: " + memGetString(memoryAddresses[2]));
		Serial.println("SUBNET: " + memGetString(memoryAddresses[3]));
		Serial.println("GATEWAY: " + memGetString(memoryAddresses[4]));
		Serial.println("APIKEY: " + string_to_hex(encryptDecrypt(apiKey)));
		Serial.println("######################################");


		//Wenn SSID gespeichert ist
		connectToWifi(ssid, pass);
		//Warten ob verbindung hergestellt werden kann
		int counter = 0;
		while (!wifiIsConnected()) {
			delay(500);
			Serial.print(".");
			if (counter++ == 10 * 120) break; //10 minuten versuchen
		}
		Serial.print("\n");

		//Verbunden?
		if (!wifiIsConnected()) {
			//Konnte nach 10 Sekunden nicht zu WLAN verbinden
			Serial.println("Could not connect to saved WiFi");
			//Access Point zur Konfiguration und Status uebersicht Starten
			registerAP();
			//Webserver fuer HTML Seiten starten
			registerWebserver(false);
		}
		else {
			//Erfolgreich verbunden
			allUp = true;
			Serial.println("Connected to saved WiFi " + ssid);
			Serial.println("IP is: " + WiFi.localIP().toString());


			String lastState = memGetString(memoryAddresses[7]);
			if (lastState == "1")
			{
				changePowerState(true);
			}
			else
			{
				changePowerState(false);
			}


			//Start Servers
			registerWebserver(true);
		}
	}
	else {
		//Wenn SSID nicht gespeichert ist
		Serial.println("No WiFi saved");
		//Access Point zur Konfiguration und Status Uebersicht Starten
		registerAP();
		//Webserver fuer HTML Seiten starten
		registerWebserver(false);
	}


}

void loop(void)
{
	if (allUp)
	{
		if (wifiIsConnected())
		{
			digitalWrite(LED_PIN, HIGH);
			server.handleClient();
			buttonWatcher();
		}
		else
		{
			//Neu Verbinden...
			Serial.println("Conection lost!");
			digitalWrite(LED_PIN, LOW);
			delaySec(5); //5 Sek Warten
			Serial.println("Trying to reconnect...");
			WiFi.reconnect(); //Versuchen neu zu verbinden
			digitalWrite(LED_PIN, HIGH);
			delaySec(5); //5 Sek Warten
		}
	}
	else
	{
		//Requests for Config Webinterface
		server.handleClient();

		//Blink
		unsigned long currentMillis = millis();
		if (currentMillis - previousMillis >= interval) {
			previousMillis = currentMillis;
			if (ledState == HIGH) {
				ledState = LOW;
			}
			else {
				ledState = HIGH;
			}
			digitalWrite(LED_PIN, ledState);
		}
	}
}
