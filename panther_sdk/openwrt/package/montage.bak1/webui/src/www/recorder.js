var client;
var mediaStream;
var context;
var audioInput;
var recorder;

var queryMic = 0; // a flag to check if query mic or not.

var bufferSize = 2048;
var resampleRate = 44100;

function twoWayAudioOn() {
	var u = document.URL;
	if ((typeof(client) == "undefined") && (queryMic == 0)) {
		//client = new WebSocket('ws://192.168.65.155:9000', 'dumb-increment-protocol');
		if (u.substring(0, 5) == "https")
			client = new WebSocket("wss://" + window.location.hostname + ":9000", 'dumb-increment-protocol');
		else
			client = new WebSocket("ws://" + window.location.hostname + ":9000", 'dumb-increment-protocol');
	} else if (queryMic == 1) {
		alert('please close microphone or reload web page');
		return;
	} else {
		alert('please click "OFF" first.');
		return;
	}

	showHide("twaOnb", 0);
	showHide("twaOffb", 1);

	client.onopen = function() {
		console.log('open');
	};
	client.onclose = function(e) {
		console.log('close');
		twoWayAudioOff();
	};
	client.onmessage = function(e) {
		console.log('message');
	};
	client.onerror = function(e) {
		console.log('error');
		twoWayAudioOff();
	};

	if (!navigator.getUserMedia)
		navigator.getUserMedia = navigator.getUserMedia ||
		navigator.webkitGetUserMedia ||
		navigator.mozGetUserMedia ||
		navigator.msGetUserMedia;

	queryMic = 1;
	if (navigator.getUserMedia) {
		navigator.getUserMedia({
			audio: true
		}, success, function(e) {
			alert('Error capturing audio.');
			twoWayAudioOff();
			queryMic = 0; // make sure micro has been closed 
		});
	} else {
		alert('getUserMedia not supported in this browser.');
		twoWayAudioOff();
		queryMic = 0; // make sure micro has been closed 
	}

	function success(e) {
		queryMic = 0; // make sure micro has been opened
		mediaStream = e;
		audioContext = window.AudioContext || window.webkitAudioContext;
		context = new audioContext();
		//console.log('context:', context);

		// the sample rate is in context.sampleRate
		audioInput = context.createMediaStreamSource(e);

		recorder = context.createScriptProcessor(bufferSize, 2, 2);
		var a = new Float32Array(bufferSize); // only for getting buffer type and length info
		//console.log('resampleRate:', resampleRate);
		var left_n = new Resampler(44100, resampleRate, 1, a);
		var right_n = new Resampler(44100, resampleRate, 1, a);
		//console.log('left:', left_n);
		//console.log('right:', right_n);

		recorder.onaudioprocess = function(e) {
			//var left_r = e.inputBuffer.getChannelData(0);
			//var right_r = e.inputBuffer.getChannelData(1);
			if (typeof(context) == "undefined") {
				twoWayAudioOff();
				return;
			}

			if (resampleRate == context.sampleRate) {
				var left = e.inputBuffer.getChannelData(0);
				var right = e.inputBuffer.getChannelData(1);
			} else {
				left_n.inputBuffer = e.inputBuffer.getChannelData(0);
				right_n.inputBuffer = e.inputBuffer.getChannelData(1);
				left_n.resampler(bufferSize);
				right_n.resampler(bufferSize);
				var left = left_n.outputBuffer;
				var right = right_n.outputBuffer;
			}
			//console.log('left_n:', l);
			//console.log('left_n  input[0]:'+left_n.inputBuffer[0]+' output[0]:'+left_n.outputBuffer[0]);
			//console.log('right_n input[0]:'+right_n.inputBuffer[0]+' output[0]:'+right_n.outputBuffer[0]);

			var buf_interleaved = interleave(convertoFloat32ToInt16(left), convertoFloat32ToInt16(right));

			if (typeof(client) != "undefined") {
				//console.log('send:');
				client.send(buf_interleaved);
			} else {
				twoWayAudioOff();
			}
		}

		audioInput.connect(recorder)
		recorder.connect(context.destination);
	}
};


function twoWayAudioOff() {
	showHide("twaOnb", 1);
	showHide("twaOffb", 0);
	if (typeof(client) != "undefined") {
		client.close();
		client = undefined;
	}
	if (typeof(mediaStream) != "undefined") {
		var t = mediaStream.getAudioTracks();
		t[0].stop();
		//mediaStream.stop();
		//console.log('track:', t[0]);
		//console.log('mediaStream:', mediaStream);
		mediaStream = undefined;
	}
	if (typeof(audioInput) != "undefined") {
		audioInput.disconnect();
		//console.log('audioInput:', audioInput);
		audioInput = undefined;
	}
	if (typeof(recorder) != "undefined") {
		recorder.disconnect();
		//console.log('recorder:', recorder);
		recorder = undefined;
	}
	if (typeof(context) != "undefined") {
		context.close();
		//console.log('context:', context);
		context = undefined;
	}
};

window.onbeforeunload = function() {
	twoWayAudioOff();
};

function convertoFloat32ToInt16(buffer) {
	var l = buffer.length;
	var buf = new Int16Array(l);

	while (l--) {
		buf[l] = buffer[l] * 0xFFFF; //convert to 16 bit
	}
	return buf;
}

function interleave(leftChannel, rightChannel) {
	var length = leftChannel.length + rightChannel.length;
	var result = new Int16Array(length);

	var inputIndex = 0;

	for (var index = 0; index < length;) {
		result[index++] = leftChannel[inputIndex];
		result[index++] = rightChannel[inputIndex];
		inputIndex++;
	}
	return result;
}
