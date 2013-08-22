var net = require('net');
var oSocket;
var first = {};
var server = net.createServer(function(c) { //'connection' listener
	if (!first.created){
		console.log('tcp server created.');
		first.created = true;
	}
	oSocket = c;
	oSocket.on('connect', function(){
		if (!first.connect){
			console.log('A client connected to TCP server');
			first.connect = true;
		}
	});
	oSocket.on('end', function() {
		console.log('Client disconnected from TCP server');
	});
	oSocket.on('data', function(data){
		if (data.length > 500){
			console.log('Too much data received. ('+data.toString().slice(0,16)+')Closing connection...');
			oSocket.end('ERROR:Too much data');
		}else{
			if (data.toString() !== '\r\n'){
				console.log('client: '+JSON.stringify(data.toString()));
			}
		}
	});
	oSocket.on('timeout', function(){
		console.log('tcp client disconnected due to timeout');
	});
	oSocket.on('drain', function(){
		//console.log('Write buffer to tcp client became empty');
	});
	oSocket.on('error', function(){
		console.log('An error occurred about tcp connection');
	});
	oSocket.on('close', function(){
		//console.log('tcp connection became fully closed.');
	});
});
server.listen(8124, function() { //'listening' listener
	console.log('server listening');
});



process.stdin.resume();
process.stdin.setEncoding('utf8');
process.stdin.on('data', function (chunk) {
  //console.log(outerconnection.send(chunk));
  //console.dir(oSocket);
  console.log(''+ oSocket.write(chunk));
  //oSocket.close();
});
