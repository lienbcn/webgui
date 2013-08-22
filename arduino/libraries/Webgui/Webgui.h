
#ifndef Webgui_h
#define Webgui_h

#define TCPPORT 8124
#define INPUTSTREAMSIZE 128
#define MAXACTIONS 40
#define MAXMOUSEDOWNACTIONS 20

#define CHECKCONNECTION(RET) \
	if (!this->connected()){ \
		Serial.println("Reconnecting to webgui server ...");\
		if(this->_connect()){\
			Serial.println("Reconnected!");\
		}\
		else{\
			Serial.println("Server does not respond!");\
			return RET;\
		}\
	}


#define INTEGER 1
#define FLOAT 2
#define STRING 3
#define BOOLEANS 4

#include "Arduino.h"
#include "../Wifi/WiFiClient.h"
#include "../Ethernet/Ethernet.h"
#include <string.h>
#include <stdio.h>
#include <avr/wdt.h>

typedef void (*CallbackTypeInt)(int); //this type is a function that returns void and receives an int.
typedef void (*CallbackTypeFloat)(float); //this type is a function that returns void and receives a float.
typedef void (*CallbackTypeCharp)(char*); //this type is a function that returns void and receives a char*.
typedef void (*CallbackTypeBoolp)(bool*); //this type is a function that returns void and receives a bool*.

typedef struct {
	int id;
	unsigned char type; //necessary to distinguish if 12 is a float or an int
	void * fnAction;
}action_generic_t;
typedef struct {
	int id;
	CallbackTypeInt fnActionMousedown;
}action_mousedown_t;


class Webgui{
	public:
		Webgui();
		bool connect(WiFiClient * client, const char * host);
		bool connect(EthernetClient * client, const char * host);
		bool connected();
		
		int addButtons(const char * name, unsigned char numbuttons, char ** buttons, CallbackTypeInt fnMouseUp/*click*/, CallbackTypeInt fnMouseDown);
		int addButtons(const char * name, unsigned char numbuttons, char ** buttons, CallbackTypeInt fnActionClick);
		int addSwitches(const char * name, unsigned char numswitches, bool * switches, CallbackTypeBoolp fnAction);
		int addOptions(const char * name, unsigned char numoptions, char ** options, CallbackTypeInt fnAction);
		int addInputAnalog(const char * name, float minvalue, float maxvalue, float defaultvalue, CallbackTypeFloat fnAction);
		int addInputString(const char * name, CallbackTypeCharp fnAction);
		int addLED(const char * name);
		int addNeedleIndicator(const char * name, float minvalue, float maxvalue);
		int addNumericDisplay(const char * name);
		int addStringDisplay(const char * name);
		void setMonitor(int id, float value);
		void setMonitor(int id, char * value);
		void setMonitor(int id, bool value);
		void remove(int id);
		void reset();

		void update();

	private:
		WiFiClient * _wificlient;
		EthernetClient * _ethernetclient;
		void _print(const char * str);
		void _println(const char * str);
		void _println();
		const char * _server;

		action_generic_t * actionsList[MAXACTIONS]; //array of pointers to action_generic_t
		unsigned char numactions;
		action_mousedown_t * mousedownActionsList[MAXMOUSEDOWNACTIONS]; //array of pointers to action_mousedown_t
		unsigned char nummousedownactions;
		void _addAction(int id, unsigned char type, void * fnAction);
		void _addActionMousedown(int id, CallbackTypeInt fnAction);

		char inputstream[INPUTSTREAMSIZE];

		int idcounter;
		int _getId();
		char * _idtostr(char * dest, int id);
		int _stridtoint(char * id);

		bool _connect();
		void _callAction(int id, char * value, bool releaseaction);
		void _analizeStream();

		unsigned long _lastConnectionAttemptTime;
		unsigned char _connectconsecutivefails;

		void _resetArduino();
};

#endif

