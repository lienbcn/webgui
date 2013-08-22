/**
 * File: arduino-communication.js
 * 
 * This module provides webgui with a set of methods that handles the communication between the server
 * and the arduino through the tcp connection.
 * 
 * Author: Nil Sanz Bertran
 * Created on: Mar 13, 2013
 */

var net = require('net'); //for tcp connections
var board = require('../board.js');
var Queue = require('../queue.js');
var browser = require('./browser-communication.js');

/**
 * This object is used to send and receive data through the tcp connection.
 */
var oSocket;
var oQueue = new Queue(function(sMessageToSend){
	if (oSocket.writable){
		return oSocket.write(sMessageToSend);
	}
	return false;
});

/**
 * All the data received from the arduino, is stored in the sStream variable.
 * One command may be splitted in more than one paquets or one paquet may contain some commands.
 * For this reason it is necessary to store the data received and analize it since one or more whole
 * commands are available to read.
 * @type {String}
 */
var sStream = '';

/**
 * This function creates the tcp server that is going to establish connection with the arduino.
 * @param  {number} nPort is the tcp port that the server is going to listen to.
 */
var start = function(nPort){
	//create tcp server. arduino
	var oTCPServer = net.createServer(function(c) { //'connection' listener
		oSocket = c;
		oQueue.send(); //send queued messages
		oSocket.on('connect', function(){
			console.log('A client connected to TCP server');
			//oSocket.write('Welcome to WEBGUI TCP server\n');
			sStream = '';
			browser.send({sCommand: 'connexion', bStatus: true});
		});
		oSocket.on('end', function() {
			console.log('Client disconnected from TCP server');
			sStream = '';
			browser.send({sCommand: 'connexion', bStatus: false});
		});
		oSocket.on('data', function(data){
			//console.log('client: '+JSON.stringify(data.toString()));
			onNewMessage(data.toString());
			browser.send({sCommand: 'connexion', bStatus: true});
		});
		oSocket.on('timeout', function(){
			console.log('tcp client disconnected due to timeout');
			browser.send({sCommand: 'connexion', bStatus: false});
		});
		oSocket.on('drain', function(){
			//console.log('Write buffer to tcp client became empty');
		});
		oSocket.on('error', function(){
			console.log('An error occurred about tcp connection');
			sStream = '';
			browser.send({sCommand: 'connexion', bStatus: false});
		});
		oSocket.on('close', function(){
			console.log('tcp connection became fully closed.');
			sStream = '';
			browser.send({sCommand: 'connexion', bStatus: false});
		});
	});
	oTCPServer.listen(nPort, function() { //'listening' listener
		console.log('TCP server listening on port '+nPort);
	});
};

/**
 * This function receives a message and sends it to the outcome queue in order to arduino receives
 * the message.
 * @param  {string} sMessage is the message that is going to be sent to arduino.
 */
var send = function(sMessage){
	oQueue.send(sMessage);
};

/**
 * This function is executed every time the tcp server receives data from arduino. This function stores
 * the data in the sStream variable and analize it in oreder to find possible commanmds. If a command is
 * found, newCommand function is called. It must be analized in syntax to assure it is a
 * correct command.
 * @param  {string} sData is the data that server receives.
 */
var onNewMessage = function(sData){
	sStream += sData;
	var sMessage, aMsg;
	var analizeMessage = function(sMsg){
		//make a regexp for each command and test all.
		var oreCommand = {
			AddControl: /^ADD_CONTROL:([a-zA-Z0-9])+,`[^\`\t\r\n\v\f]+`,(\b(buttons|switches|analog|string|options)\b)(,[^\t\r\n\v\f]+)*(\r\n|\n)$/,
			AddMonitor: /^ADD_MONITOR:([a-zA-Z0-9])+,`[^\`\t\r\n\v\f]+`,(\b(boolean|analog|digital|string)\b)(,[0-9,\.\-]+)?(\r\n|\n)$/,
			SetMonitor: /^SET_MONITOR:([a-zA-Z0-9])+,(on|off|`[^\`\t\r\n\v\f]*`|[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)(\r\n|\n)$/,
			Remove: /^REMOVE:([a-zA-Z0-9])+(\r\n|\n)$/,
			Reset: /^RESET(\r\n|\n)$/
		};
		var sProp;
		for (sProp in oreCommand){
			if (oreCommand.hasOwnProperty(sProp)){
				if (oreCommand[sProp].test(sMsg)){
					newCommand(sMsg, sProp);
					return;
				}
			}
		}
		console.log('Syntax error in arduino message: '+JSON.stringify(sMsg));
		oSocket.end('ERROR:syntax\n');
	};
	while(sStream){
		sStream = sStream.replace(/^(\r\n|\n)*/g, ''); //delete initial new lines
		aMsg = /^[^\r\n]*(\r\n|\n)/.exec(sStream);
		if (aMsg){ //a whole command found
			sMessage = aMsg[0];
			sStream = sStream.replace(/^[^\r\n]*(\r\n|\n)/, ''); //delete the command (sMessage)
			analizeMessage(sMessage);
		}
		else { //it has not received the whole command yet
			break;
		}
	}
};

/**
 * This function is called when a new whole command is received. It analize the command and call the
 * proper methods that are necessary to execute the command.
 * @param  {string} sMessage is the command itself.
 * @param  {sType} sType    is the type of the command
 */
var newCommand = function(sMessage, sType){
	try{
		if (sType === 'Reset'){
			board.clear();
			browser.send({'sCommand': 'refresh', 'aElements': board.getElements()});
			return true;
		}
		sMessage = sMessage.replace(/^[A-Z\_]+:/, ''); //remove "COMMAND_NAME:"
		var sId = /^([a-zA-Z0-9])+/.exec(sMessage)[0];
		if (sType === 'Remove'){
			board.deleteById(sId);
			browser.send({'sCommand': 'remove', 'sId': sId});
			return true;
		}
		sMessage = sMessage.replace(/^([a-zA-Z0-9])+,/, ''); //remove id+","
		sMessage = sMessage.replace(/[\s]+$/, ''); //remove the final newlines
		if (sType === 'SetMonitor'){
			var sElemType = board.getElementById(sId) && board.getElementById(sId).sType;
			if (!sElemType){
				oSocket.write('ERROR:The id '+sId+' does not exist\n');
				return;
			}
			var oMods = {'sId': sId};
			if (sElemType === 'boolean'){
				oMods.value = (sMessage === "on");
			}
			else if (sElemType in {'string':0, 'digital':0}){
				oMods.value = sMessage.replace(/`/g, ''); //remove grave accents. g: global to delete all matches
			}
			else {
				oMods.value = parseFloat(sMessage);
			}
			board.updateElement(oMods);
			browser.send({'sCommand': 'update', 'oModifications': oMods});
			return true;
		}
		var oNewElement = {'sId': sId};
		oNewElement.sName = /`[^\`\t\r\n\v\f]+`/.exec(sMessage)[0].slice(1, -1); //get the name field
		sMessage = sMessage.replace(/`[^\`\t\r\n\v\f]+`,/, ''); //remove name+","
		oNewElement.sType = /^(\b([a-z]+)\b)/.exec(sMessage)[0]; //get the type field
		sMessage = sMessage.replace(/^[a-z]+,/, ''); //remove type+","  depending on type
		var aux;
		if (sType === 'AddMonitor'){
			oNewElement.sTypeOfElement = 'monitor';
			if (oNewElement.sType === 'analog'){
				aux = sMessage.split(',');
				oNewElement.nMin = oNewElement.value = parseInt(aux[0], 10);
				oNewElement.nMax = parseInt(aux[1], 10);
			}
			else if(oNewElement.sType === 'digital'){
				oNewElement.value = '0';
			}
			else if(oNewElement.sType === 'string'){
				oNewElement.value = '';
			}
			else { //boolean
				oNewElement.value = false;
			}
			board.addElement(oNewElement);
			browser.send({'sCommand': 'insert', 'oElement': oNewElement});
			return true;
		}
		//sType is AddControl
		oNewElement.sTypeOfElement = 'control';
		if (oNewElement.sType === 'buttons'){
			oNewElement.asButtons = [];
			while (aux = /^`[^`]+`/.exec(sMessage)){ //while there are buttons inside sMessage. exec returns null or a non-empty array
				oNewElement.asButtons.push(aux[0].slice(1,-1)); //get the button without grave accents
				sMessage = sMessage.replace(/^`[^`]+`/, ''); //remove the first button with grave accents
				sMessage = sMessage.replace(/^,/, ''); //remove the first ','
			}
		}
		else if (oNewElement.sType === 'options'){
			oNewElement.asOptions = [];
			while (aux = /^`[^`]+`/.exec(sMessage)){ //while there are options inside sMessage. exec returns null or a non-empty array
				oNewElement.asOptions.push(aux[0].slice(1,-1)); //get the option without grave accents
				sMessage = sMessage.replace(/^`[^`]+`/, ''); //remove the first option with grave accents
				sMessage = sMessage.replace(/^,/, ''); //remove the first ','
			}
			oNewElement.value = "0";
		}
		else if (oNewElement.sType === 'switches'){
			oNewElement.value = [];
			aux = sMessage.split(',');
			for (var i = 0; i < aux.length; i++) {
				oNewElement.value.push(aux[i] === 'on');
			}
		}
		else if (oNewElement.sType === 'analog'){ //range
			aux = sMessage.split(',');
			oNewElement.nMin = parseFloat(aux[0]);
			oNewElement.nMax = parseFloat(aux[1]);
			oNewElement.value = parseFloat(aux[2]);
		}
		else if (oNewElement.sType === 'string'){
			oNewElement.value = '';
		}
		board.addElement(oNewElement);
		browser.send({'sCommand': 'insert', 'oElement': oNewElement});
		return true;
	}catch(err){
		console.log(err.name+'|'+err.message);
		return false;
	}
};

exports.start = start;
exports.send = send;
