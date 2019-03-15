var op_work_mode;

function init() {
    wdk.cdbLoad(['$op_work_mode']);
    wdk.cdbLoad(['$wl_channel', '$wl_bss_ssid1', '$wl_2choff', '$wl_bb_mode']);
    op_work_mode = wdk.cdbVal('$op_work_mode');
    
    if (op_work_mode == 1)
        initAPMode();
    else
        initStationMode();
}

function initAPMode() {
    showHide("AP");
    var t = wdk.getById("ap_ssid");
    t.innerHTML = wdk.cdbVal('$wl_bss_ssid1');
    
    t = wdk.getById("ap_networkmode");
    mode = wdk.cdbVal('$wl_bb_mode');
    switch(Number(mode)){
        case 1:
            t.innerHTML = "b";
        break;
        case 3:
            t.innerHTML = "b/g";
        break;
        case 7:
            t.innerHTML = "b/g/n";
        break;
    }
    ch = wdk.cdbVal('$wl_channel');
    t = wdk.getById("ap_channel");
    t.innerHTML = ch;
    
    ch_width = wdk.cdbVal('$wl_2choff');
    t = wdk.getById("ap_channelwidth");
    switch(Number(ch_width)){
        case 0:
            t.innerHTML = "20";
        break;
        case 3:
            t.innerHTML = "20/40";
        break;
    }
}

function initStationMode() {
    showHide("STA");
    setInterval(queryWPA, 2000);
}

function queryWPA() {
    var ret = wdk.cliCmd("query_wpa 1");
    var t = wdk.getById("wpa_status");
    t.innerHTML = ret;
    
    ret = wdk.cliCmd("query_wpa 2");
    t = wdk.getById("ssid");
    t.innerHTML = ret;
    
    ret = wdk.cliCmd("query_wpa 3");
    t = wdk.getById("bssid");
    t.innerHTML = ret;
}
