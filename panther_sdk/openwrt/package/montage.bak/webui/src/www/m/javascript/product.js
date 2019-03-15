wdk.cdbLoad([
	'$sys_idle',
	'$sys_language',
	'$op_work_mode',
	'$wan_mode',
	'$ra_func'
]);

//wdk._langInit();
wdk._idleInit();
var work_mode = wdk.cdbVal('$op_work_mode');
var wan_mode = Number(wdk.cdbVal('$wan_mode'));
var menus_idx;
var pages_idx;
var radioPage = null;
var audioPage = null;


function findPage(name) {
	for (var i = 0; i < pages.length; i++) {
		if (pages[i].name == name)
			return pages[i];
	}
	return 0;
}

function findGroup(name) {
	var p = findPage(name);
	if (p) {
		for (var i = 0; i < menus.length; i++) {
			if (menus[i].mid == p.gid)
				return menus[i];
		}
	}
	return 0;
}

var WAN_USERNAME_MIN_LEN = 0;
var WAN_USERNAME_MAX_LEN = 64;
var WAN_PASSWD_MIN_LEN = 0;
var WAN_PASSWD_MAX_LEN = 64;
var SSID_MAX_NUM = 4;

function header() {
	//resultbg
	var m = '<div id=resultbg style="display:none;" class=black_overlay>' +
		'<div id=result class=white_content>' +
		'<img src="images/load.gif" alt="loading icon" />' +
		'<div><font id="titleupwarng"></font></div>' +
		'</div>' +
		'</div>' +
		puts(m);
}

function cancel() {
	init();
}

function wanModeChg(m, op) {
	if (op == 3) {
		switch (m) {
			case 1:
				window.location = 'fixip.htm';
				break;
			default:
				return;
		}
	}
	else {
		switch (m) {
			case 1:
				window.location = 'fixip.htm';
				break;
			default:
				return;
		}
	}
}

function wanModeList(wm, op) {
	wdk.cdbSet('$wan_mode', wm);
	var m = '<fieldset><legend>' + wdk.msg('WAN Connection Mode') + '</legend>';
	m += '<table id=tabContent class=setting border=0>';

	if (op == 3) {
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(1,opWorkMode) ' + ((wm == 1) ? 'id=$wan_mode value=1' : '') + '>' + wdk.msg('Static IP') + '</input>' +
			'</td><td>' +
			wdk.msg('Use a static IP address. Your service provider gives a static IP address to access Internet services.') +
			'</td></tr>';
	}
	else {
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(1,opWorkMode) ' + ((wm == 1) ? 'id=$wan_mode value=1' : '') + '>' + wdk.msg('Static IP') + '</input>' +
			'</td><td>' +
			wdk.msg('Use a static IP address. Your service provider gives a static IP address to access Internet services.') +
			'</td></tr>';
	}
	m += '</table></fieldset>';
	puts(m);
}

var timeTable = [
	[0, "-12:00", "Enewetak, Kwajalein"],
	[1, "-11:00", "Midway Island, Samoa"],
	[2, "-10:00", "Hawaii"],
	[3, "-09:00", "Alaska"],
	[4, "-08:00", "Pacific Time (US &amp; Canada);Tijuana"],
	[5, "-07:00", "Arizona"],
	[6, "-07:00", "Mountain Time (US &amp; Canada)"],
	[7, "-06:00", "Central Time (US &amp; Canada)"],
	[8, "-06:00", "Mexico City, Tegucigalpa"],
	[9, "-06:00", "Saskatchewan"],
	[10, "-05:00", "Bogota, Lima, Quito"],
	[11, "-05:00", "Eastern Time (US &amp; Canada)"],
	[12, "-05:00", "Indiana (East)"],
	[13, "-04:00", "Atlantic Time (Canada)"],
	[14, "-04:00", "Caracas, La Paz"],
	[15, "-04:00", "Caracas, La Paz"],
	[16, "-03:00", "Newfoundland"],
	[17, "-03:00", "Brasilia"],
	[18, "-03:00", "Buenos Aires, Georgetown"],
	[19, "-02:00", "Mid-Atlantic"],
	[20, "-01:00", "Azores, Cape Verde Is."],
	[21, "-01:00", "Casablanca, Monrovia"],
	[22, "+00:00", "Greenwich Mean Time: Dublin, Edinburgh"],
	[23, "+00:00", "Greenwich Mean Time: Lisbon, London"],
	[24, "+01:00", "Amsterdam, Berlin, Bern, Rome"],
	[25, "+01:00", "Stockholm, Vienna, Belgrade"],
	[26, "+01:00", "Bratislava, Budapest, Ljubljana"],
	[27, "+01:00", "Prague,Brussels, Copenhagen, Madrid"],
	[28, "+01:00", "Paris, Vilnius, Sarajevo, Skopje"],
	[29, "+01:00", "Sofija, Warsaw, Zagreb"],
	[30, "+02:00", "Athens, Istanbul, Minsk"],
	[31, "+02:00", "Bucharest"],
	[32, "+02:00", "Cairo"],
	[33, "+02:00", "Harare, Pretoria"],
	[34, "+02:00", "Helsinki, Riga, Tallinn"],
	[35, "+02:00", "Helsinki, Riga, Tallinn"],
	[36, "+03:00", "Baghdad, Kuwait, Nairobi, Riyadh"],
	[37, "+03:00", "Moscow, St. Petersburg"],
	[38, "+03:00", "Tehran"],
	[39, "+04:00", "Abu Dhabi, Muscat, Tbilisi, Kazan"],
	[40, "+04:00", "Volgograd, Kabul"],
	[41, "+05:00", "Islamabad, Karachi, Ekaterinburg"],
	[42, "+06:00", "Almaty, Dhaka"],
	[43, "+07:00", "Bangkok, Jakarta, Hanoi"],
	[44, "+08:00", "Beijing, Chongqing, Urumqi"],
	[45, "+08:00", "Hong Kong, Perth, Singapore, Taipei"],
	[46, "+09:00", "Toyko, Osaka, Sapporo, Yakutsk"],
	[47, "+10:00", "Brisbane"],
	[48, "+10:00", "Canberra, Melbourne, Sydney"],
	[49, "+10:00", "Guam, Port Moresby, Vladivostok"],
	[50, "+10:00", "Hobart"],
	[51, "+11:00", "Magadan, Solamon, New Caledonia"],
	[52, "+12:00", "Fiji, Kamchatka, Marshall Is."],
	[53, "+12:00", "Wellington, Auckland"]
];

function genTimeOpt() {
	var s = '';
	for (var i = 0; i < timeTable.length; i++) {
		var t = timeTable[i];
		s += '<option value=' + t[0] + '> (GMT' + t[1] + ') ' + t[2] + '</option>';
	}
	document.write(s);
}
var _months = ['JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];

function genMonOpt() {
	var s = '';
	for (var i = 0; i < 12; i++) {
		var t = wdk.msg(_months[i]);
		s += '<option value=' + (i + 1) + '>' + t + '</option>';
	}
	document.write(s);
}

function genDayOpt() {
	var s = '';
	for (var i = 1; i < 32; i++) {
		s += '<option value=' + (i) + '>' + i + '</option>';
	}
	document.write(s);
}

var week = ["Sun.", "Mon.", "Tues.", "Wed.", "Thur.", "Fri.", "Sat."];
var month = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

function createSelectTagChild(start, end, selected, special_case, t, val) {
	var str = new String("");

	for (var i = start; i <= end; i++) {
		switch (special_case) {
			case "week":
				if (i == selected)
					str += "<option value=" + i + " selected>" + week[i] + "</option>";
				else
					str += "<option value=" + i + ">" + week[i] + "</option>";
				break;
			case "month":
				if (i == selected)
					str += "<option value=" + i + " selected>" + month[i - 1] + "</option>";
				else
					str += "<option value=" + i + ">" + month[i - 1] + "</option>";
				break;
			case "table":
				if (i == selected)
					str += "<option value=" + i + " selected>" + t[i] + "</option>";
				else
					str += "<option value=" + i + ">" + t[i] + "</option>";
				break;
			case "diffvalue":
				if (i == selected)
					str += "<option value=" + val[i] + " selected>" + t[i] + "</option>";
				else
					str += "<option value=" + val[i] + ">" + t[i] + "</option>";
				break;
			default:
				if (i == selected)
					str += "<option value=" + i + " selected>" + i + "</option>";
				else
					str += "<option value=" + i + ">" + i + "</option>";
				break;
		}
	}
	document.writeln(str);
}

/* Highlight the input row as the error occurs.
 *  id: The dom element id like: "$wl_bss_ssid1"
 */
var highlightedArr = new Array();

function highlight(id) {
	if (id == null) return;

	//Clear all previous highlighted elements.
	for (var j in highlightedArr) {
		document.getElementById(highlightedArr[j]).removeAttribute("style");
	}

	highlightedArr = [];

	highlightedArr.push(id);
	var highlightColor = "rgb(255,255,0)";
	if (id == null) return;
	var target = document.getElementById(id);
	target.style.backgroundColor = highlightColor;
	target.focus();
}

/**
 * To show a prompt and reboot DUT, then redirecting page after a specific period.
 * The default case is to wait a small periold for saving cdb variables then rebooting DUT.
 * @method reboot
 * @param {number} t - add extra waitng time delay time for reboot (unit:second)
 * @param {number} d - delay time before sending reboot request
 * @return {undefined}
 */
function reboot(t, d) {
	var TD = 15;
	var D = 2;
	var td = TD + (t ? t : 0) + (d ? d : 0);
	var rd = td - ((d) ? d : D);
	if (wdk)
		wdk.redirect_restore(td, rd);
	else
		alert("please include wdk.js");
}
