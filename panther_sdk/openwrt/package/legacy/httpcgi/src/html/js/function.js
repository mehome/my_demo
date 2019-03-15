var time=0;  
var delay_time=1000;  
var loop_num=0;

var xzx_module_upload_content_xhr;
var xzx_module_upload_content_ot;
var xzx_module_upload_content_oloaded;
var xzx_module_upload_content_sh;

var xzx_module_bluetooth_upload_content_xhr;
var xzx_module_bluetooth_upload_content_ot;
var xzx_module_bluetooth_upload_content_oloaded;
var xzx_module_bluetooth_upload_content_sh;

var bluetooth_time=0;  
var bluetooth_delay_time=1000;  
var bluetooth_loop_num=0;

var Conexant_sys_time=0;
var Conexant_sys_delay_time=1000;
var Conexant_sys_loop_num=0;

var Conexant_bin_time=0;
var Conexant_bin_delay_time=1000;
var Conexant_bin_loop_num=0;

var micarray_recording_info_mics = 0;
var micarray_recording_info_refs = 0;

function xzx_module_upload_content_upgrade()
{
	var xzx_module_upload_content_progressBar = document.getElementById("xzx_module_upload_content_progressBar");
	var xzx_module_upload_content_percentageDiv = document.getElementById("xzx_module_upload_content_percentage");
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				if ((response.upgrade < 100) && (response.upgrade >=0 ))
				{
					xzx_module_upload_content_progressBar.max = 100;
					xzx_module_upload_content_progressBar.value = response.upgrade;
					xzx_module_upload_content_percentageDiv.innerHTML = Math.round(response.upgrade / 100 * 100) + "%";
				} else {
					if (response.upgrade == 100)
					{
						xzx_module_upload_content_progressBar.max = 100;
						xzx_module_upload_content_progressBar.value = response.upgrade;
						xzx_module_upload_content_percentageDiv.innerHTML = Math.round(response.upgrade / 100 * 100) + "%";
					}
					window.clearInterval(xzx_module_upload_content_sh);
				}
			} else {
				window.clearInterval(xzx_module_upload_content_sh);
			}
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=GetDownloadProgress", true);
	xmlhttp.send();
}

function xzx_module_upload_content_uploadComplete(evt)
{
	document.getElementById("xzx_module_upload_content_Progress_title").innerHTML = "upgrade: ";
	
	var xzx_module_upload_content_progressBar = document.getElementById("xzx_module_upload_content_progressBar");
	var xzx_module_upload_content_percentageDiv = document.getElementById("xzx_module_upload_content_percentage");
	xzx_module_upload_content_progressBar.max = 100;
	xzx_module_upload_content_progressBar.value = 0;
	xzx_module_upload_content_percentageDiv.innerHTML = "0%";
	
	var xzx_module_upload_content_time = document.getElementById("xzx_module_upload_content_time");
	xzx_module_upload_content_time.innerHTML = "";
	xzx_module_upload_content_sh = window.setInterval(function(){xzx_module_upload_content_upgrade()}, 1000);
}

function xzx_module_upload_content_uploadFailed(evt)
{
	alert("upload firmware error!");
}

function xzx_module_upload_content_cancleUploadFile()
{
	xzx_module_upload_content_xhr.abort();
}

function xzx_module_upload_content_progressFunction(evt)
{
	var xzx_module_upload_content_progressBar = document.getElementById("xzx_module_upload_content_progressBar");
	var xzx_module_upload_content_percentageDiv = document.getElementById("xzx_module_upload_content_percentage");
	
	if (evt.lengthComputable)
	{
		xzx_module_upload_content_progressBar.max = evt.total;
		xzx_module_upload_content_progressBar.value = evt.loaded;
		xzx_module_upload_content_percentageDiv.innerHTML = Math.round(evt.loaded / evt.total * 100) + "%";
	}
	
	var xzx_module_upload_content_time = document.getElementById("xzx_module_upload_content_time");
	var nt = new Date().getTime();
	var pertime = (nt - xzx_module_upload_content_ot)/1000;
	xzx_module_upload_content_ot = new Date().getTime();
	
	var perload = evt.loaded - xzx_module_upload_content_oloaded;
	xzx_module_upload_content_oloaded = evt.loaded;
	
	var speed = perload/pertime;
	var bspeed = speed;
	var units = 'b/s';
	if (speed/1024>1) {
		speed = speed/1024;
		units = 'k/s';
	}
	if (speed/1024>1) {
		speed = speed/1024;
		units = 'M/s';
	}
	speed = speed.toFixed(1);
	
	var resttime = ((evt.total-evt.loaded)/bspeed).toFixed(1);
	xzx_module_upload_content_time.innerHTML = ', Speed: ' + speed + units + ', Time remaining: ' + resttime + 's';
	if (bspeed == 0)
		xzx_module_upload_content_time.innerHTML = 'upload has been canceled!';
}

function xzx_module_upload_content_uploadFile() 
{
	var fileObj = document.getElementById("xzx_module_upload_content_file").files[0];
	var url = "/cgi-bin/posthtmlupdate.cgi";
	
	var form = new FormData();
	form.append("mf", fileObj);
	
	xzx_module_upload_content_xhr = new XMLHttpRequest();
	xzx_module_upload_content_xhr.open("post", url, true);
	xzx_module_upload_content_xhr.onload = xzx_module_upload_content_uploadComplete;
	xzx_module_upload_content_xhr.onerror = xzx_module_upload_content_uploadFailed;
	xzx_module_upload_content_xhr.upload.onprogress = xzx_module_upload_content_progressFunction;
	xzx_module_upload_content_xhr.upload.onloadstart = function() {
		xzx_module_upload_content_ot = new Date().getTime();
		xzx_module_upload_content_oloaded = 0;
	};
	xzx_module_upload_content_xhr.send(form);
	
	document.getElementById("xzx_module_upload_content_Progress_title").innerHTML = "upload:   ";
}

function xzx_module_bluetooth_upload_content_upgrade()
{
	var xzx_module_bluetooth_upload_content_progressBar = document.getElementById("xzx_module_bluetooth_upload_content_progressBar");
	var xzx_module_bluetooth_upload_content_percentageDiv = document.getElementById("xzx_module_bluetooth_upload_content_percentage");
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				if ((response.upgrade < 100) && (response.upgrade >=0 ))
				{
					xzx_module_bluetooth_upload_content_progressBar.max = 100;
					xzx_module_bluetooth_upload_content_progressBar.value = response.upgrade;
					xzx_module_bluetooth_upload_content_percentageDiv.innerHTML = Math.round(response.upgrade / 100 * 100) + "%";
				} else {
					if (response.upgrade == 100)
					{
						xzx_module_bluetooth_upload_content_progressBar.max = 100;
						xzx_module_bluetooth_upload_content_progressBar.value = response.upgrade;
						xzx_module_bluetooth_upload_content_percentageDiv.innerHTML = Math.round(response.upgrade / 100 * 100) + "%";
					}
					window.clearInterval(xzx_module_bluetooth_upload_content_sh);
				}
			} else {
				window.clearInterval(xzx_module_bluetooth_upload_content_sh);
			}
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=GetBluetoothDownloadProgress", true);
	xmlhttp.send();
}

function xzx_module_bluetooth_upload_content_uploadComplete(evt)
{
	document.getElementById("xzx_module_bluetooth_upload_content_Progress_title").innerHTML = "upgrade: ";
	
	var xzx_module_bluetooth_upload_content_progressBar = document.getElementById("xzx_module_bluetooth_upload_content_progressBar");
	var xzx_module_bluetooth_upload_content_percentageDiv = document.getElementById("xzx_module_bluetooth_upload_content_percentage");
	xzx_module_bluetooth_upload_content_progressBar.max = 100;
	xzx_module_bluetooth_upload_content_progressBar.value = 0;
	xzx_module_bluetooth_upload_content_percentageDiv.innerHTML = "0%";
	
	var xzx_module_bluetooth_upload_content_time = document.getElementById("xzx_module_bluetooth_upload_content_time");
	xzx_module_bluetooth_upload_content_time.innerHTML = "";
	xzx_module_bluetooth_upload_content_sh = window.setInterval(function(){xzx_module_bluetooth_upload_content_upgrade()}, 1000);
}

function xzx_module_bluetooth_upload_content_uploadFailed(evt)
{
	alert("upload firmware error!");
}

function xzx_module_bluetooth_upload_content_cancleUploadFile()
{
	xzx_module_bluetooth_upload_content_xhr.abort();
}

function xzx_module_bluetooth_upload_content_progressFunction(evt)
{
	var xzx_module_bluetooth_upload_content_progressBar = document.getElementById("xzx_module_bluetooth_upload_content_progressBar");
	var xzx_module_bluetooth_upload_content_percentageDiv = document.getElementById("xzx_module_bluetooth_upload_content_percentage");
	
	if (evt.lengthComputable)
	{
		xzx_module_bluetooth_upload_content_progressBar.max = evt.total;
		xzx_module_bluetooth_upload_content_progressBar.value = evt.loaded;
		xzx_module_bluetooth_upload_content_percentageDiv.innerHTML = Math.round(evt.loaded / evt.total * 100) + "%";
	}
	
	var xzx_module_bluetooth_upload_content_time = document.getElementById("xzx_module_bluetooth_upload_content_time");
	var nt = new Date().getTime();
	var pertime = (nt - xzx_module_bluetooth_upload_content_ot)/1000;
	xzx_module_bluetooth_upload_content_ot = new Date().getTime();
	
	var perload = evt.loaded - xzx_module_bluetooth_upload_content_oloaded;
	xzx_module_bluetooth_upload_content_oloaded = evt.loaded;
	
	var speed = perload/pertime;
	var bspeed = speed;
	var units = 'b/s';
	if (speed/1024>1) {
		speed = speed/1024;
		units = 'k/s';
	}
	if (speed/1024>1) {
		speed = speed/1024;
		units = 'M/s';
	}
	speed = speed.toFixed(1);
	
	var resttime = ((evt.total-evt.loaded)/bspeed).toFixed(1);
	xzx_module_bluetooth_upload_content_time.innerHTML = ', Speed: ' + speed + units + ', Time remaining: ' + resttime + 's';
	if (bspeed == 0)
		xzx_module_bluetooth_upload_content_time.innerHTML = 'upload has been canceled!';
}

function xzx_module_bluetooth_upload_content_uploadFile() 
{
	var fileObj = document.getElementById("xzx_module_bluetooth_upload_content_file").files[0];
	var url = "/cgi-bin/postbluetoothupdate.cgi";
	
	var form = new FormData();
	form.append("mf", fileObj);
	
	xzx_module_bluetooth_upload_content_xhr = new XMLHttpRequest();
	xzx_module_bluetooth_upload_content_xhr.open("post", url, true);
	xzx_module_bluetooth_upload_content_xhr.onload = xzx_module_bluetooth_upload_content_uploadComplete;
	xzx_module_bluetooth_upload_content_xhr.onerror = xzx_module_bluetooth_upload_content_uploadFailed;
	xzx_module_bluetooth_upload_content_xhr.upload.onprogress = xzx_module_bluetooth_upload_content_progressFunction;
	xzx_module_bluetooth_upload_content_xhr.upload.onloadstart = function() {
		xzx_module_bluetooth_upload_content_ot = new Date().getTime();
		xzx_module_bluetooth_upload_content_oloaded = 0;
	};
	xzx_module_bluetooth_upload_content_xhr.send(form);
	
	document.getElementById("xzx_module_bluetooth_upload_content_Progress_title").innerHTML = "upload:   ";
}

function xzx_module_upload_sendClicked(F)  
{
	if(document.usb_upload.uploadedfile.value == "")
	{  
		document.usb_upload.uploadedfile.focus();  
		alert('Please select the firmware!');  
		return false;  
	}  
	
	F.submit();  
	xzx_module_upload_bartimer = window.setInterval(function(){xzx_module_upload_setProcess();}, 1000);
}

function xzx_module_bluetooth_upload_sendClicked(F)  
{
	if(document.bluetooth_usb_upload.bluetooth_uploadedfile.value == "")
	{  
		document.bluetooth_usb_upload.bluetooth_uploadedfile.focus();  
		alert('File name can not be empty !');  
		return false;  
	}  
			
	F.submit();  
	xzx_module_bluetooth_upload_bartimer = window.setInterval(function(){xzx_module_bluetooth_upload_setProcess();}, 1000);
}

function init()	
{
	show_div(0, "progress_div");
}

function bluetooth_init()	
{
	bluetooth_show_div(0, "bluetooth_progress_div");
}  

function start_init()
{
	init();
	bluetooth_init();
	Conexant_sys_init();
	Conexant_bin_init();
}

function my_OK()
{
	setTimeout(function(){init()}, 150000);
	setTimeout(function(){alert("upgrade succeed!")}, 150000);
}

function bluetooth_my_OK()
{
	setTimeout(function(){bluetooth_init()}, 120000);
	setTimeout(function(){alert("upgrade succeed!")}, 120000);
}

function my_Error(str)
{
	alert(str);
	init();
}  

function bluetooth_my_Error(str)
{
	alert(str);
	bluetooth_init();
}  

function start_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_recording_content").innerHTML = "start recording OK!";
			} else {
				document.getElementById("xzx_module_recording_content").innerHTML = "start recording err!";
			}
		} else {
			document.getElementById("xzx_module_recording_content").innerHTML = "start recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=xzxAction:startrecord", true);

	xmlhttp.send(null);
}

function stop_recording() 
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_recording_content").innerHTML = "stop recording OK!";
			} else {
				document.getElementById("xzx_module_recording_content").innerHTML = "stop recording err!";
			}
		} else {
			document.getElementById("xzx_module_recording_content").innerHTML = "stop recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=xzxAction:stoprecord", true);
	xmlhttp.send();
}

function timing_recording() 
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_recording_content").innerHTML = "timing recording OK!";
			} else {
				document.getElementById("xzx_module_recording_content").innerHTML = "timing recording err!";
			}
		} else {
			document.getElementById("xzx_module_recording_content").innerHTML = "timing recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=xzxAction:fixedrecord", true);
	xmlhttp.send();
}

function download_file(url)
{
	try {
		var elemIF = document.createElement("iframe");
		elemIF.src = url;
		elemIF.style.display = "none";
		document.body.appendChild(elemIF);
	} catch(e) {

	}
}

function download_file2(url)
{
	window.open(url, '_blank');
}

function download_file3(url)
{
	var element = document.createElement('a');
	element.setAttribute('href', url);
	element.setAttribute('download', '');
	element.style.display = 'none';
	document.body.appendChild(element);
	element.click();
	document.body.removeChild(element);
}

function get_recording_file() 
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_recording_content").innerHTML = "get recording file OK!";
				download_file(response.url)
			} else {
				document.getElementById("xzx_module_recording_content").innerHTML = "get recording file err!";
			}
		} else {
			document.getElementById("xzx_module_recording_content").innerHTML = "get recording file err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=xzxAction:getrecordfileurl", true);
	xmlhttp.send();
}

function compare_channel()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_recording_content").innerHTML = "left:" + response.left + " right:" + response.right;
			} else {
				document.getElementById("xzx_module_recording_content").innerHTML = "compare err!";
			}
		} else {
			document.getElementById("xzx_module_recording_content").innerHTML = "compare err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=xzxAction:compare", true);
	xmlhttp.send();
}

function dot_normal_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot normal recording ...";
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot normal recording err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot normal recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRecord:cx20921:denoise:5", true);
	xmlhttp.send();
}

function dot_microphone_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot microphone recording ...";
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot microphone recording err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot microphone recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRecord:cx20921:mic:5", true);
	xmlhttp.send();
}

function dot_reference_signal_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot reference signal recording ...";
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot reference signal recording err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot reference signal recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRecord:cx20921:ref:5", true);
	xmlhttp.send();
}

function dot_get_normal_recording_file()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get normal recording file OK!";
				download_file(response.url)
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get recording file err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get recording file err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayMic:cx20921:denoise", true);
	xmlhttp.send();
}

function dot_get_microphone_recording_file()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get microphone recording file OK!";
				download_file(response.url)
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get microphone recording file err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get microphone recording file err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayMic:cx20921:mic", true);
	xmlhttp.send();
}

function dot_get_reference_signal_recording_file()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get signal recording file OK!";
				download_file(response.url)
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get signal recording file err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot get signal recording file err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRef:cx20921", true);
	xmlhttp.send();
}

function dot_stop_recording() 
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot stop recording OK!";
				download_file(response.url)
			} else {
				document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot stop recording err!";
			}
		} else {
			document.getElementById("xzx_module_dot_recording_content").innerHTML = "dot stop recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayStop:cx20921", true);
	xmlhttp.send();
}

function micarray_get_info()
{
	var xmlhttp;
	var xzx_module_micarray_MicarrayMic_file_select;
	var xzx_module_micarray_MicarrayRef_file_select;
				
	micarray_recording_info_mics = 0;
	micarray_recording_info_refs = 0;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
	/*	var x1 = document.getElementById("xzx_module_micarray_recording_content").innerHTML = xmlhttp.readyState;
		var x2 = document.getElementById("xzx_module_micarray_recording_content").innerHTML = xmlhttp.status;
		alert("x1="+x1);
		alert("x2="+x2);
		*/
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
		//	var x3 = document.getElementById("xzx_module_micarray_recording_content").innerHTML =response.ret;
		//	alert("response="+x3);
			if (response.ret == "OK")
			{
				micarray_recording_info_mics = response.mics;
				micarray_recording_info_refs = response.refs;
				
				for (i = 0; i < micarray_recording_info_mics; i++)
				{
					xzx_module_micarray_MicarrayMic_file_select += '<option value="' + (i+1) + '">' + (i+1) + '</option>';
				}
				document.getElementById("xzx_module_micarray_MicarrayMic_file").innerHTML = xzx_module_micarray_MicarrayMic_file_select;
				
				for (i = 0; i < micarray_recording_info_refs; i++)
				{
					xzx_module_micarray_MicarrayRef_file_select += '<option value="'+ (i+1) + '">' + (i+1) + '</option>';
				}
				document.getElementById("xzx_module_micarray_MicarrayRef_file").innerHTML = xzx_module_micarray_MicarrayRef_file_select;
				
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "mics: " + micarray_recording_info_mics + "<br/>refs: " + micarray_recording_info_refs;
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get info err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get info err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=GetMicarrayInfo", true);
	xmlhttp.send();
}

function micarray_start_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
	/*	var x1 = document.getElementById("xzx_module_micarray_recording_content").innerHTML = xmlhttp.readyState;
		var x2 = document.getElementById("xzx_module_micarray_recording_content").innerHTML = xmlhttp.status;
		alert("x1="+x1);
		alert("x2="+x2);
*/
		if (xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
		//	var x3 = document.getElementById("xzx_module_micarray_recording_content").innerHTML =response.ret;
		//	alert("response="+x3);


			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray start recording ...";
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "1micarray start recording err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray start recording ok!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRecord:micarray:mic:5", true);
	xmlhttp.send();
}


function micarray_stop_recording()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray stop recording ...";
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray stop recording err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray stop recording ok!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayStop:micarray", true);
	xmlhttp.send();
}

function micarray_get_normal_recording_file()
{
	var xmlhttp;
	var download_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				download_url = "http://" + window.location.host + "/" + response.url;
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get normal recording file ... " + download_url;
				download_file(download_url);
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get normal recording err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get normal recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayMic:micarray:denoise", true);
	xmlhttp.send();
}

function micarray_get_mic_recording_file()
{
	var xmlhttp;
	var download_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get mic recording ...";
				
				var select_id = document.getElementById("xzx_module_micarray_MicarrayMic_file").value;				
				for (var i = 0; i < response.miclist.length; i++)
				{
					if (select_id == response.miclist[i].mic)
					{
						download_url = "http://" + window.location.host + "/" + response.miclist[i].url;
						document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get mic recording file ... " + download_url;
						download_file(download_url);
					}
				}
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get mic recording err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get mic recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayMic:micarray:mic", true);
	xmlhttp.send();
}

function micarray_get_ref_recording_file()
{
	var xmlhttp;
	var download_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				
				var select_id = document.getElementById("xzx_module_micarray_MicarrayRef_file").value;				
				for (var i = 0; i < response.reflist.length; i++)
				{
					if (select_id == response.reflist[i].refs)
					{
						download_url = "http://" + window.location.host + "/" + response.reflist[i].url;
						document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get ref recording file ..." + download_url;
						download_file(download_url)
					}
				}
			} else {
				document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get ref recording err!";
			}
		} else {
			document.getElementById("xzx_module_micarray_recording_content").innerHTML = "micarray get ref recording err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=MicarrayRef:micarray", true);
	xmlhttp.send();
}

function xzx_module_VAD_scope_change(obj)
{
	var VAD_scope = document.getElementById("xzx_module_VAD_scope").value;
	
	if (!((VAD_scope >= 500000) && (VAD_scope <= 2000000)))
	{
		window.alert("500000 <= Scope of activities <= 2000000");
		document.getElementById("xzx_module_VAD_scope").value = 500000;
	}
}

function start_VAD()
{
	var xmlhttp;
	var scope, sound_card;
	var command_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_VAD_view").innerHTML = "VAD test ...";
			} else {
				document.getElementById("xzx_module_VAD_view").innerHTML = "VAD test err!";
			}
		} else {
			document.getElementById("xzx_module_VAD_view").innerHTML = "VAD test err!";
		}
	}
	
	sound_card = document.getElementById("xzx_module_VAD_sound_card").value;
	scope = document.getElementById("xzx_module_VAD_scope").value;
	
	command_url = "/cgi-bin/httpapi.cgi?command=xzxAction:vad:" + scope + ":" + sound_card;
	
	xmlhttp.open("GET", command_url, true);
	xmlhttp.send();
}

function reset_VAD()
{
	document.getElementById("xzx_module_VAD_scope").value="500000";
	document.getElementById("xzx_module_VAD_sound_card").value="default";
	document.getElementById("xzx_module_VAD_view").innerHTML = "";
}

function Conexant_sys_getRefToDivNest(divID, oDoc)   
{
	if( !oDoc ) { 
		oDoc = document; 
	}
	
	if( document.layers ) {
		if( oDoc.layers[divID] ) { return oDoc.layers[divID]; } else {
			for( var x = 0, y; !y && x < oDoc.layers.length; x++ ) {
				y = Conexant_sys_getRefToDivNest(divID,oDoc.layers[x].document);
			}
			return y; 
		} 
	}
	
	if( document.getElementById ) 
	{ 
		return document.getElementById(divID); 
	}  

	if( document.all ) 
	{ 
		return document.all[divID];
	}
	
	return document[divID];  
}

function Conexant_sys_progressBar( oBt, oBc, oBg, oBa, oWi, oHi, oDr )   
{
	Conexant_sys_MWJ_progBar++; 
	this.id = 'Conexant_sys_MWJ_progBar' + Conexant_sys_MWJ_progBar; 
	this.dir = oDr; 
	this.width = oWi; 
	this.height = oHi; 
	this.amt = 0;  
	
	//write the bar as a layer in an ilayer in two tables giving the border  
	document.write( '<span id = "Conexant_sys_progress_div" class = "off" > <table border="0" cellspacing="0" cellpadding="'+oBt+'">'+  
	'<tr><td>Please wait...</td></tr><tr><td bgcolor="'+oBc+'">'+  
	'<table border="0" cellspacing="0" cellpadding="0"><tr><td height="'+oHi+'" width="'+oWi+'" bgcolor="'+oBg+'">'
	);  
				
	if( document.layers ) {
		document.write( '<ilayer height="'+oHi+'" width="'+oWi+'"><layer bgcolor="'+oBa+
		'" name="Conexant_sys_MWJ_progBar'+Conexant_sys_MWJ_progBar+'"></layer></ilayer>' 
		);
	} else {
		document.write( '<div style="position:relative;top:0px;left:0px;height:'+oHi+'px;width:'+oWi+';">'+  
		'<div style="position:absolute;top:0px;left:0px;height:0px;width:0;font-size:1px;background-color:'+oBa+
		';" id="Conexant_sys_MWJ_progBar'+Conexant_sys_MWJ_progBar+'"></div></div>' 
		);
	}  

	document.write( '</td></tr></table></td></tr></table></span>\n' );
	this.setBar = Conexant_sys_resetBar; //doing this inline causes unexpected bugs in early NS4  
	this.setCol = Conexant_sys_setColour;  
}  

function Conexant_sys_resetBar( a, b )   
{
	// work out the required size and use various methods to enforce it  
	this.amt = ( typeof( b ) == 'undefined' ) ? a : b ? ( this.amt + a ) : ( this.amt - a );  
	if( isNaN( this.amt ) ) 
	{ 
		this.amt = 0; 
	} 
	if( this.amt > 1 ) 
	{ 
		this.amt = 1; 
	} 
	if( this.amt < 0 ) 
	{ 
		this.amt = 0; 
	}  
		
	var theWidth = Math.round( this.width * ( ( this.dir % 2 ) ? this.amt : 1 ) );
	var theHeight = Math.round( this.height * ( ( this.dir % 2 ) ? 1 : this.amt ) );  
	var theDiv = Conexant_sys_getRefToDivNest( this.id ); 
	if( !theDiv ) 
	{ 
		window.status = 'Progress: ' + Math.round( 100 * this.amt ) + '%'; return; 
	}
	
	if( theDiv.style ) 
	{ 
		theDiv = theDiv.style; 
		theDiv.clip = 'rect(0px '+theWidth+'px '+theHeight+'px 0px)'; 
	}  

	var oPix = document.childNodes ? 'px' : 0;
	theDiv.width = theWidth + oPix; 
	theDiv.pixelWidth = theWidth; 
	theDiv.height = theHeight + oPix; 
	theDiv.pixelHeight = theHeight;  
	
	if( theDiv.resizeTo ) 
	{
		theDiv.resizeTo( theWidth, theHeight ); 
	}  
	
	theDiv.left = ( ( this.dir != 3 ) ? 0 : this.width - theWidth ) + oPix; 
	theDiv.top = ( ( this.dir != 4 ) ? 0 : this.height - theHeight ) + oPix;  
}  

function Conexant_sys_setColour( a )   
{
	//change all the different colour styles
	var theDiv = Conexant_sys_getRefToDivNest( this.id ); 
	if( theDiv.style ) { 
		theDiv = theDiv.style; 
	}  
	theDiv.bgColor = a; 
	theDiv.backgroundColor = a; 
	theDiv.background = a;  
}  

function Conexant_sys_show_div(Conexant_sys_show,Conexant_sys_id) {  
	if(Conexant_sys_show) {  
		document.getElementById(Conexant_sys_id).style.display  = "block";  
	} else {  
		document.getElementById(Conexant_sys_id).style.display  = "none";  
	}
}  

function Conexant_sys_progress()  
{
	if (Conexant_sys_time < 1)
	{
		Conexant_sys_time = Conexant_sys_time + 0.033;
	} else {
		Conexant_sys_time = 0;  
		Conexant_sys_loop_num++;  
	}  

	setTimeout('Conexant_sys_progress()',Conexant_sys_delay_time);
	Conexant_sys_myProgBar.setBar(Conexant_sys_time);  
}  

function Conexant_sys_sendClicked(F)  
{
	if(document.Conexant_sys_usb_upload.Conexant_sys_uploadedfile.value == "")
	{  
		document.Conexant_sys_usb_upload.Conexant_sys_uploadedfile.focus();  
		alert('File name can not be empty !');  
		return false;  
	}  
			
	F.submit();  
	Conexant_sys_show_div(true, "Conexant_sys_progress_div");  
	Conexant_sys_progress();
}

function Conexant_sys_init()	
{
	Conexant_sys_show_div(0, "Conexant_sys_progress_div");
}

function Conexant_sys_my_OK()
{
	setTimeout(function(){Conexant_sys_init()}, 0);
	setTimeout(function(){alert("upgrade succeed!")}, 0);
}

function Conexant_sys_my_Error(str)
{
	alert(str);
	Conexant_sys_init();
}  

/////////////////////////////////////////////////////////////////////////////////////////////////////
function Conexant_bin_getRefToDivNest(divID, oDoc)   
{
	if( !oDoc ) { 
		oDoc = document; 
	}
	
	if( document.layers ) {
		if( oDoc.layers[divID] ) { return oDoc.layers[divID]; } else {
			for( var x = 0, y; !y && x < oDoc.layers.length; x++ ) {
				y = Conexant_bin_getRefToDivNest(divID,oDoc.layers[x].document);
			}
			return y; 
		} 
	}
	
	if( document.getElementById ) 
	{ 
		return document.getElementById(divID); 
	}  

	if( document.all ) 
	{ 
		return document.all[divID];
	}
	
	return document[divID];  
}

function Conexant_bin_progressBar( oBt, oBc, oBg, oBa, oWi, oHi, oDr )   
{
	Conexant_bin_MWJ_progBar++; 
	this.id = 'Conexant_bin_MWJ_progBar' + Conexant_bin_MWJ_progBar; 
	this.dir = oDr; 
	this.width = oWi; 
	this.height = oHi; 
	this.amt = 0;  
	
	//write the bar as a layer in an ilayer in two tables giving the border  
	document.write( '<span id = "Conexant_bin_progress_div" class = "off" > <table border="0" cellspacing="0" cellpadding="'+oBt+'">'+  
	'<tr><td>Please wait...</td></tr><tr><td bgcolor="'+oBc+'">'+  
	'<table border="0" cellspacing="0" cellpadding="0"><tr><td height="'+oHi+'" width="'+oWi+'" bgcolor="'+oBg+'">'
	);  
				
	if( document.layers ) {
		document.write( '<ilayer height="'+oHi+'" width="'+oWi+'"><layer bgcolor="'+oBa+
		'" name="Conexant_bin_MWJ_progBar'+Conexant_bin_MWJ_progBar+'"></layer></ilayer>' 
		);
	} else {
		document.write( '<div style="position:relative;top:0px;left:0px;height:'+oHi+'px;width:'+oWi+';">'+  
		'<div style="position:absolute;top:0px;left:0px;height:0px;width:0;font-size:1px;background-color:'+oBa+
		';" id="Conexant_bin_MWJ_progBar'+Conexant_bin_MWJ_progBar+'"></div></div>' 
		);
	}  

	document.write( '</td></tr></table></td></tr></table></span>\n' );
	this.setBar = Conexant_bin_resetBar; //doing this inline causes unexpected bugs in early NS4  
	this.setCol = Conexant_bin_setColour;  
}  

function Conexant_bin_resetBar( a, b )   
{
	// work out the required size and use various methods to enforce it  
	this.amt = ( typeof( b ) == 'undefined' ) ? a : b ? ( this.amt + a ) : ( this.amt - a );  
	if( isNaN( this.amt ) ) 
	{ 
		this.amt = 0; 
	} 
	if( this.amt > 1 ) 
	{ 
		this.amt = 1; 
	} 
	if( this.amt < 0 ) 
	{ 
		this.amt = 0; 
	}  
		
	var theWidth = Math.round( this.width * ( ( this.dir % 2 ) ? this.amt : 1 ) );
	var theHeight = Math.round( this.height * ( ( this.dir % 2 ) ? 1 : this.amt ) );  
	var theDiv = Conexant_bin_getRefToDivNest( this.id ); 
	if( !theDiv ) 
	{ 
		window.status = 'Progress: ' + Math.round( 100 * this.amt ) + '%'; return; 
	}
	
	if( theDiv.style ) 
	{ 
		theDiv = theDiv.style; 
		theDiv.clip = 'rect(0px '+theWidth+'px '+theHeight+'px 0px)'; 
	}  

	var oPix = document.childNodes ? 'px' : 0;
	theDiv.width = theWidth + oPix; 
	theDiv.pixelWidth = theWidth; 
	theDiv.height = theHeight + oPix; 
	theDiv.pixelHeight = theHeight;  
	
	if( theDiv.resizeTo ) 
	{
		theDiv.resizeTo( theWidth, theHeight ); 
	}  
	
	theDiv.left = ( ( this.dir != 3 ) ? 0 : this.width - theWidth ) + oPix; 
	theDiv.top = ( ( this.dir != 4 ) ? 0 : this.height - theHeight ) + oPix;  
}  

function Conexant_bin_setColour( a )   
{
	//change all the different colour styles
	var theDiv = Conexant_bin_getRefToDivNest( this.id ); 
	if( theDiv.style ) { 
		theDiv = theDiv.style; 
	}  
	theDiv.bgColor = a; 
	theDiv.backgroundColor = a; 
	theDiv.background = a;  
}  

function Conexant_bin_show_div(Conexant_bin_show,Conexant_bin_id) {  
	if(Conexant_bin_show) {  
		document.getElementById(Conexant_bin_id).style.display  = "block";  
	} else {  
		document.getElementById(Conexant_bin_id).style.display  = "none";  
	}
}  

function Conexant_bin_progress()  
{
	if (Conexant_bin_time < 1)
	{
		Conexant_bin_time = Conexant_bin_time + 0.033;
	} else {
		Conexant_bin_time = 0;  
		Conexant_bin_loop_num++;  
	}  

	setTimeout('Conexant_bin_progress()',Conexant_bin_delay_time);
	Conexant_bin_myProgBar.setBar(Conexant_bin_time);  
}  

function Conexant_bin_sendClicked(F)  
{
	if(document.Conexant_bin_usb_upload.Conexant_bin_uploadedfile.value == "")
	{  
		document.Conexant_bin_usb_upload.Conexant_bin_uploadedfile.focus();  
		alert('File name can not be empty !');  
		return false;  
	}  
			
	F.submit();  
	Conexant_bin_show_div(true, "Conexant_bin_progress_div");  
	Conexant_bin_progress();
}

function Conexant_bin_init()	
{
	Conexant_bin_show_div(0, "Conexant_bin_progress_div");
}

function Conexant_bin_my_OK()
{
	setTimeout(function(){Conexant_bin_init()}, 0);
	setTimeout(function(){alert("upgrade succeed!")}, 0);
}

function Conexant_bin_my_Error(str)
{
	alert(str);
	Conexant_bin_init();
}  

function conexant_show_div(conexant_show,conexant_id) {  
	if(conexant_show) {  
		document.getElementById(conexant_id).style.display  = "block";  
	} else {  
		document.getElementById(conexant_id).style.display  = "none";  
	}
}

function conexant_init()	
{
	conexant_show_div(0, "conexant_progress_div");
} 

function Conexant_upgrade_my_OK()
{
	document.getElementById("xzx_module_Conexant_upgrade_view").innerHTML = "Conexant upgrade OK!";
	alert("upgrade succeed!");
}
 
function Conexant_upgrade()
{
	var xmlhttp;
	var scope, sound_card;
	var command_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_Conexant_upgrade_view").innerHTML = "Conexant upgrade ... ...";
				setTimeout(function(){Conexant_upgrade_my_OK()}, 45000);
			} else {
				document.getElementById("xzx_module_Conexant_upgrade_view").innerHTML = "Conexant upgrade err!";
			}
		} else {
			document.getElementById("xzx_module_Conexant_upgrade_view").innerHTML = "Conexant upgrade err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=UpdataConexant", true);
	xmlhttp.send();
}

function get_wakeup_test()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_app_display_view").innerHTML = " get wakeup times: " + response.times;
			} else {
				document.getElementById("xzx_module_app_display_view").innerHTML = "get wakeup times err!";
			}
		} else {
			document.getElementById("xzx_module_app_display_view").innerHTML = "get wakeup times err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=GetWakeupTimes", true);
	xmlhttp.send();
}

function enable_test_mode()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_test_mode_view").innerHTML = "enable test mode OK!";
			} else {
				document.getElementById("xzx_module_test_mode_view").innerHTML = "enable test mode err!";
			}
		} else {
			document.getElementById("xzx_module_test_mode_view").innerHTML = "enable test mode err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=SetTestMode:1", true);
	xmlhttp.send();
}

function disable_test_mode()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_test_mode_view").innerHTML = "disable test mode OK!";
			} else {
				document.getElementById("xzx_module_test_mode_view").innerHTML = "disable test mode err!";
			}
		} else {
			document.getElementById("xzx_module_test_mode_view").innerHTML = "disable test mode err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=SetTestMode:0", true);
	xmlhttp.send();
}

function get_test_mode()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_test_mode_view").innerHTML = "get test mode: " + response.test;
			} else {
				document.getElementById("xzx_module_test_mode_view").innerHTML = "get test mode err!";
			}
		} else {
			document.getElementById("xzx_module_test_mode_view").innerHTML = "get test mode err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=GetTestMode", true);
	xmlhttp.send();
}

function play_test_music()
{
	var xmlhttp;
	var url_val;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_test_mode_view").innerHTML = "play test music ok!";
			} else {
				document.getElementById("xzx_module_test_mode_view").innerHTML = "play test music err!";
			}
		} else {
			document.getElementById("xzx_module_test_mode_view").innerHTML = "play test music err!";
		}
	}
	
	url_val = "/cgi-bin/httpapi.cgi?command=PlayTestMusic:" + document.getElementById("xzx_module_test_mode_url").value;
	
	xmlhttp.open("GET", url_val, true);
	xmlhttp.send();
}

function stop_test_music()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				document.getElementById("xzx_module_test_mode_view").innerHTML = "stop test music ok!";
			} else {
				document.getElementById("xzx_module_test_mode_view").innerHTML = "stop test music err!";
			}
		} else {
			document.getElementById("xzx_module_test_mode_view").innerHTML = "stop test music err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=StopTestMusic", true);
	xmlhttp.send();
}

var cloud_test_log_status = 0;
var cloud_test_log_url;
function enable_cloud_test_log()
{
	var xmlhttp;
	var download_url;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				cloud_test_log_status = 1;
				cloud_test_log_url = "http://" + window.location.host + "/" +response.url;
				document.getElementById("xzx_module_test_log_view").innerHTML = "enable cloud test log ok! get url:" + cloud_test_log_url;
			} else {
				document.getElementById("xzx_module_test_log_view").innerHTML = "enable cloud test log err!";
			}
		} else {
			document.getElementById("xzx_module_test_log_view").innerHTML = "enable cloud test log err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=LogCaptureEnable", true);
	xmlhttp.send();
}

function disable_cloud_test_log()
{
	var xmlhttp;
	
	if (window.XMLHttpRequest) {
		xmlhttp = new XMLHttpRequest();
	} else {
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			var response = JSON.parse(xmlhttp.responseText);
			if (response.ret == "OK")
			{
				cloud_test_log_status = 0;
				document.getElementById("xzx_module_test_log_view").innerHTML = "disable cloud test log ok!";
			} else {
				document.getElementById("xzx_module_test_log_view").innerHTML = "disable cloud test log err!";
			}
		} else {
			document.getElementById("xzx_module_test_log_view").innerHTML = "disable cloud test log err!";
		}
	}
	
	xmlhttp.open("GET", "/cgi-bin/httpapi.cgi?command=LogCaptureDisable", true);
	xmlhttp.send();
}

function get_cloud_test_log()
{
	
	if (cloud_test_log_status == 1)
	{
		document.getElementById("xzx_module_test_log_view").innerHTML = "download file: " + cloud_test_log_url;
		download_file3(cloud_test_log_url);
	} else {
		alert('disable cloud test log!');  
	}
}
