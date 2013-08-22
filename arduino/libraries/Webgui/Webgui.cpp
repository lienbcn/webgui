
#include "Webgui.h"

Webgui::Webgui(){
	numactions = 0;
	idcounter = 1;
	memset(inputstream, 0, INPUTSTREAMSIZE);
	_lastConnectionAttemptTime = 0;
	_connectconsecutivefails = 0;
	_wificlient = NULL;
	_ethernetclient = NULL;
}

int Webgui::_getId(){
	return idcounter++;
}

char * Webgui::_idtostr(char * dest, int id){
	memset(dest, 0, 5);
	sprintf(dest, "%04x", id);
	return dest;
}

int Webgui::_stridtoint(char * sid){
	int id;
	id = (int)(strtoul(sid, NULL, 16));
	return id;
}

void Webgui::_resetArduino(){
	Serial.println(F("Restarting arduino ..."));
	delay(500);
	void (*softReset) (void) = 0; //declare reset function @ address 0
	softReset();
}

bool Webgui::_connect(){
	if (_wificlient != NULL){
		return this->connect(_wificlient, _server);
	}
	else{
		return this->connect(_ethernetclient, _server);
	}
}

void Webgui::_print(const char * str){
	if (_wificlient != NULL){
		_wificlient->print(str);
	}
	else{
		_ethernetclient->print(str);
	}
}

void Webgui::_println(const char * str){
	if (_wificlient != NULL){
		_wificlient->println(str);
	}
	else{
		_ethernetclient->println(str);
	}
}

void Webgui::_println(){
	if (_wificlient != NULL){
		_wificlient->println();
	}
	else{
		_ethernetclient->println();
	}
}

bool Webgui::connect(WiFiClient * client, const char * host){
	_wificlient = client;
	_server = host;
	if (millis() - _lastConnectionAttemptTime < 5000){
		Serial.println(F("Waiting some time before connecting to webgui server."));
		delay(1000);
		return false;
	}
	_lastConnectionAttemptTime = millis();
	int status = (_wificlient)->connect(_server, TCPPORT);
	if (status){
		this->_println(); //send some data in order to avoid arduino gets crazy
		_connectconsecutivefails = 0;
		return true;
	}
	Serial.println(F("It could not connect to webgui server."));
	if (++ _connectconsecutivefails >= 6){
		_resetArduino();
	}
	return false;
}

bool Webgui::connect(EthernetClient * client, const char * host){
	_ethernetclient = client;
	_server = host;
	if (millis() - _lastConnectionAttemptTime < 5000){
		Serial.println(F("Waiting some time before connecting to webgui server."));
		delay(1000);
		return false;
	}
	_lastConnectionAttemptTime = millis();
	int status = (_ethernetclient)->connect(_server, TCPPORT);
	if (status){
		_ethernetclient->println(); //send some data in order to avoid arduino gets crazy
		_connectconsecutivefails = 0;
		return true;
	}
	Serial.println(F("It could not connect to webgui server."));
	if (++ _connectconsecutivefails >= 6){
		_resetArduino();
	}
	return false;
}

bool Webgui::connected(){
	if (_wificlient != NULL){
		return _wificlient->connected() ? true : false;
	}
	else{
		return _ethernetclient->connected() ? true : false;
	}
}

void Webgui::update(){
	if (!this->connected()){
		if (!this->_connect()){
			return;
		}
	}
	if (_wificlient != NULL){
		while (_wificlient->available()) {
			char c = _wificlient->read();
			inputstream[strlen(inputstream)] = c;
			if (strlen(inputstream) >= INPUTSTREAMSIZE-1){
				break;
			}
		}
	}
	else{
		while (_ethernetclient->available()) {
			char c = _ethernetclient->read();
			inputstream[strlen(inputstream)] = c;
			if (strlen(inputstream) >= INPUTSTREAMSIZE-1){
				break;
			}
		}
	}
	if (strlen(inputstream) > 0){
		this->_analizeStream();
	}
}

void Webgui::_analizeStream(){
	while(strlen(inputstream) > 0){
		int firstln;
		bool found = false;
		for (firstln = 0; firstln < strlen(inputstream); firstln++) { //find the first \n:
			if (inputstream[firstln] == '\n'){
				found = true;
				break;
			}
		}
		if (!found && strlen(inputstream) >= INPUTSTREAMSIZE-2){ //too large
			memset(inputstream, 0, INPUTSTREAMSIZE); //delete all
			return;
		}
		if (!found){ //not complete command
			return;
		}
		//here there is 1 complete command
		char sid[5];
		char value[INPUTSTREAMSIZE - 12]; //max length of value is the total stream lenght minus ACTION:<id>,
		char event[16];
		int indexinstream = 12; //after ACTION:<id>,
		bool isstring = false;
		bool ismousedown = false;
		int count = 0;
		memset(sid, 0, 5);
		memset(value, 0, INPUTSTREAMSIZE - 12);
		memset(event, 0, 16);
		if (!strncmp(inputstream, "ACTION", 6)){
			strncpy(sid, inputstream+7, 4);
			if (inputstream[indexinstream] == '`'){
				isstring = true;
			}
			while(inputstream[indexinstream] != '\n'){
				value[strlen(value)] = inputstream[indexinstream];
				if(isstring){
					if (inputstream[indexinstream] == '`'){
						count ++;
					}
					if (count == 2){
						break;
					}
				}else{
					if (inputstream[indexinstream] == ','){
						value[strlen(value) -1] = '\0'; //remove last ','
						if (!strncmp(&inputstream[++indexinstream], "mousedown", 9)){
							ismousedown = true;
						}
						break;
					}
				}
				indexinstream++;
			}
			if (strlen(value) == 0){
				Serial.println(F("Error with the ACTION command. There is no value."));
				goto FINAL;
			}
			while(value[strlen(value) -1] == '\n' || value[strlen(value) -1] == '\r'){
				value[strlen(value) -1] = '\0';
			}
			this->_callAction(_stridtoint(sid), value, ismousedown);
		}
		else if(!strncmp(inputstream, "ERROR", 5)){
			for (int j = 0; j <= firstln; j++){ //print the error
				Serial.print(inputstream[j]);
			}
			_resetArduino();
		}
FINAL:
		//delete the first line
		unsigned int newlen = strlen(inputstream+firstln+1); //get the length from the \n to final
		strcpy(inputstream, inputstream+firstln+1);
		if (newlen != strlen(inputstream)){
			_resetArduino();
		}
		memset(inputstream+newlen, 0, INPUTSTREAMSIZE-newlen);
	}
}
void Webgui::_callAction(int id, char * value, bool mousedownaction){
	if (mousedownaction){
		for (int i = 0; i < nummousedownactions; ++i){
			if (id == mousedownActionsList[i]->id){
				int valueInt = (int)atoi(value);
				mousedownActionsList[i]->fnActionMousedown(valueInt);
			}
			return;
		}
	}
	else {
		for (int i = 0; i < numactions; ++i){
			if (id == actionsList[i]->id){
				if (actionsList[i]->type == INTEGER){
					int valueInt = (int)atoi(value);
					((CallbackTypeInt)actionsList[i]->fnAction)(valueInt);
					return;
				}
				else if (actionsList[i]->type == FLOAT){
					float valueFloat = (float)strtod(value, NULL);
					((CallbackTypeFloat)actionsList[i]->fnAction)(valueFloat);
					return;
				}
				else if (actionsList[i]->type == STRING){
					char * valueCharp = (char*)malloc(strlen(value) -2 + 1); //2 '`'; 1 '/0'
					if (valueCharp == NULL){
						Serial.println(F("It could not allocate memory. Out of RAM?"));
						return;
					}
					memset(valueCharp, 0, strlen(value) -2 + 1);
					strncpy(valueCharp, value+1, strlen(value) -2); //without '`';
					((CallbackTypeCharp)actionsList[i]->fnAction)(valueCharp);
					free(valueCharp);
					return;
				}
				if (actionsList[i]->type == BOOLEANS){
					int numpipes = 0;
					for (int j = 0; j < strlen(value); j++){ //count pipes '|'
						if (value[j] == '|'){
							numpipes++;
						}
					}
					bool * valueBoolp = (bool*)malloc(sizeof(bool) * (numpipes + 1));
					if (valueBoolp == NULL){
						Serial.println(F("It could not allocate memory. Out of RAM?"));
						return;
					}
					unsigned char indexVal = 1; //index of value
					unsigned char boolindex = 0; //index of array of bools
					while(boolindex < (numpipes + 1)){ //inside the bounds of array of bools
						if (value[indexVal] == 'n'){ //on
							valueBoolp[boolindex++] = true;
							indexVal += 3; //on,
						}
						else{ //off
							valueBoolp[boolindex++] = false;
							indexVal += 4; //off,
						}
					}
					((CallbackTypeBoolp)actionsList[i]->fnAction)(valueBoolp);
					free(valueBoolp);
					return;
				}
			}
		}
	}
}
void Webgui::_addAction(int id, unsigned char type, void * fnAction){
	if (numactions >= MAXACTIONS){
		Serial.println(F("There are too many actions. It could not save more actions"));
		return;
	}
	action_generic_t * act = (action_generic_t *)malloc(sizeof(action_generic_t));
	if (act == NULL){
		Serial.println(F("It could not allocate more actions"));
		return;
	}
	act->id = id;
	act->type = type;
	act->fnAction = fnAction;
	actionsList[numactions++] = act;
}
void Webgui::_addActionMousedown(int id, CallbackTypeInt fnAction){
	if (nummousedownactions >= MAXMOUSEDOWNACTIONS){
		Serial.println(F("There are too many MD actions. It could not save more actions"));
		return;
	}
	action_mousedown_t * act = (action_mousedown_t *)malloc(sizeof(action_mousedown_t));
	if (act == NULL){
		Serial.println(F("It could not allocate more actions (mousedown)"));
		return;
	}
	act->id = id;
	act->fnActionMousedown = fnAction;
	mousedownActionsList[nummousedownactions++] = act;
}

int Webgui::addButtons(const char * name, unsigned char numbuttons, char ** buttons, CallbackTypeInt fnMouseUp/*click*/, CallbackTypeInt fnMouseDown){
	char sid[5];
	int id;
	if (!numbuttons){
		return -1;
	}
	CHECKCONNECTION(-1);
	_print("ADD_CONTROL:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_print("`,buttons,");
	for ( unsigned char i = 0; i < numbuttons; i++ ){
		_print("`");
		_print(buttons[i]);
		_print("`");
		if (i != numbuttons - 1){
			_print(",");
		}
	}
	_println();
	_addAction(id, INTEGER, (void*)fnMouseUp);
	if (fnMouseDown != NULL){
		_addActionMousedown(id, fnMouseDown);
	}
	return id;
}
int Webgui::addButtons(const char * name, unsigned char numbuttons, char ** buttons, CallbackTypeInt fnActionClick){
	return this->addButtons(name, numbuttons, buttons, fnActionClick, NULL);
}
int Webgui::addSwitches(const char * name, unsigned char numswitches, bool * switches, CallbackTypeBoolp fnAction){
	char sid[5];
	int id;
	if (!numswitches){
		return -1;
	}
	CHECKCONNECTION(-1);
	_print("ADD_CONTROL:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_print("`,switches,");
	for ( unsigned char i = 0; i < numswitches; i++ ){
		if (switches[i]){
			_print("on");
		}
		else {
			_print("off");
		}
		if (i != numswitches - 1){
			_print(",");
		}
	}
	_println();
	_addAction(id, BOOLEANS, (void*)fnAction);
	return id;
}
int Webgui::addOptions(const char * name, unsigned char numoptions, char ** options, CallbackTypeInt fnAction){
	char sid[5];
	int id;
	if (!numoptions){
		return -1;
	}
	CHECKCONNECTION(-1);
	_print("ADD_CONTROL:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_print("`,options,");
	for ( unsigned char i = 0; i < numoptions; i++ ){
		_print("`");
		_print(options[i]);
		_print("`");
		if (i != numoptions - 1){
			_print(",");
		}
	}
	_println();
	_addAction(id, INTEGER, (void*)fnAction);
	return id;
}
int Webgui::addInputAnalog(const char * name, float minvalue, float maxvalue, float defaultvalue, CallbackTypeFloat fnAction){
	char sid[5];
	int id;
	if (defaultvalue > maxvalue || defaultvalue < minvalue){
		return -1;
	}
	CHECKCONNECTION(-1);
	_print("ADD_CONTROL:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_print("`,analog,");
	char aux[32];
	memset(aux, 0, 32);
	dtostrf((double)minvalue, 8, 6, aux);
	_print(aux);
	_print(",");
	memset(aux, 0, 32);
	dtostrf((double)maxvalue, 8, 6, aux);
	_print(aux);
	_print(",");
	memset(aux, 0, 32);
	dtostrf((double)defaultvalue, 8, 6, aux);
	_println(aux);
	_addAction(id, FLOAT, (void*)fnAction);
	return id;
}
int Webgui::addInputString(const char * name, CallbackTypeCharp fnAction){
	char sid[5];
	int id;
	CHECKCONNECTION(-1);
	_print("ADD_CONTROL:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_println("`,string");
	_addAction(id, STRING, (void*)fnAction);
	return id;
}
int Webgui::addLED(const char * name){
	char sid[5];
	int id;
	CHECKCONNECTION(-1);
	_print("ADD_MONITOR:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_println("`,boolean");
	return id;
}
int Webgui::addNeedleIndicator(const char * name, float minvalue, float maxvalue){
	char sid[5];
	int id;
	CHECKCONNECTION(-1);
	_print("ADD_MONITOR:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_print("`,analog,");
	char aux[32];
	memset(aux, 0, 32);
	dtostrf((double)minvalue, 8, 6, aux);
	_print(aux);
	_print(",");
	memset(aux, 0, 32);
	dtostrf((double)maxvalue, 8, 6, aux);
	_println(aux);
	return id;
}
int Webgui::addNumericDisplay(const char * name){
	char sid[5];
	int id;
	CHECKCONNECTION(-1);
	_print("ADD_MONITOR:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_println("`,digital");
	return id;
}
int Webgui::addStringDisplay(const char * name){
	char sid[5];
	int id;
	CHECKCONNECTION(-1);
	_print("ADD_MONITOR:");
	id = this->_getId();
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(name);
	_println("`,string");
	return id;
}
void Webgui::setMonitor(int id, float value){
	CHECKCONNECTION();
	_print("SET_MONITOR:");
	char sid[5];
	_idtostr(sid, id);
	_print(sid);
	_print(",");
	char aux[32];
	memset(aux, 0, 32);
	dtostrf((double)value, 8, 6, aux);
	_println(aux);
}
void Webgui::setMonitor(int id, char * value){
	CHECKCONNECTION();
	_print("SET_MONITOR:");
	char sid[5];
	_idtostr(sid, id);
	_print(sid);
	_print(",`");
	_print(value);
	_println("`");
}

void Webgui::setMonitor(int id, bool value){
	CHECKCONNECTION();
	_print("SET_MONITOR:");
	char sid[5];
	_idtostr(sid, id);
	_print(sid);
	_print(",");
	if (value){
		_println("on");
	}
	else{
		_println("off");
	}
}
void Webgui::remove(int id){
	CHECKCONNECTION();
	_print("REMOVE:");
	char sid[5];
	_idtostr(sid, id);
	_println(sid);
	//delete from action lists
	int i, j;
	for (i = 0; i < numactions; i++){
		if (actionsList[i]->id == id){
			free(actionsList[i]);
			for (j=i; j < numactions-1; j++){ //shift all the pointers to left
				actionsList[j] = actionsList[j+1];
			}
			actionsList[numactions-1] = NULL; //delete last, that is repeated
			numactions--;
		}
	}
	for (i = 0; i < nummousedownactions; i++){
		if (mousedownActionsList[i]->id == id){
			free(mousedownActionsList[i]);
			for (j=i; j < nummousedownactions-1; j++){ //shift all the pointers to left
				mousedownActionsList[j] = mousedownActionsList[j+1];
			}
			mousedownActionsList[nummousedownactions-1] = NULL; //delete last, that is repeated
			nummousedownactions--;
		}
	}
}
void Webgui::reset(){
	CHECKCONNECTION();
	_println("RESET");
	//delete all action lists
	int i;
	for (i = 0; i < numactions; i++){
		free(actionsList[i]);
	}
	numactions = 0;
	for (i = 0; i < nummousedownactions; i++){
		free(mousedownActionsList[i]);
	}
	nummousedownactions = 0;
}
