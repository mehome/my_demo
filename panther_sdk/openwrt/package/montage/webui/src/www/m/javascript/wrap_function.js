function dataObject() {
	this.get_config_obj = function(param) {

		if (param == null) {
			return;
		}
		wdk.cdbLoad(param);
	}

	this.config_val = function(param) {
		return wdk.cdbVal(param);
	}

	this.config_set = function(which_value, obj) {
		wdk.cdbSet(obj, which_value);
	}
}

function show_words(word) {
	try {
		document.write(word);
	} catch (e) {
		console.log(e);
	}
}

function get_words(word) {
	document.write(wdk.msg(word));
}

function get_by_id(id) {
	return wdk.getById(id);
}

function time_format_trans(time) {
	return timeStr(time);
}

function back(url) {
	window.location.href = url;
}

function set_checked(which_value, obj) {
	if (obj.length > 1) {
		obj[0].checked = true;
		for (var pp = 0; pp < obj.length; pp++) {
			if (obj[pp].value == which_value) {
				obj[pp].checked = true;
			}
		}
	} else {
		obj.checked = false;
		if (obj.value == which_value) {
			obj.checked = true;
		}
	}
}

function get_checked_value(obj) {
	if (obj == null) {
		alert('obj is null');
		return;
	}
	if (obj.length > 1) {
		for (pp = 0; pp < obj.length; pp++) {
			if (obj[pp].checked) {
				return obj[pp].value;
			}
		}
	} else {
		if (obj.checked) {
			return obj.value;
		} else {
			return 0;
		}
	}
}

function set_selectIndex(which_value, obj) {
	for (var pp = 0; pp < obj.options.length; pp++) {
		if (which_value == obj.options[pp].value) {
			obj.selectedIndex = pp;
			break;
		}
	}
}

function addr_obj(addr, e_msg, allow_zero, is_network, is_udp) {
	this.addr = addr;
	this.e_msg = e_msg;
	this.allow_zero = allow_zero;
	this.is_network = is_network;
	this.is_udp = is_udp || false;
}

function ip_num(IP_array) {
	var total1 = 0;
	if (IP_array.length > 1) {
		total1 += parseInt(IP_array[3], 10);
		total1 += parseInt(IP_array[2], 10) * 256;
		total1 += parseInt(IP_array[1], 10) * 256 * 256;
		total1 += parseInt(IP_array[0], 10) * 256 * 256 * 256;
	}
	return total1;
}

function check_LAN_ip(LAN_IP, CHK_IP) {
	if (ip_num(LAN_IP) == ip_num(CHK_IP)) {
		return false;
	}
	return true;
}

function check_ip_order(start_ip, end_ip) {
	var temp_start_ip = start_ip.addr;
	var temp_end_ip = end_ip.addr;
	var total1 = ip_num(temp_start_ip);
	var total2 = ip_num(temp_end_ip);

	if (total1 > total2)
		return false;
	return true;
}

function check_lanip_order(ip, start_ip, end_ip) {
	var temp_start_ip = start_ip.addr;
	var temp_end_ip = end_ip.addr;
	var temp_ip = ip.addr;
	var total1 = ip_num(temp_start_ip);
	var total2 = ip_num(temp_end_ip);
	var total3 = ip_num(temp_ip);
	if (total1 <= total3 && total3 <= total2)
		return true;
	return false;
}


function genOpt(optContent) {
	if (optContent == null) {
		console.log("Parsing argument error!");
		return;
	}

	var optList = [];
	for (var i = 0; i < optContent.length; i++) {
		var opt = document.createElement('option');
		opt.value = i;
		opt.innerHTML = optContent[i];
		optList.push(opt);
	}
	return optList;
}


var highlightedArr = new Array();

function highlight(id) {
	if (id == null) return;
	var highlightColor = "rgb(255,255,0)";

	//Clear all previous highlighted elements.
	for (var j in highlightedArr) {
		document.getElementById(highlightedArr[j]).removeAttribute("style");
	}

	highlightedArr = [];
	highlightedArr.push(id);

	if (id != null) {
		var target = document.getElementById(id);
		target.style.backgroundColor = highlightColor;
		target.focus();
	}
}

window.onload = function() {
	var btn = document.createElement('input');
	btn.setAttribute('type', 'button');
	btn.setAttribute('value', 'Desktop Web');
	btn.className = 'changeToDskWeb';
	btn.onclick = function() {
		window.location = window.location.origin + "/status.htm";
	};
	document.body.appendChild(btn);
}
