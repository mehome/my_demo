wdk.cdbLoad([
	'$wl_ap_isolated',
	'$wl_bb_mode',
	'$wl_bss_isolated',
	'$wl_bss_bssid1-' + SSID_MAX_NUM,
	'$wl_bss_cipher1-' + SSID_MAX_NUM,
	'$wl_bss_enable1-' + SSID_MAX_NUM,
	'$wl_bss_key_mgt1-' + SSID_MAX_NUM,
	'$wl_bss_role1-' + SSID_MAX_NUM
]);
wdk.cdbLoad([
	'$wl_bss_num',
	'$wl_region',
	'$wl_bss_sec_type1-' + SSID_MAX_NUM,
	'$wl_bss_ssid1-' + SSID_MAX_NUM,
	'$wl_bss_ssid_hidden1-' + SSID_MAX_NUM,
	'$wl_bss_wep_1key1-' + SSID_MAX_NUM,
	'$wl_bss_wep_2key1-' + SSID_MAX_NUM
]);
wdk.cdbLoad([
	'$wl_channel',
	'$wl_enable',
	'$wl_bss_wep_3key1-' + SSID_MAX_NUM,
	'$wl_bss_wep_4key1-' + SSID_MAX_NUM,
	'$wl_bss_wep_index1-' + SSID_MAX_NUM,
	'$wl_bss_wpa_psk1-' + SSID_MAX_NUM,
	'$wl_bss_wps_state1-' + SSID_MAX_NUM
]);
var work_mode = wdk.cdbVal('$op_work_mode');
var SSID_NUM = 0;

//CIPHER MODE
var WAPI_CIPHER_MODE = 16;
var AES_CIPHER_MODE = 8;
var TKIP_CIPHER_MODE = 4;
var WEP104_CIPHER_MODE = 2;
var WEP40_CIPHER_MODE = 1;
var NONE_CIPHER_MODE = 0;

//SECURITY MODE
var DISABLE_SEC_MODE = 0;
var OPEN_SEC_MODE = 1;
var SHARED_SEC_MODE = 2;
var WEP_SEC_MODE = 4;
var WEP_CIPHER_PLUS_SEC = 5;
var WPA_SEC_MODE = 8;
var WPA2_SEC_MODE = 16;
var WPA_WPA2_SEC_MODE = 24;
var WPS_SEC_MODE = 32;
var WAPI_SEC_MODE = 128;

// WIFI ROLE
var WIFI_NONE = 0;
var WIFI_STA_ROLE = 1;
var WIFI_AP_ROLE = 2;
var WIFI_MAT = 256;

function init_working_mode(checked) {
	SSID_NUM = 0;
	showHide('wl_wbs1_hiden', 0);
	showHide('wl_wbs2_hiden', 0);
	showHide('wl_opt', 0);
	showHide('wl_setting', 0);
	if (checked) {
		SSID_NUM = 1;
		switch (work_mode) {
			case '1':
			case '2':
			case '6':
				break;
			case '3':
			case '9':
			case '4':
			case '5':
				SSID_NUM = 2;
				break;
		}
		if ((work_mode == 4) || (work_mode == 6))
			showHide('wl_wbs2_hiden', 1);
		else {
			if (work_mode == 3 || work_mode == 5 || work_mode == 9)
				showHide('wl_wbs2_hiden', 1);
			showHide('wl_wbs1_hiden', 1);
		}
		showHide('wl_opt', 1);
		showHide('wl_setting', 1);
	}
}

function display_working_mode() {
	wdk.putmsg("Wireless Setting");
	puts(" - ");
	switch (work_mode) {
		case '1':
			wdk.putmsg("AP mode");
			break;
		case '2':
			wdk.putmsg("Wireless Router mode");
			break;
		case '3':
			wdk.putmsg("WISP mode");
			break;
		case '4':
			wdk.putmsg("Repeater mode");
			break;
		case '5':
			wdk.putmsg("Bridge mode");
			break;
	}
}

var apList;
var ScanWaitTime = 3;
var ScanTime = 0;
var IntervalID = 0;

function buttonDisable(id, val) {
	wdk.getById(id).disabled = val;
}

function ScanMsgDisp() {
	var s;

	s = ScanWaitTime - ScanTime;

	if (s != 0) {
		wdk.getById('ScanMsg').innerHTML = 'Please wait ' + s + (s > 1 ? ' seconds' : ' second');
		ScanTime++;
	} else {
		clearInterval(IntervalID);
		buttonDisable('survey', false);
		wdk.getById('ScanMsg').innerHTML = '';
		wdk.getById('ScanMsg').style.display = "none";
		ScanTime = 0;
		ScanResult();
	}
}

function CancelScanResult() {
	wdk.getById('aplist').style.display = "none";
}

function APSelected(idx) {
	var m = apList[idx - 1].split('&');

	if (m[1].search("ssid=") != -1) {
		m[1] = apList[idx - 1].substring(apList[idx - 1].indexOf("&ssid=") + 1, apList[idx - 1].indexOf("&ch="));
	}

	if (m.length && m.length > 1) {
		for (var j = 0; j < m.length; j++) {
			var mm = m[j].split('=');
            if (j == 1)
                mm[1] = m[j].substring(5);
			if (mm[0] == "bssid") {
				var e = wdk.getById('$wl_bss_bssid2');
				e.value = mm[1];
			}
			if (mm[0] == "ssid") {
				var e = wdk.getById('$wl_bss_ssid2');
				e.value = mm[1];
			}
		}
	}
	var selectedAPChannel = m[2].split('=')[1];
	document.getElementById('$wl_channel').value = selectedAPChannel;

	//The caller function should be added here.
	var securityData = m[5].split('=');
	var securitySetting = securityData[1];
	securitySetting = convertToCdbSetting(m);
	var cipherMode = m[6].split("=")[1] >> 1;
	sec_view_disp(securitySetting, cipherMode);
}

function convertToCdbSetting(m) {
	var securityData = m[5].split('=');
	var securitySetting = securityData[1];
	securitySetting = securitySetting << 2;
	return securitySetting;
}

function ScanResult() {
	var info = wdk.cliCmd('ap result');
	var fake = "bssid=&ssid=&ch=&sig=&band=&sec=&pcipher=&gcipher=";
	var max = 255;
	var total;

	refreshList('entry', max, 1);
	wdk.fill_form_entry(wdk.getById('entry1'), fake + '&id=1');

	apList = info.split("\n");
	apList.pop();
	total = (apList == '') ? 0 : apList.length;
	refreshList('entry', max, total);

	for (var i = 1; i <= total; i++) {
		var str = apList[i - 1];
		var strnum = Number(str.replace(/^.*sec=([0-9]*).*$/g, "$1"));
		var sec = "";

		if (strnum != 0) {
			if (strnum & 1)
				sec = sec + "WEP ";
			if (strnum & 2)
				sec = sec + "WPA ";
			if (strnum & 4)
				sec = sec + "WPA2 ";
			if (strnum & 8)
				sec = sec + "WPS ";
			if (strnum & 16)
				sec = sec + "WAPI ";
		}
		if (sec == "")
			sec = "None";
		sec = "sec=" + sec;
		str = str.replace(/sec=[0-9]*/g, sec);

		// Replace the cipher value to a readable string.
		var pcipherVal = Number(str.replace(/^.*pcipher=([0-9]*).*$/g, "$1"));
		var pcipherStr = "";
		if (pcipherVal != 0) {
			if (pcipherVal & 8)
				pcipherStr += "TKIP ";
			if (pcipherVal & 16)
				pcipherStr += "AES ";
		}
		if (pcipherStr == "")
			pcipherStr = "";
		pcipherStr = "pcipher=" + pcipherStr;
		str = str.replace(/pcipher=[0-9]*/g, pcipherStr);
		wdk.fill_survey_entry(wdk.getById('entry' + i), str + '&id=' + i);
	}

	wdk.getById('aplist').style.display = "block";
}

function ScanSurvey() {
	buttonDisable('survey', true);
	wdk.cliCmd('ap scan all');
	IntervalID = setInterval('ScanMsgDisp()', 1000);
	wdk.getById('ScanMsg').style.display = "block";
}

function sec_view_disp(sec_mode, cipher_mode) {
	if (sec_mode == DISABLE_SEC_MODE) //disable
	{
		showHide('wl_sec_option', 0)
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 0);
		wdk.getById('wl_sec_type').value = DISABLE_SEC_MODE;
	} else if (sec_mode & (WEP_SEC_MODE)) //wep
	{
		showHide('wl_sec_option', 1);
		showHide('wl_wep_disp', 1);
		showHide('wl_wpa_disp', 0);

		//for IE that 0 will display nothing because of <select>
		if (Number(wdk.cdbVal('$wl_bss_cipher2')) & (WEP40_CIPHER_MODE + WEP104_CIPHER_MODE))
			wdk.getById('wl_sec_wep_cipher').value = Number(wdk.cdbVal('$wl_bss_cipher2')) & (WEP40_CIPHER_MODE + WEP104_CIPHER_MODE)
		else
			wdk.getById('wl_sec_wep_cipher').value = WEP40_CIPHER_MODE;
		wdk.getById('wl_sec_wep_auth_mode').value = sec_mode & (OPEN_SEC_MODE + SHARED_SEC_MODE);
		wdk.getById('wl_sec_type').value = WEP_CIPHER_PLUS_SEC;
		wdk.getById('wl_sec_option').innerHTML = 'Static WEP Settings';
	} else if ((sec_mode & (WPA_WPA2_SEC_MODE)) == WPA_WPA2_SEC_MODE) //wpa/wpa2
	{
		showHide('wl_sec_option', 1);
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 1);
		showHide('wl_wapi_cipher_disp', 0);
		showHide('wl_wpa_cipher_disp', 1);
		showHide('wl_wpa_psk_disp', 1);
		/* Test
		if (Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE))
			wdk.getById('wl_wpa_cipher').value = Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE); //TKIP or AES
		else
			wdk.getById('wl_wpa_cipher').value = TKIP_CIPHER_MODE;
*/
		wdk.getById('wl_wpa_cipher').value = cipher_mode;
		wdk.getById('wl_sec_type').value = (WPA_WPA2_SEC_MODE);
		wdk.getById('wl_sec_option').innerHTML = 'WPA/WPA2 Settings';
	} else if (sec_mode & (WPA_SEC_MODE)) //wpa
	{
		showHide('wl_sec_option', 1);
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 1);
		showHide('wl_wapi_cipher_disp', 0);
		showHide('wl_wpa_cipher_disp', 1);
		showHide('wl_wpa_psk_disp', 1);
		/* Test
		if (Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE))
			wdk.getById('wl_wpa_cipher').value = Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE); //TKIP or AES
		else
			wdk.getById('wl_wpa_cipher').value = TKIP_CIPHER_MODE;
*/
		wdk.getById('wl_wpa_cipher').value = cipher_mode;
		wdk.getById('wl_sec_type').value = (WPA_SEC_MODE);
		wdk.getById('wl_sec_option').innerHTML = 'WPA Settings';
	} else if (sec_mode & (WPA2_SEC_MODE)) //wpa2
	{
		showHide('wl_sec_option', 1);;
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 1);
		showHide('wl_wapi_cipher_disp', 0);
		showHide('wl_wpa_cipher_disp', 1);
		showHide('wl_wpa_psk_disp', 1);
		/* Test
		if (Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE))
			wdk.getById('wl_wpa_cipher').value = Number(wdk.cdbVal('$wl_bss_cipher2')) & (AES_CIPHER_MODE + TKIP_CIPHER_MODE); //TKIP or AES
		else
			wdk.getById('wl_wpa_cipher').value = TKIP_CIPHER_MODE;
*/
		wdk.getById('wl_wpa_cipher').value = cipher_mode;
		wdk.getById('wl_sec_type').value = (WPA2_SEC_MODE);
		wdk.getById('wl_sec_option').innerHTML = 'WPA2 Settings';
	} else if (sec_mode & (WAPI_SEC_MODE)) //wapi
	{
		showHide('wl_sec_option', 1);;
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 1);
		showHide('wl_wapi_cipher_disp', 1);
		showHide('wl_wpa_cipher_disp', 0);
		showHide('wl_wpa_psk_disp', 1);

		wdk.getById('wl_wapi_cipher').value = WAPI_CIPHER_MODE;
		wdk.getById('wl_sec_type').value = (WAPI_SEC_MODE);
		wdk.getById('wl_sec_option').innerHTML = 'WAPI Settings';
	} else {
		showHide('wl_sec_option', 0)
		showHide('wl_wep_disp', 0);
		showHide('wl_wpa_disp', 0);
		wdk.getById('wl_sec_type').value = DISABLE_SEC_MODE;
	}
}

function key_confirm(key, idx, ssid, ssidmsg) {
	if (key.name == "wep") {
		var auth_len_idx = wdk.getById('$wl_bss_wep_index' + ssid).value;

		if (idx - 1 == auth_len_idx) {
			if (key.k[idx].value.length == 0) {
				key.tx_val = '';
				return 1;
			} else if ((key.k[idx].value.length == 5 && key.auth_len == OPEN_SEC_MODE) |
				(key.k[idx].value.length == 13 && key.auth_len == SHARED_SEC_MODE)) {
				if (!chkStr(key.k[idx], 'WEP Key' + idx)) {
					key.k[idx].focus();
					return 0;
				}
				key.tx_val = key.k[idx].value;
				return 1;
			} else if ((key.k[idx].value.length == 10 && key.auth_len == OPEN_SEC_MODE) |
				(key.k[idx].value.length == 26 && key.auth_len == SHARED_SEC_MODE)) {
				if (!chkHex(key.k[idx], 'WEP Key' + idx)) {
					key.k[idx].focus();
					return 0;
				}
				key.tx_val = key.k[idx].value;
				return 1;
			} else {
				if (key.auth_len == OPEN_SEC_MODE) {
					alert(ssidmsg + ' WEP Key' + idx + ' is Invalid!' + ' It should be 5 ASCII characters or 10 hex digits.');
				} else
					alert(ssidmsg + ' WEP Key' + idx + ' is Invalid!' + ' It should be 13 ASCII characters or 26 hex digits.');
				return 0;
			}
		} else {
			if (key.k[idx].value.length == 0) {
				key.tx_val = '';
				return 1;
			} else if ((key.k[idx].value.length == 5) || (key.k[idx].value.length == 13)) {
				if (!chkStr(key.k[idx], 'WEP Key' + idx)) {
					key.k[idx].focus();
					return 0;
				}
				key.tx_val = key.k[idx].value;
				return 1;
			} else if ((key.k[idx].value.length == 10) || (key.k[idx].value.length == 26)) {
				if (!chkHex(key.k[idx], 'WEP Key' + idx)) {
					key.k[idx].focus();
					return 0;
				}
				key.tx_val = key.k[idx].value;
				return 1;
			} else {
				alert(ssidmsg + ' WEP Key' + idx + ' is Invalid!');
				return 0;
			}
		}
	} else if (key.name == "wpa") {
		if (!chkPreKey(key.k[idx], ssidmsg + " Pre-shared Key")) {
			key.k[idx].focus();
			return 0;
		}
		key.tx_val = key.k[idx].value;
		return 1;
	}
	return 1;
}

function wl_apply() {
	var f = document.frm;
	var sec_mode = DISABLE_SEC_MODE;

	if (Number(f.$wl_enable.checked)) {
		wdk.cdbSet('$wl_bss_role1', WIFI_AP_ROLE);
		//Check the user's input is correct or not.
		switch (work_mode) {
			case '3': // WISP mode
			case '9': // Station mode		
				sec_mode = wdk.getById('wl_sec_type').value;
				if (!f.wl_bss_ssid2.value) {
					alert(wdk.msg("Target AP SSID") + wdk.msg(" must input !"));
					highlight("$wl_bss_ssid2");
					return false;
				}
				if (!chkMAC(f.wl_bss_bssid2, "Target AP BSSID")) {
					highlight("$wl_bss_bssid2");
					return false;
				}
			case '0': // 2-port ether
			case '1': // AP mode
			case '2': // Wireless Router mode
				if (!f.wl_bss_ssid1.value) {
					alert(wdk.msg("SSID") + wdk.msg(" must input !"));
					highlight("$wl_bss_ssid1");
					return false;
				}
				//if (/[;]/.test(f.wl_bss_ssid1.value)) {
				//	alert(wdk.msg("SSID") + wdk.msg(" can't contain any special characters as below") + "\n" + wdk.msg(" ; "));
				//	highlight("$wl_bss_ssid1");
				//	return false;
				//}
				if ((work_mode == 0) || (work_mode == 1) || (work_mode == 2))
					wdk.cdbSet('$wl_bss_role2', WIFI_NONE);
				break;
			case '5': // Bridge mode
				if (!f.wl_bss_ssid1.value) {
					alert(wdk.msg("SSID") + wdk.msg(" must input !"));
					highlight("$wl_bss_ssid1");
					return false;
				}
				//if (/[;]/.test(f.wl_bss_ssid1.value)) {
				//	alert(wdk.msg("SSID") + wdk.msg(" can't contain any special characters as below") + "\n" + wdk.msg(" ; "));
				//	highlight("$wl_bss_ssid1");
				//	return false;
				//}
			case '4': // Repeater mode
				if (!f.wl_bss_ssid2.value) {
					alert(wdk.msg("Target AP SSID") + wdk.msg(" must input !"));
					highlight("$wl_bss_ssid2");
					return false;
				}
				if (!chkMAC(f.wl_bss_bssid2, "Target AP BSSID")) {
					highlight("$wl_bss_bssid2");
					return false;
				}
				wdk.cdbSet('$wl_bss_role2', WIFI_STA_ROLE + WIFI_MAT);
				sec_mode = wdk.getById('wl_sec_type').value;

				break;
		}

		// prepare security information
		if (sec_mode == DISABLE_SEC_MODE) {} else if (sec_mode == WEP_CIPHER_PLUS_SEC) {
			var auth_mode = wdk.getById('wl_sec_wep_auth_mode').value;
			var obj_wep_key = new Object();
			obj_wep_key.k = new Array();
			obj_wep_key.num = 4;
			obj_wep_key.auth_len = wdk.getById('wl_sec_wep_cipher').value;
			obj_wep_key.name = "wep";

			for (i = 1; i <= obj_wep_key.num; i++) {
				obj_wep_key.k[i] = wdk.getById('$wl_bss_wep_' + i + 'key2');
			}
		} else // WPA, WPA2, WPA/WPA2 and WAPI
		{
			var obj_pre_key = new Object();
			obj_pre_key.k = new Array();
			obj_pre_key.k[0] = wdk.getById('$wl_bss_wpa_psk2');
			obj_pre_key.name = "wpa";
		}
		if (sec_mode == WEP_CIPHER_PLUS_SEC) {
			for (i = 1; i <= obj_wep_key.num; i++) {
				if ((wdk.getById('$wl_bss_wep_index2').value == (i - 1)) && obj_wep_key.k[i].value == '') {
					alert(wdk.msg("Target AP WEP Key" + i + " must input a value!"));
					//wdk.getById('$wl_bss_wep_'+i+'key2').focus();
					highlight("$wl_bss_wep_" + i + "key2");

					return;
				} {
					if (!key_confirm(obj_wep_key, i, 2, "Target AP")) {
						return;
					}
					wdk.cdbSet('$wl_bss_wep_' + i + 'key2', obj_wep_key.tx_val);
				}
			}
			wdk.cdbSet('$wl_bss_sec_type2', (auth_mode == OPEN_SEC_MODE) ? (OPEN_SEC_MODE + WEP_SEC_MODE) : (SHARED_SEC_MODE + WEP_SEC_MODE));
			wdk.cdbSet('$wl_bss_cipher2', wdk.getById('wl_sec_wep_cipher').value);
			wdk.cdbSet('$wl_bss_key_mgt2', '0'); //disable  802.1x.
		} else if (sec_mode == (WPA_SEC_MODE) || sec_mode == (WPA2_SEC_MODE) || sec_mode == (WPA_WPA2_SEC_MODE)) {
			{
				if (!key_confirm(obj_pre_key, 0, 2, "Target AP")) {
					return;
				}
				wdk.cdbSet('$wl_bss_wpa_psk2', obj_pre_key.tx_val);
			}
			wdk.cdbSet('$wl_bss_sec_type2', Number(sec_mode) + Number(OPEN_SEC_MODE));
			wdk.cdbSet('$wl_bss_cipher2', wdk.getById('wl_wpa_cipher').value);
			wdk.cdbSet('$wl_bss_key_mgt2', '0'); //disable  802.1x.
		} else if (sec_mode == (WAPI_SEC_MODE)) {
			{
				if (!key_confirm(obj_pre_key, 0, 2, "Target AP")) {
					return;
				}
				wdk.cdbSet('$wl_bss_wpa_psk2', obj_pre_key.tx_val);
			}
			wdk.cdbSet('$wl_bss_sec_type2', Number(sec_mode) + Number(OPEN_SEC_MODE));
			wdk.cdbSet('$wl_bss_cipher2', wdk.getById('wl_wapi_cipher').value);
			wdk.cdbSet('$wl_bss_key_mgt2', '0'); //disable  802.1x.
		} else // disable
		{
			wdk.cdbSet('$wl_bss_sec_type2', DISABLE_SEC_MODE);
			wdk.cdbSet('$wl_bss_cipher2', NONE_CIPHER_MODE);
			wdk.cdbSet('$wl_bss_key_mgt2', '0'); //disable  802.1x.
		}

		wdk.cdbSet('$wl_bss_ssid_hidden1', (Number(!f.wl_bss_ssid_hidden1.checked)));
		if (work_mode == 4) // Repeater mode
		{
			//wdk.getById('$wl_bss_ssid1').value = wdk.getById('$wl_bss_ssid2').value;
			wdk.cdbSet('$wl_bss_ssid_hidden1', 0);
			wdk.cdbSet('$wl_bss_sec_type1', wdk.cdbVal('$wl_bss_sec_type2'));
			wdk.cdbSet('$wl_bss_cipher1', wdk.cdbVal('$wl_bss_cipher2'));
			wdk.cdbSet('$wl_bss_key_mgt1', wdk.cdbVal('$wl_bss_key_mgt2'));
			wdk.cdbSet('$wl_bss_wep_index1', wdk.cdbVal('$wl_bss_wep_index2'));
			wdk.cdbSet('$wl_bss_wep_1key1', wdk.cdbVal('$wl_bss_wep_1key2'));
			wdk.cdbSet('$wl_bss_wep_2key1', wdk.cdbVal('$wl_bss_wep_2key2'));
			wdk.cdbSet('$wl_bss_wep_3key1', wdk.cdbVal('$wl_bss_wep_3key2'));
			wdk.cdbSet('$wl_bss_wep_4key1', wdk.cdbVal('$wl_bss_wep_4key2'));
			wdk.cdbSet('$wl_bss_wpa_psk1', wdk.cdbVal('$wl_bss_wpa_psk2'));
		}

		// WPS only support for ssid1
		if (Number(!f.wl_bss_ssid_hidden1.checked)) {
			if (Number(wdk.cdbVal('$wl_bss_sec_type1')) & Number(WPS_SEC_MODE)) {
				if (!confirm(wdk.msg("Are you sure to disable broadcast SSID?\nWPS will be also invalid!"))) {
					return;
				}
				wdk.cdbSet('$wl_bss_sec_type1', Number(wdk.cdbVal('$wl_bss_sec_type1')) & ~WPS_SEC_MODE);
			}
		}

		//	WPS 2.0 SPEC 12 p128
		//	AP must transition to the "Configured" state if any of the following occur:
		//	1. ...
		//	2. ...
		//	3. Manual configuration by user.
		//		modify any one of the following:
		//		1.the SSID
		//		2.the encryption algorithm
		//		3.the authentication algorithm
		//		4.any key or pass phrase

		if (wdk.cdbVal('$wl_bss_ssid1') != wdk.getById('$wl_bss_ssid1').value) {
			wdk.cdbSet('$wl_bss_wps_state1', 2);
		}
	}
	wdk.cdbSet('$wl_bss_num', SSID_NUM);
	return true;
}

wdk.cdbLoad([
	'$dhcps_enable',
	'$wan_pmode',
	'$wl_bss_enable1-2',
]);

// function trsltn() {

// 	wdk.getById("w1").text = wdk.msg('AP');
// 	wdk.getById("w3").text = wdk.msg('WISP');
// 	wdk.getById("w9").text = wdk.msg('STA');
// }

// function init() {
// 	trsltn();
// 	wdk.init_form();
// 	showTargetAP();
// 	//Apply wireless setting.
// 	wl_init();
// }

//function apply() {
//	var f = document.frm;
//	var sys_wm = wdk.getByName(f, 'sys_wm');
//
//	wdk.cdbSet('$wl_bss_enable1', 1);
//	wdk.cdbSet('$wl_bss_enable2', 0);
//	wdk.cdbSet('$dhcps_enable', 1); // default on
//	switch (sys_wm.value) {
//		case '1': // AP
//			break;
//		case '2': // Wireless Router
//			break;
//		case '3': // WISP
//			wdk.cdbSet('$wl_bss_enable2', 1);
//			break;
//		case '9': // Station
//			wdk.cdbSet('$wl_bss_enable2', 1);
//			break;
//		case '4': // AP Client Repeater
//		case '5': // AP Client Bridge
//			wdk.cdbSet('$dhcps_enable', 0);
//			wdk.cdbSet('$wl_bss_enable2', 1);
//			break;
//		case '7': // 3G
//			//reserve previous $wan_mode when switching to 3G mode
//			if (work_mode != 7) {
//				var w = wdk.cdbVal('$wan_mode');
//				wdk.cdbSet('$wan_pmode', w);
//			}
//			wdk.cdbSet('$wan_mode', 9);
//			break;
//	}
//
//	wdk.cdbSet('$op_work_mode', sys_wm.value);
//
//	//restore previous $wan_mode when out of 3G mode
//	if (wdk.cdbChg('$op_work_mode') && (work_mode == 7)) {
//		var p = wdk.cdbVal('$wan_pmode');
//		wdk.cdbSet('$wan_mode', p);
//	}
//
//	//Apply wireless setting.
//	if (scanResult)
//		wl_apply();
//
//	if (wdk.cdbChg('$op_work_mode')) {
//		wdk.showResultBG();
//		reboot();
//		wdk.save_form(0, 1);
//	} else
//		wdk.save_form(2000);
//}

// var modeVal = [3, 4, 5, 9]; // WISP, RPTR, BRGE, STA

// var selVal;
// var scanResult;

// function showTargetAP() {
// 	scanResult = false;
// 	var sel = wdk.getById("$op_work_mode");
// 	var selInd = sel.selectedIndex;
// 	selVal = sel.options[selInd].value;
// 	selVal = parseInt(selVal);
// 	for (var i = 0; i < modeVal.length; i++) {
// 		if (modeVal[i] == selVal) {
// 			scanResult = true;
// 			break;
// 		}
// 	}

// 	if (scanResult) {
// 		document.getElementById("wl_wbs2_hiden").style.display = 'block';
// 	} else {

// 		document.getElementById("wl_wbs2_hiden").style.display = 'none';
// 		document.getElementById("aplist").style.display = 'none';
// 	}
// }

/**
 * [wl_init For work.htm ONLY, since this function's instructions are
 * included in wireless_bais::init(). But each init() is unique, those
 * function can't be extracted to one copy.]
 * @return {[type]} [description]
 */
// function wl_init() {
// 	var f = document.frm;
// 	//critical
// 	init_working_mode(Number(wdk.cdbVal('$wl_enable')));
// 	wdk.init_form();
// 	f.wl_bss_ssid_hidden1.checked = (Number(wdk.cdbVal('$wl_bss_ssid_hidden1')) == 0) ? 1 : 0;

// 	var mgt = wdk.cdbVal('$wl_bss_key_mgt2');
// 	var sec_mode = wdk.cdbVal('$wl_bss_sec_type2');

// 	if (mgt == 1) // whether 8021x
// 	{
// 		// we don't support client mode using 8021X
// 		wdk.cdbSet('$wl_bss_key_mgt2', 0);
// 	}
// 	sec_view_disp(sec_mode);
// }

function addAutoChannelOpt() {
	var channelSel = document.getElementById("$wl_channel");
	var autoChannel = document.createElement("option");
	autoChannel.value = 0;
	autoChannel.innerHTML = "auto";
	channelSel.appendChild(autoChannel);
}

function forSpecialChar() {
	var singleQuote = "'";
	var ssidNameArr = ["$wl_bss_ssid1", "$wl_bss_ssid2"];
	for (var i = 0; i < ssidNameArr.length; i++) {
		var ssidInput = document.getElementById(ssidNameArr[i]);
		var ssidVal = ssidInput.value;
		if (ssidVal[0] == singleQuote && ssidVal[ssidVal.length - 1] != singleQuote) {
			ssidVal = singleQuote + ssidVal;
			ssidInput.value = ssidVal;
		}
		if (ssidVal[0] == singleQuote && ssidVal[ssidVal.length - 1] == singleQuote) {
			ssidVal = singleQuote + ssidVal + singleQuote;
			ssidInput.value = ssidVal;
		}
	}
}
