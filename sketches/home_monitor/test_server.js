var dgram = require('dgram');

var s = dgram.createSocket('udp4');

s.bind(5354, function () {
	s.addMembership('239.0.100.1');
});

s.on('message', function (msg, rinfo) {
	console.log(msg, rinfo);
});
