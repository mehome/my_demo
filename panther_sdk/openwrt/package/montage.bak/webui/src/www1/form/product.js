var menus = [
	/*--ifdef CONFIG_WIZARD_WEB--*/
	{
		name: 'wizard.htm',
		title: 'Quick Setup',
		mid: 0
	},
	/*--endif--*/
	{
		name: 'admin.htm',
		title: 'Admin',
		mid: 1
	},
	/*--ifdef CONFIG_WAN--*/
	{
		name: 'wan.htm',
		title: 'WAN',
		mid: 2
	},
	/*--endif--*/
	/*--ifdef CONFIG_WLAN--*/
	{
		name: 'wireless_basic.htm',
		title: 'Wireless',
		mid: 3
	},
	/*--endif--*/
	{
		name: 'dhcps.htm',
		title: 'LAN',
		mid: 4
	},
	/*--ifdef CONFIG_NAT--*/
	{
		name: 'vserver.htm',
		title: 'NAT',
		mid: 5
	},
	/*--endif--*/
	/*--ifdef CONFIG_FW--*/
	{
		name: 'hacker.htm',
		title: 'Firewall',
		mid: 6
	},
	/*--endif--*/
	{
		name: 'routetable.htm',
		title: 'Routing',
		mid: 7
	}, {
		/*--ifdef CONFIG_MPEGTS_ASYNCFIFO--*/
		name: 'stb.htm',
		/*--endif--*/
		/*--ifdef CONFIG_IPV6--*/
		name: 'ipv6.htm',
		/*--endif--*/
		/*--ifdef CONFIG_TFTPC--*/
		name: 'tftpc.htm',
		/*--endif--*/
		/*--ifdef CONFIG_DDNS--*/
		name: 'ddns.htm',
		/*--endif--*/
		/*--ifdef CONFIG_UPNP--*/
		name: 'upnp.htm',
		/*--endif--*/
		/*--ifdef CONFIG_PRINTER_SERVER--*/
		name: 'printer.htm',
		/*--endif--*/
		title: 'Misc',
		mid: 9
	}, {
		/*--ifdef CONFIG_VIDEO_STREAM--*/
		name: 'stream.htm',
		/*--endif--*/
		title: 'Webcam',
		mid: 10
	}, {
		/*--ifdef CONFIG_RECORDER--*/
		name: 'record.htm',
		/*--endif--*/
		/*--ifdef CONFIG_DLNA--*/
		name: 'dlna.htm',
		/*--endif--*/
		/*--ifdef CONFIG_SAMBA--*/
		name: 'smb.htm',
		/*--endif--*/
		/*--ifdef CONFIG_ELFINDER--*/
		name: 'elfinder.htm',
		/*--endif--*/
		/*--ifdef CONFIG_FILE_EXPLORER--*/
		name: 'files.htm',
		/*--endif--*/
		title: 'Storage',
		mid: 11
	},
	/*--ifdef CONFIG_MEDIA--*/
	{
		name: 'mediaControl.htm',
		title: 'Media',
		mid: 12
	},
	/*--endif--*/
	{
		name: 'status.htm',
		title: 'Status',
		mid: 13
	}
];
var pages = [
	/*--ifdef CONFIG_WIZARD_WEB--*/
	{
		name: 'wizard.htm',
		title: 'Wizard',
		gid: 0
	},
	/*--endif--*/
	{
		name: 'admin.htm',
		title: 'Management',
		gid: 1
	}, {
		name: 'work.htm',
		title: 'Working Mode',
		gid: 1
	},
	/*--ifdef CONFIG_HOTSPOT--*/
	{
		name: 'uam.htm',
		title: 'Hotspot',
		gid: 1
	},
	/*--endif--*/
	{
		name: 'hostconf.htm',
		title: 'System Settings',
		gid: 1
	}, {
		name: 'upgrade.htm',
		title: 'Firmware Upgrade',
		gid: 1
	}, {
		name: 'config.htm',
		title: 'Configuration',
		gid: 1
	}, {
		name: 'tool.htm',
		title: 'Tools',
		gid: 1
	}, {
		name: 'lang.htm',
		title: 'Language',
		gid: 1
	}, {
		name: 'logconf.htm',
		title: 'Log Settings',
		gid: 1
	}, {
		name: 'logout.htm',
		title: 'Logout',
		gid: 1
	},
	/*--ifdef CONFIG_WAN--*/
	{
		name: 'wan.htm',
		title: 'WAN Mode',
		gid: 2
	},
	/*--endif--*/
	/*--ifdef CONFIG_DHCPC--*/
	{
		name: 'dhcpc.htm',
		title: 'DHCP Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 2
	},
	/*--endif--*/
	{
		name: 'fixip.htm',
		title: 'Static IP Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 1
	},
	/*--ifdef CONFIG_PPPOE--*/
	{
		name: 'pppoe.htm',
		title: 'PPPOE Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 3
	},
	/*--endif--*/
	/*--ifdef CONFIG_PPTP--*/
	{
		name: 'pptp.htm',
		title: 'PPTP Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 4
	},
	/*--endif--*/
	/*--ifdef CONFIG_L2TP--*/
	{
		name: 'l2tp.htm',
		title: 'L2TP Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 5
	},
	/*--endif--*/
	/*--ifdef CONFIG_BPA--*/
	{
		name: 'bigpond.htm',
		title: 'Bigpond Cable Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 6
	},
	/*--endif--*/
	/*--ifdef CONFIG_OP_3G--*/
	{
		name: '3g.htm',
		title: '3G Mode',
		gid: 2,
		hidden: 1,
		wan_mode_idx: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_WLAN--*/
	{
		name: 'wireless_basic.htm',
		title: 'Basic Settings',
		gid: 3
	}, {
		name: 'wireless_security.htm',
		title: 'Security',
		gid: 3
	}, {
		name: 'wireless_macf.htm',
		title: 'MAC Filtering',
		gid: 3
	}, {
		name: 'wireless_advanced.htm',
		title: 'Advanced Settings',
		gid: 3
	},
	/*--ifdef CONFIG_WEB_WPS--*/
	{
		name: 'wireless_wps.htm',
		title: 'WPS',
		gid: 3
	},
	/*--endif--*/
	/*--ifdef CONFIG_WLAN_GPIO_TRIGGER--*/
	{
		name: 'wireless_gpio_trig.htm',
		title: 'GPIO',
		gid: 3
	},
	/*--endif--*/
	/*--ifdef CONFIG_SMARTCONFIG--*/
	{
		name: 'smart_config.htm',
		title: 'Smart Config',
		gid: 3
	},
	/*--endif--*/
	/*--ifdef CONFIG_WDS--*/
	{
		name: 'wireless_wds.htm',
		title: 'WDS',
		gid: 3
	},
	/*--endif--*/
	{
		name: 'wireless_stationlist.htm',
		title: 'Station List',
		gid: 3
	},
	/*--endif--*/
	{
		name: 'dhcps.htm',
		title: 'LAN Settings',
		gid: 4
	}, {
		name: 'clientlist.htm',
		title: 'DHCP Client List',
		gid: 4
	}, {
		name: 'vserver.htm',
		title: 'Virtual Server',
		gid: 5
	}, {
		name: 'porttrig.htm',
		title: 'Port Triggering',
		gid: 5
	}, {
		name: 'portmap.htm',
		title: 'Port Mapping',
		gid: 5
	}, {
		name: 'alg.htm',
		title: 'Passthrough',
		gid: 5
	}, {
		name: 'dmz.htm',
		title: 'DMZ',
		gid: 5
	}, {
		name: 'hacker.htm',
		title: 'Firewall Options',
		gid: 6
	}, {
		name: 'clientf.htm',
		title: 'Client Filtering',
		gid: 6
	}, {
		name: 'urlf.htm',
		title: 'URL Filtering',
		gid: 6
	}, {
		name: 'macf.htm',
		title: 'MAC Filtering',
		gid: 6
	}, {
		name: 'routetable.htm',
		title: 'Routing Table',
		gid: 7
	},
	/*--ifdef CONFIG_STATIC_ROUTE--*/
	{
		name: 'staticroute.htm',
		title: 'Static Routing',
		gid: 7
	},
	/*--endif--*/
	/*--ifdef CONFIG_UPNP--*/
	{
		name: 'upnp.htm',
		title: 'UPnP',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_DDNS--*/
	{
		name: 'ddns.htm',
		title: 'DDNS',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_TFTPC--*/
	{
		name: 'tftpc.htm',
		title: 'TFTP Client',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_IPV6--*/
	{
		name: 'ipv6.htm',
		title: 'IPv6',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_MPEGTS_ASYNCFIFO--*/
	{
		name: 'stb.htm',
		title: 'STB',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_PRINTER_SERVER--*/
	{
		name: 'printer.htm',
		title: 'Printer',
		gid: 9
	},
	/*--endif--*/
	/*--ifdef CONFIG_VIDEO_STREAM--*/
	{
		name: 'stream.htm',
		title: 'RTP Streaming',
		gid: 10
	},
	/*--endif--*/
	{
		name: 'webcam_record.htm',
		title: 'Record',
		gid: 10
	},
	/*--ifdef CONFIG_SONIX_MOTION_DETECTION--*/
	{
		name: 'sonix_md.htm',
		title: 'Motion Detection',
		gid: 10
	},
	/*--endif--*/
	/*--ifdef CONFIG_WEBCAM_IMAGE_PARAMETERS--*/
	{
		name: 'image.htm',
		title: 'Image',
		gid: 10
	},
	/*--endif--*/
	/*--ifdef CONFIG_SONIX_IMAGE--*/
	{
		name: 'sonix_image.htm',
		title: 'Extension',
		gid: 10
	},
	/*--endif--*/
	/*--ifdef CONFIG_SONIX_OSD--*/
	{
		name: 'sonix_osd.htm',
		title: 'On-Screen Display',
		gid: 10
	},
	/*--endif--*/
	/*--ifdef CONFIG_PACKAGE_samba36_client--*/
	{
		name: 'samba_upload.htm',
		title: 'Samba Upload',
		gid: 10
	},
	/*--endif--*/
	/*--ifdef CONFIG_FILE_EXPLORER--*/
	{
		name: 'files.htm',
		title: 'File Explorer',
		gid: 11
	},
	/*--endif--*/
	/*--ifdef CONFIG_ELFINDER--*/
	{
		name: 'elfinder.htm',
		title: 'elfinder',
		gid: 11
	},
	/*--endif--*/
	/*--ifdef CONFIG_SAMBA--*/
	{
		name: 'smb.htm',
		title: 'Samba',
		gid: 11
	},
	/*--endif--*/
	/*--ifdef CONFIG_DLNA--*/
	{
		name: 'dlna.htm',
		title: 'DLNA',
		gid: 11
	},
	/*--endif--*/
	/*--ifdef CONFIG_RECORDER--*/
	{
		name: 'record.htm',
		title: 'Record',
		gid: 11
	},
	/*--endif--*/
	/*--ifdef CONFIG_MEDIA--*/
	{
		name: 'mediaControl.htm',
		title: 'Media Control',
		gid: 12
	},
	/*--endif--*/
	/*--ifdef CONFIG_RADIO--*/
	{
		name: 'radio.htm',
		title: 'Radio',
		gid: 12
	}, {
		name: 'pandora.htm',
		title: 'Pandora',
		gid: 12
	},
	/*--endif--*/
	/*--ifdef CONFIG_AUDIOPLAYER--*/
	{
		name: 'audio.htm',
		title: 'Audio Player',
		gid: 12
	},
	/*--endif--*/
	{
		name: 'status.htm',
		title: 'Status',
		gid: 13
	},
	/*--ifdef CONFIG_LOG--*/
	{
		name: 'log.htm',
		title: 'Log',
		gid: 13
	},
	/*--endif--*/
	{
		name: null,
		title: null,
		gid: null
	}
];

wdk.cdbLoad([
	'$sys_idle',
	/*--ifdef CONFIG_BRIDGE_MODE--*/
	'$sys_funcmode',
	/*--endif--*/
	'$sys_language',
	'$op_work_mode',
	'$wan_mode',
	'$ra_func',
	'$ra_radiotype'
]);

wdk._langInit();
//wdk._idleInit();
/*--ifdef CONFIG_BRIDGE_MODE--*/
var funcmode = wdk.cdbVal('$sys_funcmode');
/*--endif--*/
var work_mode = wdk.cdbVal('$op_work_mode');
var wan_mode = Number(wdk.cdbVal('$wan_mode'));
var menus_idx;
var pages_idx;
var radioPage = null;
var audioPage = null;
var pandoraPage = null;

function parsePages() {
	var raFunc = Number(wdk.cdbVal('$ra_func'));
	var raRadioType = Number(wdk.cdbVal('$ra_radiotype'));
	switch (raFunc) {
		case 0:
			if (radioPage) radioPage.hidden = 1;
			if (audioPage) audioPage.hidden = 1;
			if (pandoraPage) pandoraPage.hidden = 1;
			break;

		case 1:
			if (radioPage) {
				radioPage.hidden = (raRadioType == 0) ? 0 : 1;
			}
			if (pandoraPage) {
				pandoraPage.hidden = (raRadioType == 1) ? 0 : 1;
			}

			if (audioPage) audioPage.hidden = 1;
			break;

		case 2:
			if (radioPage) radioPage.hidden = 1;
			if (audioPage) audioPage.hidden = 1;
			if (pandoraPage) pandoraPage.hidden = 1;
			break;

		case 3:
			if (radioPage) radioPage.hidden = 1;
			if (audioPage) audioPage.hidden = 0;
			if (pandoraPage) pandoraPage.hidden = 1;
			break;
	}
}

for (pages_idx = 0; pages_idx < pages.length; pages_idx++) {
	if (typeof pages[pages_idx].wan_mode_idx != 'undefined' && pages[pages_idx].wan_mode_idx == wan_mode) {
		for (menus_idx = 0; menus_idx < menus.length; menus_idx++) {
			if (menus[menus_idx].name == 'wan.htm') {
				menus[menus_idx].name = pages[pages_idx].name;
				break;
			}
		}
	}
	/*--ifdef CONFIG_RADIO--*/
	if (pages[pages_idx].name == "radio.htm")
		radioPage = pages[pages_idx];

	if (pages[pages_idx].name == "pandora.htm")
		pandoraPage = pages[pages_idx];
	/*--endif--*/
	/*--ifdef CONFIG_AUDIOPLAYER--*/
	if (pages[pages_idx].name == "audio.htm")
		audioPage = pages[pages_idx];
	/*--endif--*/
}
parsePages();

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


function mobweb() {
	location = window.location.protocol + "//" + window.location.hostname + (window.location.port ? ':' + window.location.port : '') + '/m/';
}

function header() {
	var cg = findGroup(wdk.getThisPage());
	var tfile = wdk.getTranslationFile();
	var m = ((tfile == '') ? '' : '<SCRIPT src="' + wdk.getTranslationFile() + '"></SCRIPT>') +
		'<div id=wrapper>' +
		//resultbg
		'<div id=resultbg style="display:none;" class=black_overlay>' +
		'<div id=result class=white_content>' +
		'<img src="images/load.gif" alt="loading icon" />' +
		'<div><font id="titleupwarng"></font></div>' +
		'</div>' +
		'</div>' +
		//header
		'<div id=header>' +
		'<div id=banner>' +
		'<div id=logo onclick=goUrl("http://www.montage-tech.com/Wi-Fi/index.html") ></div>' + '<div id="switchMobWeb" onclick=mobweb() >Mobile</div>' +
		'</div>' +
		'<div id=menu>' +
		'<ul id=menuMainList>';
	for (var i = 0; i < menus.length; i++) {
		if (!menus[i].name)
			continue;
		if (menus[i].title != cg.title) // not current
		{
			/*--ifdef CONFIG_BRIDGE_MODE--*/
			if (funcmode == 1) //bridge mode
			{
				switch (menus[i].title) {
					case 'WAN':
					case 'NAT':
					case 'Firewall':
					case 'Routing':
					case 'Misc':
						m += '<li><a><script>wdk.putmsg("' + menus[i].title + '")</script></a>';
						break;
					default:
						m += '<li><a href=' + menus[i].name + '><script>wdk.putmsg("' + menus[i].title + '")</script></a>';
						break;
				}
			} else
			/*--endif--*/
			{
				if (work_mode == 1 || work_mode == 4 || work_mode == 5) //ap mode or repeater or bridge
				{
					switch (menus[i].title) {
						case 'WAN':
							break;
						default:
							m += '<li><a href=' + menus[i].name + '><script>wdk.putmsg("' + menus[i].title + '")</script></a>';
							break;
					}
				} else
					m += '<li><a href=' + menus[i].name + '><script>wdk.putmsg("' + menus[i].title + '")</script></a>';
			}
		} else //current
		{
			/*--ifdef CONFIG_BRIDGE_MODE--*/
			if (funcmode == 1) //bridge mode
			{
				switch (menus[i].title) {
					case 'WAN':
					case 'NAT':
					case 'Firewall':
					case 'Routing':
					case 'Misc':
						goUrl('status.htm');
						break;
					default:
						break;
				}
			}
			/*--endif--*/
			if (work_mode == 1 || work_mode == 4 || work_mode == 5) //ap mode or repeater or bridge
			{
				switch (menus[i].title) {
					case 'WAN':
						goUrl('status.htm');
						break;
					default:
						break;
				}
			}

			m += '<li class=current><span><script>wdk.putmsg("' + menus[i].title + '")</script></span>';
			m += '<div id=menuSub><ul id=menuSubList>';
			for (var j = 0; j < pages.length; j++) {
				if (menus[i].mid != pages[j].gid)
					continue;
				if (pages[j].hidden)
					continue;
				if (work_mode == 4) // repeater
				{
					if (pages[j].title == 'Security')
						continue;
				}
				//current goup
				if (pages[j].name == wdk.getThisPage() || pages[j].gid == 2 || pages[j].gid == 8)
					m += '<li><span><script>wdk.putmsg("' + pages[j].title + '")</script></span></li>';
				else
					m += '<li><a href=' + pages[j].name + '><script>wdk.putmsg("' + pages[j].title + '")</script></a></li>';
			}
			m += '</ul></div>';
		}
		m += '</li>';
	}
	m += '</ul>' +
		'</div>' +
		'</div>' +
		'<div id=main>';
	puts(m);
}

function footer(nodef) {
	var cp = findPage(wdk.getThisPage());
	var m = '';
	if (!nodef) {
		m += '<br><div class=submitFooter id=button>' + '<input title="Apply settings" class=button name=apply_button value="' + wdk.msg('OK') + '" type=button onclick=apply()>' + '<input title="Cancel changes" class=button name=reset_button value="' + wdk.msg('Cancel') + '" type=button onclick=cancel()>' + '</div>';
	}
	m += '</div><!--/main-->';
	m += '<br><div id=help>' + '<div><h2> Help</h2></div>' + '<iframe width=100% height=90% frameborder=0 scrolling=no src="' + wdk.getHelpFile() + '#' + cp.name + '"></iframe>' + '</div>' + '<!--/div-->';
	m += '<div class=floatKiller></div>' + '</div><!--/Wrapper-->';

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
				/*--ifdef CONFIG_DHCPC--*/
			case 2:
			case 3:
			case 4:
			case 5:
				window.location = 'dhcpc.htm';
				break;
				/*--endif--*/
			default:
				return;
		}
	}
	/*--ifdef CONFIG_OP_3G--*/
	else if (op == 7) {
		window.location = '3g.htm';
	}
	/*--endif--*/
	else {
		switch (m) {
			case 1:
				window.location = 'fixip.htm';
				break;
				/*--ifdef CONFIG_DHCPC--*/
			case 2:
				window.location = 'dhcpc.htm';
				break;
				/*--endif--*/
				/*--ifdef CONFIG_PPPOE--*/
			case 3:
				window.location = 'pppoe.htm';
				break;
				/*--endif--*/
				/*--ifdef CONFIG_PPTP--*/
			case 4:
				window.location = 'pptp.htm';
				break;
				/*--endif--*/
				/*--ifdef CONFIG_L2TP--*/
			case 5:
				window.location = 'l2tp.htm';
				break;
				/*--endif--*/
				/*--ifdef CONFIG_BPA--*/
			case 6:
				window.location = 'bigpond.htm';
				break;
				/*--endif--*/
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
		/*--ifdef CONFIG_DHCPC--*/
		m += '<tr><td style="width:40%">' +
			'<input name=mode type=radio onClick=wanModeChg(2,opWorkMode) ' + ((wm == 2) ? 'id=$wan_mode value=2' : '') + '>' + wdk.msg('Dynamic IP Address') + '</input>' +
			'</td><td>' +
			wdk.msg('Obtain an IP address automatically from your service provider.') +
			'</td></tr>';
		/*--endif--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(1,opWorkMode) ' + ((wm == 1) ? 'id=$wan_mode value=1' : '') + '>' + wdk.msg('Static IP') + '</input>' +
			'</td><td>' +
			wdk.msg('Use a static IP address. Your service provider gives a static IP address to access Internet services.') +
			'</td></tr>';
	}
	/*--ifdef CONFIG_OP_3G--*/
	else if (op == 7) {
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(9,opWorkMode) ' + ((wm == 9) ? 'id=$wan_mode value=9' : '') + '>' + wdk.msg('3G Router') + '</input>' +
			'</td><td>' +
			wdk.msg('WAN is connected to the Internet using a USB-based 3G/UMTS/HSDPA dongle.') +
			'</td></tr>';
	}
	/*--endif--*/
	else {
		/*--ifdef CONFIG_DHCPC--*/
		m += '<tr><td style="width:40%">' +
			'<input name=mode type=radio onClick=wanModeChg(2,opWorkMode) ' + ((wm == 2) ? 'id=$wan_mode value=2' : '') + '>' + wdk.msg('Dynamic IP Address') + '</input>' +
			'</td><td>' +
			wdk.msg('Obtain an IP address automatically from your service provider.') +
			'</td></tr>';
		/*--endif--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(1,opWorkMode) ' + ((wm == 1) ? 'id=$wan_mode value=1' : '') + '>' + wdk.msg('Static IP') + '</input>' +
			'</td><td>' +
			wdk.msg('Use a static IP address. Your service provider gives a static IP address to access Internet services.') +
			'</td></tr>';
		/*--ifdef CONFIG_PPPOE--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(3,opWorkMode) ' + ((wm == 3) ? 'id=$wan_mode value=3' : '') + '>' + wdk.msg('PPPOE') + '</input>' +
			'</td><td>' +
			wdk.msg('PPP over Ethernet is a common connection method used for xDSL.') +
			'</td></tr>';
		/*--endif--*/
		/*--ifdef CONFIG_PPTP--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(4,opWorkMode) ' + ((wm == 4) ? 'id=$wan_mode value=4' : '') + '>' + wdk.msg('PPTP') + '</input>' +
			'</td><td>' +
			wdk.msg('PPP Tunneling Protocol can support multi-protocol Virtual Private Networks (VPN).') +
			'</td></tr>';
		/*--endif--*/
		/*--ifdef CONFIG_L2TP--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(5,opWorkMode) ' + ((wm == 5) ? 'id=$wan_mode value=5' : '') + '>' + wdk.msg('L2TP') + '</input>' +
			'</td><td>' +
			wdk.msg('Layer 2 Tunneling Protocol can support multi-protocol Virtual Private Networks (VPN).') +
			'</td></tr>';
		/*--endif--*/
		/*--ifdef CONFIG_BPA--*/
		m += '<tr><td>' +
			'<input name=mode type=radio onClick=wanModeChg(6,opWorkMode) ' + ((wm == 6) ? 'id=$wan_mode value=6' : '') + '>' + wdk.msg('BigPond') + '</input>' +
			'</td><td>' +
			wdk.msg('Australia ISP service.') +
			'</td></tr>';
		/*--endif--*/
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
	/*--ifdef CONFIG_LINUX--*/
	TD += 20;
	/*--endif--*/
	/*--ifdef CONFIG_FPGA--*/
	TD *= 2;
	/*--endif--*/
	var D = 2;
	var td = TD + (t ? t : 0) + (d ? d : 0);
	var rd = td - ((d) ? d : D);
	if (wdk)
		wdk.redirect_restore(td, rd);
	else
		alert("please include wdk.js");
}
