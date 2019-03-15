/*
	Common display/input validation routines
*/

function addOptions(s, v) {
	for (var i = 0; i < v.length; i++) {
		var op = document.createElement("option");
		op.text = op.value = v[i];
		s.add(op);
	}
}

function chkDate(month, day) {
	var d = new Date();
	var year = d.getFullYear();

	if (month == 2) {
		if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0))) //leap year
		{
			if ((day < 1) || (day > 29)) return 0;
		} else {
			if ((day < 1) || (day > 28)) return 0;
		}
	} else if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
		if ((day < 1) || (day > 31)) return 0;
	} else {
		if ((day < 1) || (day > 30)) return 0;
	}

	return 1;
}

function chkEmailAdd(addr, m) {
	var re = /^([\w]+)(.[\w]+)*@([\w]+)(.[\w]{2,3}){1,2}$/;
	if (re.test(addr)) return 1;
	if (m) alert(wdk.msg(m) + wdk.msg(" is invalid !"));
	return 0;
}

function chkHex(h, w) {
	var illegal = /[^a-fA-F0-9]/;
	if (illegal.test(h.value)) {
		alert(wdk.msg(w) + wdk.msg(" contains invalid Hex digits!     ") + wdk.msg("(The hex digits are 0-9 and A-F)"));
		return false;
	}
	return true;
}

function chkHexNum(v, a, b, s) {
	if (!chkHex(v)) {
		alert(wdk.msg(s) + wdk.msg(" isn't a hex-digit number !"));
		v.value = v.defaultValue;
		return 0;
	}
	if (v.value != '') {
		if ((v.value < a.toString(16)) || (v.value > b.toString(16))) {
			alert(wdk.msg(s) + wdk.msg(" is between ") + a.toString(16) + wdk.msg(" and ") + b.toString(16));
			v.value = v.defaultValue;
			return 0;
		}
	} else {
		alert(wdk.msg(s) + wdk.msg(" must input a value !"));
		return 0;
	}
	return 1;
}

function chkIP(ipa, m, subnet) {
	var re = /^(\d{1,3}\.){3}\d{1,3}$/;
	if (re.test(ipa.value)) {
		var s = ipa.value.split('.');
		for (var i = 0; i < 4; i++) {
			var n = parseInt(s[i]);
			if (n < 256) {
				if (subnet) continue;
				if ((i == 1 || i == 2) && n == 0) continue;
				if (n != 255 && n != 0) continue;
			}
			if (m) alert(wdk.msg(m) + wdk.msg(" is invalid !"));
			return 0;
		}
		return 1;
	}
	if (m) alert(wdk.msg(m) + wdk.msg(" is invalid !"));
	return 0;
}

function chkIpOrDomain(ipa, m) {
	var re = /^(\d{1,3}\.){3}\d{1,3}$/;
	// ip
	if (re.test(ipa.value))
		return chkIP(ipa, m, 0);
	// domain in string
	//var t = /[~!@#$\%\^&*\(\)\_+-=|\\\]\[\}\{\"\'\;\:\?\/\>\<\,\`]{1,}/;
	//if (t.test(ipa.value)) { if (m) alert(wdk.msg(m)+wdk.msg(" is invalid !")); return 0; }
	var specialChars = "\\(\\)><@,;:\\\\\\\"\\.\\[\\]#!^\\$=";
	var validChars = "\[^\\s" + specialChars + "\]";
	var atom = validChars + '+';
	var domainPat = new RegExp("^" + atom + "(\\." + atom + ")*$");
	if (!domainPat.test(ipa.value)) {
		if (m) alert(wdk.msg(m) + wdk.msg(" is invalid !"));
		return 0;
	}

	return 1;
}

function chkIpOrNull(ipa, m) {
	var ip = getIP(ipa);
	if (ip == '' || ip == '0.0.0.0') return true;
	return chkIP(ipa, m);
}

function chkMAC(ma, s, sp) {
	var t = /[0-9a-fA-F]{2}/;
	var m = new Array();

	if (ma.value == '') {
		alert(wdk.msg(s) + wdk.msg(" must input a value !"));
		return 0;
	}
	if (ma.length == 6) {
		for (var i = 0; i < 6; i++)
			m[i] = ma[i].value;
	} else
		m = ma.value.split(":");

	//if(m.length != 6)  { alert(wdk.msg(s)+wdk.msg(" is invalid !"));  return 0; }
	if (m.length != 6) {
		alert(wdk.msg(s) + wdk.msg(" format error"));
		return 0;
	}
	if (sp) {
		if (m.toString() == ',,,,,') return 1;
	}
	for (var i = 0; i < 6; i++) {
		if (!t.test(m[i])) {
			alert(wdk.msg(s) + wdk.msg(" is invalid !"));
			return 0;
		}
	}
	return 1;
}

function chkMsk(mn, str) {
	var m = [];
	if (mn.length == 4)
		for (i = 0; i < 4; i++) m[i] = mn[i].value;
	else {
		m = mn.value.split('.');
		if (m.length != 4) {
			alert(wdk.msg(str) + wdk.msg(" is invalid !"));
			return 0;
		}
	}

	var t = /[^0-9]{1,}/;
	for (var i = 0; i < 4; i++) {
		if (t.test(m[i]) || m[i] > 255) {
			alert(str + wdk.msg(" is invalid !"));
			return 0;
		}
	}

	var v = (m[0] << 24) | (m[1] << 16) | (m[2] << 8) | (m[3]);
	var f = 0;
	for (var k = 0; k < 32; k++) {
		if ((v >> k) & 1) f = 1;
		else if (f == 1) {
			alert(str + wdk.msg(" is invalid !"));
			//for(var i=0; i<4; i++) m[i].value=m[i].defaultValue;
			return 0;
		}
	}
	if (f == 0) {
		alert(str + wdk.msg(" is invalid !"));
		return 0;
	}
	return 1;
}

function chkNum(v, a, b, s) {
	if (isNaN(v.value) || isNaN(parseInt(v.value))) {
		alert(wdk.msg(s) + wdk.msg(" isn't a number !"));
		v.value = v.defaultValue;
		return 0;
	}
	if (v.value != '') {
		if ((v.value < a) || (v.value > b)) {
			alert(wdk.msg(s) + wdk.msg(" is out of range !"));
			v.value = v.defaultValue;
			return 0;
		}
	} else {
		alert(wdk.msg(s) + wdk.msg(" must input a value !"));
		return 0;
	}
	return 1;
}

function chkNumOrNull(v, s) {
	if (isNaN(v.value)) {
		alert(wdk.msg(s) + wdk.msg(" isn't a number !"));
		return 0;
	}
	if (v.value == '') {
		alert(wdk.msg(s) + wdk.msg(" must input a value !"));
		return 0;
	}
	if (Number(v.value) < 0) {
		alert(wdk.msg(s) + wdk.msg(" value must NOT less than 0 !"));
		return 0;
	}
	return 1;
}

function chkPass(p, pv, c, m) {
	if (c.value == '0') return 1;
	// modified
	if (!chkPassIsSame(p.value, pv.value, m)) return 0;
	if (!confirm(wdk.msg('Change password ?'))) {
		c.value = 0;
		p.value = pv.value = p.defaultValue;
		return 0;
	}
	return 1;
}

function chkPassInput(p, pv, c) {
	if (c.value == '0') {
		p.value = pv.value = ""; // reset to null;
		c.value = '1';
	}
}

function chkPassInput2(po, p, pv, c) {
	if (c.value == '0') {
		po.value = p.value = pv.value = ""; // reset to null;
		c.value = '1';
	}
}

function chkPassIsSame(p1, p2, m) {
	//if (p1 != p2) { alert("Password is not consistent!") ; return 0 ; }
	if (p1 != p2) {
		if (m) alert(wdk.msg(m));
		return 0;
	} else return 1;
}

function chkPortRange(v, m) {
	var t = /[^0-9,\s-]{1,}/;
	if ((v.value.length == 0) || t.test(v.value)) {
		alert(wdk.msg(m) + wdk.msg(" has illegal value or input format is incorrect !"));
		return 0;
	}
	var n = v.value.split(",");

	for (var i = 0; i < n.length; i++) {
		if (n[i] == '') break;

		if (!chkPortRange1(n[i], m)) return 0;
		else
		if ((n[i] < 1) || (n[i] > 65535)) {
			alert(wdk.msg(m) + wdk.msg(" is out of range !"));
			return 0;
		}
	}
	return 1;
}

function chkPortRange1(v, m) {
	var r = /-/;
	if (r.test(v)) {
		var s = v.split("-");
		for (var j = 0; j < s.length; j++) {
			if ((s[j] < 1) || (s[j] > 65535)) {
				alert(wdk.msg(m) + wdk.msg(" is out of range !"));
				return 0;
			}
		}
	}
	return 1;
}

function chkPreKey(s, w) {
	var illeagalPatt;
	if (s.value.length >= 64) {
		illeagalPatt = /[^0-9a-fA-F]/;
		if (illeagalPatt.test(s.value)) {
			alert(wdk.msg(w) + wdk.msg(" requires 64 hex digits or ASCII code between 8 and 63."));
			return 0;
		}
	} else if (s.value.length > 63 || s.value.length < 8) {
		alert(wdk.msg(w) + wdk.msg(" requires 64 hex digits or ASCII code between 8 and 63."));
		return 0;
	} else {
		illeagalPatt = /[^\x20-\x7E]/;
		if (illeagalPatt.test(s.value)) {
			alert(wdk.msg(w) + wdk.msg(" contains invalid characters!"));
			return 0;
		}
	}
	return 1;
}

function chkStr(s, w, r) {
	var illegal = /[^a-zA-Z0-9#_\.@-]/; //non letter && numbers && '#' && '_' && '.' && '@' && '-'
	if (r) illegal = r; //self-defined
	if (illegal.test(s.value)) {
		alert(wdk.msg(w) + wdk.msg(" contains invalid characters!"));
		return 0;
	}
	return 1;
}

function chkStrLen(s, m, M, w) {
	var str = s.value;
	if (str.length < m || str.length > M) {
		alert(wdk.msg(w) + wdk.msg(" length is between ") + m + wdk.msg(" and ") + M + wdk.msg(" characters"));
		return 0;
	}
	return 1;
}

function chkStrNoNull(s, w, r) {
	var illegal = /[^a-zA-Z0-9#_\.@-]/; //non letter && numbers && '#' && '_' && '.' && '@' && '-'
	if (r) illegal = r; //self-defined
	if (!s.value) {
		alert(wdk.msg(w) + wdk.msg(" must input a value !"));
		return 0;
	}
	if (illegal.test(s.value)) {
		alert(wdk.msg(w) + wdk.msg(" contains invalid characters!"));
		return 0;
	}
	return 1;
}

/**
 * check substring exist or not
 * @method chkSubStr
 * @param {string} all - the string want to search
 * @param {string} str - substring (e.g. IP or MAC address)
 * @param {boolean} ingore - ingore case or not
 * @return {boolean} true or false
 */

function chkSubStr(all, str, ingore) {
	var patt = new RegExp(str, ingore ? "i" : "");
	var n = all.search(patt);
	if (n > -1)
		return true;
	return false;
}

function chkText(s, w) {
	var illegal1 = /[\;& ]/;
	if (illegal1.test(s.value)) {
		alert(wdk.msg(w) + wdk.msg(" contains invalid characters \;& and space!"));
		return 0;
	}
	var illegal2 = /[\u4e00-\u9fa5]/; //Chinese word
	if (illegal2.test(s.value)) {
		alert(wdk.msg("Please input English word!"));
		return 0;
	}

	return 1;
}

function chkUrl(URLstr, m) {
	var t = /[~!@#$\%\^\&\*\(\)]{1,}/;

	if (URLstr.value == '') {
		alert(wdk.msg(m) + wdk.msg(" must input !"));
		return 0;
	}
	if (t.test(URLstr.value)) {
		if (m) alert(wdk.msg(m) + wdk.msg(" is invalid !"));
		return 0;
	}

	return 1;
}

function isBlank(v) {
	var s = v.value;
	var i = 0;
	for (i = 0; i < s.length; i++) {
		if (s.charCodeAt(i) != 32) break;
	}
	if (i == s.length) {
		v.focus();
		return true;
	}
	return false;
}

function isHN(str) {
	if (str.length == 0) return false;
	for (var i = 0; i < str.length; i++) {
		if (str.charAt(i) <= '9' && str.charAt(i) >= '0') continue;
		if (str.charAt(i) <= 'F' && str.charAt(i) >= 'A') continue;
		if (str.charAt(i) <= 'f' && str.charAt(i) >= 'a') continue;
		return false;
	}
	return true;
}

function isIE() {
	var agt = navigator.userAgent.toLowerCase();
	return (agt.indexOf("msie") != -1); // ie
}

function decomList(str, len, idx, dot) {
	var t = str.split(dot);
	return t[idx];
}

function decomListLen(str, dot) {
	var t = str.split(dot);
	return t.length;
}

function setMAC(ma, macs, nodef) {
	var re = /^[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}$/;
	if (!(re.test(macs) || macs != '')) return 0;
	if (ma.length != 6) {
		ma.value = macs;
		return 1;
	}
	if (macs != '')
		var d = macs.split(":");
	else
		var d = ['', '', '', '', '', ''];

	for (var i = 0; i < 6; i++) {
		ma[i].value = d[i];
		if (!nodef) ma[i].defaultValue = d[i];
	}
	return 1;
}

function setSelectValue(id, v) {
	var s = wdk.getById(id);
	for (var k = 0; k < s.options.length; k++) {
		s.options[k].selected = (s.options[k].value == v);
	}
}

function setIP(ipa, ips, nodef) {
	var re = /^(\d{1,3}\.){3}(\d{1,3})$/;
	if (re.test(ips)) {
		var d = ips.split(".");
		if (!ipa.length)
			ipa.value = ips;
		else
			for (i = 0; i < 4; i++) {
				ipa[i].value = d[i];
				if (!nodef) ipa[i].defaultValue = d[i];
			}
		return 1;
	}
	return 0;
}

function getBrowserType() {
	var BrowserType;
	var agt = navigator.userAgent.toLowerCase();

	if (agt.indexOf("msie") != -1) BrowserType = "IE";
	else if (agt.indexOf("navigator") != -1 || agt.indexOf("netscape") != -1) BrowserType = "Netscape";
	else if (agt.indexOf("firefox") != -1) BrowserType = "Firefox";
	else if (agt.indexOf("chrome") != -1) BrowserType = "Chrome";
	else if (agt.indexOf("safari") != -1) BrowserType = "Safari";
	else if (agt.indexOf("opera") != -1) BrowserType = "Opera";
	else if (agt.indexOf("camino") != -1) BrowserType = "Camino";
	else if (agt.indexOf("konqueror") != -1) BrowserType = "Konqueror";
	else BrowserType = "Other";

	return BrowserType;
}

function getDateStr() {
	var day = new Date();
	var m = day.getMonth() + 1;
	var d = day.getDate();
	if (m < 10)
		m = "0" + m;
	if (d < 10)
		d = "0" + d;
	var result = String(m) + String(d)
	return result;
}

function getIP(d) {
	if (d.length != 4) return d.value;
	var ip = d[0].value + "." + d[1].value + "." + d[2].value + "." + d[3].value;
	if (ip == "...")
		ip = "";
	return ip;
}

function getMAC(m) {
	if (m.length != 6) return m.value;
	var mac = m[0].value + ":" + m[1].value + ":" + m[2].value + ":" + m[3].value + ":" + m[4].value + ":" + m[5].value;
	mac = mac.toUpperCase();
	if (mac == ":::::")
		mac = "";
	return mac;
}

function getSelectValue(id) {
	var s = wdk.getById(id);
	if (s.options.length == 0)
		return "";
	else
		return s.options[s.selectedIndex].value;
}

function getStyle(objId) {
	var obj = document.getElementById(objId);
	if (obj) return obj.style;
	else return 0;
}

function twoDig(n) {
	return String(n + 100).substr(1, 2);
}

function timeStr(t) {
	if (t < 0) return '00:00:00';
	var s = t % 60;
	var m = parseInt(t / 60) % 60;
	var h = parseInt(t / 3600) % 24;
	var d = parseInt(t / 86400);
	if (d > 999) {
		return 'Forever';
	}

	var m = (d ? (d + ' days ') : '') + twoDig(h) + ':' + twoDig(m) + ':' + twoDig(s);
	return m;
}


function setVisibility(id, v) {
	var st = getStyle(id);
	if (st) {
		st.visibility = v;
		return true;
	} else return false;
}

function setDisplay(id, v) {
	var st = getStyle(id);
	if (st) {
		st.display = v;
		return true;
	} else return false;
}

function showHide(id, en) {
	var bl = wdk.getById(id);
	bl.style.display = ((en != 0) ? 'block' : 'none');
}

function showHideMultiple(idx, id, en) {
	var v = id.split('&');
	var e = en.split('&');
	for (var i = 0; i < idx; i++) {
		if (e[i] == 'true')
			showHide(v[i], 1);
		else
			showHide(v[i], 0);
	}
}

function showHideFuncMode(id, en) {
	var bl = wdk.getById(id);
	bl.style.display = ((en != 1) ? 'block' : 'none');
}

function rule2var(str, name) {
	var it = str.split('&');
	var re = new RegExp(name + "=(.*)");
	for (var i = 0; i < it.length; i++) {
		if (re.test(it[i])) {
			var v1 = (it[i]).split('=');
			if (v1[0] == name)
				return RegExp.$1;
		}
	}
	return '';
}

function updaterulevar(str, name, value) {
	var it = str.split('&');
	var re = new RegExp(name + "=(.*)");
	for (var i = 0; i < it.length; i++) {
		if (re.test(it[i])) {
			it[i] = name + '=' + value;
			break;
		}
	}
	return it.join('&');
}

function showHidden(len) {
	var s = "";
	for (i = 0; i < len; i++)
		s = s + "*";
	return s;
}

function refreshPage() {
	window.location = wdk.getThisPage();
}

function goUrl(url) {
	window.location.href = url;
}

function subNet(n) {
	return n.replace(/\.\d+$/, ".");
}

/*-----------------------------------------------------------------
	select row
-----------------------------------------------------------------*/
var _old_entry = 0;
var _old_color = 0;

function markEntry(ne) {
	if (_old_entry != ne) {
		if (_old_entry)
			_old_entry.style.backgroundColor = _old_color;
		_old_entry = ne;
		_old_color = ne.style.backgroundColor;
		ne.style.backgroundColor = "#FFFF00";
	}
}

function unmarkEntry(e) {
	if (e)
		e.style.backgroundColor = _old_color;
	else
	if (_old_entry)
		_old_entry.style.backgroundColor = _old_color;
	_old_entry = 0;
}
/*-----------------------------------------------------------------
	form
-----------------------------------------------------------------*/
function cpyEntry(id, num) {
	var row = wdk.getById(id);
	var tb = row.parentNode;
	var name = id.replace(/\d+$/, '');
	var k = Number(id.match(/\d+$/));
	for (var i = 1; i <= num; i++) {
		var n = row.cloneNode(true);
		n.id = name + (k + i);
		tb.appendChild(n);
	}
}

function delEntry(id, num) {
	var name = id.replace(/\d+$/, '');
	var k = Number(id.match(/\d+$/));
	for (; num > 0; num--) {
		var e = wdk.getById(name + (num + k));
		if (e) {
			e.parentNode.removeChild(e);
		}
	}
}

function refreshList(id, max, num) {
	var one = id + '1';
	unmarkEntry(0);
	delEntry(one, max - 1);
	setVisibility(one, (num == 0) ? "hidden" : "visible");
	cpyEntry(one, num - 1);
}

function ip_lsb(v, ip) {
	v.defaultValue = v.value = ip.match(/\d+$/);
}

function cloneMac(en, mac) {
	var cln_MAC = wdk.cliCmd('http peermac');

	if (!en.checked) {
		alert(wdk.msg("Clone MAC Address is disabled !"));
		return;
	}
	setMAC(mac, cln_MAC, 1);
}

function ByMask_IPrange(IP, MASK, SoE) {
	var IP_AMOUNT_MAX = 256;
	var ip = IP.split(".");
	var mask = MASK.split(".");
	var i;

	for (i = 0; i < ip.length; i++) {
		/* SoE=0:show this Network IP ; SoE=1:show this Network Broadcast IP */
		if ((ip[i] < 0) || (ip[i] > 255)) {
			alert("Invalid IP!");
			return false;
		}
		if (SoE == 1) {
			mask[i] = ~mask[i] + IP_AMOUNT_MAX;
			ip[i] = ip[i] | mask[i];
		} else
			ip[i] = ip[i] & mask[i];
	}
	return ip.join(".");
}

function BinaryChk_IP(IP, MASK, GW) {
	var NetworkIP = ByMask_IPrange(IP, MASK, 0);
	if (!NetworkIP) return false;
	var BroadcastIP = ByMask_IPrange(IP, MASK, 1);
	if (!BroadcastIP) return false;
	if (NetworkIP == IP) {
		alert("Attention:IP value should not be Network IP.\nNetwork IP : " + NetworkIP + "\nBroadcast IP : " + BroadcastIP);
		return false;
	}
	if (BroadcastIP == IP) {
		alert("Attention:IP value should not be Broadcast IP.\nNetwork IP : " + NetworkIP + "\nBroadcast IP : " + BroadcastIP);
		return false;
	}
	if (NetworkIP == GW) {
		alert("Attention:Gateway value should not be Network IP.\nNetwork IP : " + NetworkIP + "\nBroadcast IP : " + BroadcastIP);
		return false;
	}
	if (BroadcastIP == GW) {
		alert("Attention:Gateway value should not be Broadcast IP.\nNetwork IP : " + NetworkIP + "\nBroadcast IP : " + BroadcastIP);
		return false;
	}
	if (NetworkIP != ByMask_IPrange(GW, MASK, 0)) {
		alert("Attention:IP and Gateway are not in the same subnet!!");
		return false;
	}
	return true;
}

function substr_count(haystack, needle, offset, length) {
	var cnt = 0;

	haystack += '';
	needle += '';
	if (isNaN(offset)) {
		offset = 0;
	}
	if (isNaN(length)) {
		length = 0;
	}
	offset--;

	while ((offset = haystack.indexOf(needle, offset + 1)) != -1) {
		if (length > 0 && (offset + needle.length) > length)
			return false;
		else
			cnt++;
	}
	return cnt;
}

// test_ipv4
//
// Test for a valid dotted IPv4 address
//
function test_ipv4(ip) {
	var match = ip.match(/(([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|255[0-5])\.){3}([1-9]?[0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])/);
	return match != null;
}

// test_ipv6
//
// Test if the input is a valid ipv6 address.
//
function test_ipv6(ip) {
	// Test for empty address
	if (ip.length < 3) {
		return ip == "::";
	}

	// Check if part is in IPv4 format
	if (ip.indexOf('.') > 0) {
		lastcolon = ip.lastIndexOf(':');
		if (!(lastcolon && test_ipv4(ip.substr(lastcolon + 1))))
			return false;
		// replace IPv4 part with dummy
		ip = ip.substr(0, lastcolon) + ':0:0';
	}

	// Check uncompressed
	if (ip.indexOf('::') < 0) {
		var match = ip.match(/^(?:[a-f0-9]{1,4}:){7}[a-f0-9]{1,4}$/i);
		return match != null;
	}

	// Check colon-count for compressed format
	if (substr_count(ip, ':')) {
		var match = ip.match(/^(?::|(?:[a-f0-9]{1,4}:)+):(?:(?:[a-f0-9]{1,4}:)*[a-f0-9]{1,4})?$/i);
		return match != null;
	}

	// Not a valid IPv6 address
	return false;
}

/**
 * A wrapper function of document.write()
 * @method puts
 * @param {string} s - a string would be written into HTML file
 * @return {undefined}
 */
function puts(s) {
	document.write(s);
}

function range(min, max, step) {
	var matrix = [];
	var inival, endval, plus;
	var walker = step || 1;

	if (!isNaN(min) && !isNaN(max)) {
		inival = min;
		endval = max;
	} else {
		inival = (isNaN(min) ? 0 : min);
		endval = (isNaN(max) ? 0 : max);
	}

	plus = ((inival > endval) ? false : true);
	if (plus) {
		while (inival <= endval) {
			matrix.push(inival);
			inival += walker;
		}
	} else {
		while (inival >= endval) {
			matrix.push(inival);
			inival -= walker;
		}
	}

	return matrix;
}
