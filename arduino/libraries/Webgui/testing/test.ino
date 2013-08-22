#include <Webgui.h>
#include <WiFi.h>
 
Webgui webgui; //initialize an instance of the class

char ssid[] = "Top"; //  your network SSID (name) 
char pass[] = "0123ABCDEF";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
char server[] = "192.168.1.17";
WiFiClient client;
int status = WL_IDLE_STATUS;

int ledpin = 6;
int idt;
float val = 0;
bool forward = true;

void onButtonClick(int button){
	Serial.print(button==0 ? F("Start") : F("Stop"));
	Serial.println(F(" button was clicked"));
}
void onButtonPress(int button){
	Serial.print(button==0 ? F("Up") : F("Down"));
	Serial.println(F(" button was pressed"));
}
void onButtonRelease(int button){
	Serial.print(button==0 ? F("Up") : F("Down"));
	Serial.println(F(" button was released"));
}

void onOptionSelect(int option){
	Serial.print(F("Option selected: "));
	if (option == 2){
		Serial.println(F("Motor 3"));
	}
	if (option == 1){
		Serial.println(F("Motor 2"));
	}
	if (option == 0){
		Serial.println(F("Motor 1"));
	}
}
void onSlider(float value){
	Serial.print(F("Led power set to: "));
	int power1byte = (int)(value/100.0*255.0);
	Serial.println(power1byte);
	analogWrite(ledpin, power1byte);
}
void onMessage(char * value){
	Serial.print(F("Message from the GUI: "));
	Serial.println(value);
}
void onSwitches(bool * value){
	Serial.print(F("New switches status: "));
	for (int i = 0; i < 4; ++i){
		Serial.print(value[i] ? F("On ") : F("Off "));
	}
	Serial.println();
}

void setup(){
	pinMode(ledpin, OUTPUT);
	Serial.begin(9600);
	while(!Serial);
	Serial.println(F("Serial ready @ 9600 bauds"));
	// attempt to connect to Wifi network:
	while ( status != WL_CONNECTED) {
		status = WiFi.begin(ssid, keyIndex, pass);
	}
	Serial.println(F("Connected to wifi !"));
	while(!webgui.connect(&client, server)); //try to connect as many times as needed
	Serial.println(F("Connected to webgui server !"));
	webgui.reset(); //clear the board
	char * btns[] = {"Start", "Stop"};
	webgui.addButtons("Click", 2, btns, &onButtonClick);
	char * btns2[] = {"Up", "Down"};
	webgui.addButtons("Press / release", 2, btns2, &onButtonRelease, &onButtonPress);
	bool sws[] = {true, false, false, true};
	webgui.addSwitches("Switch panel", 4, sws, &onSwitches);
	char * opts[] = {"Motor 1", "Motor 2", "Motor 3"};
	webgui.addOptions("Select motor", 3, opts, &onOptionSelect);
	webgui.addInputAnalog("Led Power", 0, 100, 0, &onSlider);
	webgui.addInputString("Send a message to arduino", &onMessage);

	int a = webgui.addLED("Indicator on / off");
	idt = webgui.addNeedleIndicator("Speed [mph]", 0, 80);
	int c = webgui.addNumericDisplay("Temperature [F]");
	int d = webgui.addStringDisplay("Message from arduino");
	webgui.setMonitor(a, (bool)true);
	webgui.setMonitor(idt, (float)(100.0/3.0));
	webgui.setMonitor(c, (float)53.5);
	webgui.setMonitor(d, "Hello, I'm an arduino");
}

void loop(){
	webgui.update();
	//continuously move the needle indicator
	if (forward){
		val = val + 1.0;
		if (val >= 80){
			forward = false;
		}
	}
	else{
		val = val - 1.0;
		if (val <= 0){
			forward = true;
		}
	}
	webgui.setMonitor(idt, val);
}