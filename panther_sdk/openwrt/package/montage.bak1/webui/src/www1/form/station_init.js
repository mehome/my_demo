var op_work_mode;
//SECURITY MODE
var DISABLE_SEC_MODE = 0;
var WEP_SEC_MODE = 4;
var WPA_SEC_MODE = 8;
var WPA2_SEC_MODE = 16;
var WPA_WPA2_SEC_MODE = 24;
// WEP SECURITY MODE
var WEP_OPEN_MODE = 1;
var WEP_SHARED_MODE = 2;
var WEP40_CIPHER_MODE = 1;
var WEP104_CIPHER_MODE = 2;
// WPA
var CCMP_CIPHER_MODE = 8;
var TKIP_CIPHER_MODE = 4;
var NONE_CIPHER_MODE = 0;
/************************* 
  '$op_work_mode'       - 1(AP), 9(Station)
  '$wl_bss_ssid1'       - SSID (AP Mode)
 *************************/
function loadCommonCdb() {
    wdk.cdbLoad(['$op_work_mode', '$wl_bss_ssid1']);
}

/************************* 
  '$wl_bss_sec_type1'   - NONE, WEP, WPA, WPA2
  '$wl_bss_cipher1'     - WEP40_CIPHER_MODE, WEP104_CIPHER_MODE for WEP
                          CCMP_CIPHER_MODE, TKIP_CIPHER_MODE for WPA
  '$wl_bss_wep_1key1'   - WEP key (String)
  '$wl_bss_wpa_rekey'   - Group rekey interval (WPA)
  '$wl_wpa_strict_rekey'- Strict rekey
  '$wl_bss_wpa_psk1'    - WPA Key
 *************************/
function loadSecurityCdb() {
    wdk.cdbLoad(['$wl_bss_sec_type1', '$wl_bss_cipher1', '$wl_bss_wep_1key1']);
    wdk.cdbLoad(['$wl_bss_wpa_rekey', '$wl_wpa_strict_rekey', '$wl_bss_wpa_psk1']);
}

/************************* 
  '$wl_ampdu_tx_enable' - AMPDU TX
  '$wl_ampdu_rx_enable' - AMPDU RX
 *************************/
function loadAdvCdb() {
    wdk.cdbLoad(['$wl_ampdu_tx_enable', '$wl_ampdu_rx_enable']);
}


function init() {
    loadCommonCdb();
    loadSecurityCdb();
    loadAdvCdb();
    op_work_mode = wdk.cdbVal('$op_work_mode');
    
    document.getElementById('enable_station').checked = (op_work_mode == 9);

    onChangeEnable();
    selectSecurityOption(wdk.cdbVal('$wl_bss_sec_type1'));
    onChangeSecurity();
    
    setText('wl_bss_ssid1', wdk.cdbVal('$wl_bss_ssid1'));
    document.getElementById('wl_ampdu_tx_enable').checked = (wdk.cdbVal('$wl_ampdu_tx_enable') == 1);
    document.getElementById('wl_ampdu_rx_enable').checked = (wdk.cdbVal('$wl_ampdu_rx_enable') == 1);
}

function onChangeEnable() {
    if (document.getElementById('enable_station').checked == true) {
        showHide('setting', 1);
    } else {
        showHide('setting', 0);
    }
}

function selectSecurityOption(secType) {
    if (secType & WEP_SEC_MODE) {
        setSelectValue('security', WEP_SEC_MODE);
        wep_cipher_type = secType - WEP_SEC_MODE;
        wep_cipher_len  = Number(wdk.cdbVal('$wl_bss_cipher1')) & (WEP40_CIPHER_MODE + WEP104_CIPHER_MODE);
        
        if (wep_cipher_len == 0)
            wep_cipher_len = WEP40_CIPHER_MODE;
        
        document.getElementById('wep_cipher_type' + wep_cipher_type).checked = true;
        document.getElementById('wep_cipher_len' + wep_cipher_len).checked = true;
        setText('wl_bss_key', wdk.cdbVal('$wl_bss_wep_1key1'));
    } else if ((secType & WPA_SEC_MODE) || (secType & WPA2_SEC_MODE)) {
        setSelectValue('security', secType);
        cipher_mode = Number(wdk.cdbVal('$wl_bss_cipher1')) & (CCMP_CIPHER_MODE + TKIP_CIPHER_MODE);
        
        if (cipher_mode == 0)
            cipher_mode = TKIP_CIPHER_MODE;
        document.getElementById('wap_pairwise' + cipher_mode).checked = true;
        setText('wl_bss_key', wdk.cdbVal('$wl_bss_wpa_psk1'));
    } else {
        setSelectValue('security', DISABLE_SEC_MODE);
    }
}

function onChangeSecurity() {
    var security = document.getElementById('security').value;

    switch(Number(security)){
        case WEP_SEC_MODE:
            showWEPUI();
        break;
        case WPA_SEC_MODE:
        case WPA2_SEC_MODE:
        case WPA_WPA2_SEC_MODE:
            showWPAUI();
        break;
        default:
            hideAllSecurityUI();
        break;
    }
}

function showWEPUI() {
    showHide('keytype', 0);
    showHide('wepmode', 1);
    showHide('passphase', 1);
}

function showWPAUI() {
    showHide('wepmode', 0);
    showHide('keytype', 1);
    showHide('passphase', 1);
}

function hideAllSecurityUI() {
    showHide('wepmode', 0);
    showHide('keytype', 0);
    showHide('passphase', 0);
}

function apply() {
    showHide("applyit", 0);
    showHide("wait_apply", 1);
    
    setNewValues();
    setTimeout("commit()", 100)
}

function commit() {
    wdk.save_form(2000);
    setTimeout("checkSiteAvaliable()", 2000);
}

function setNewValues() {
    var security = document.getElementById('security').value;
    switch(Number(security)){
        case WEP_SEC_MODE:
        setWEPValue();
        break;
        case WPA_SEC_MODE:
        case WPA2_SEC_MODE:
        case WPA_WPA2_SEC_MODE:
        setWPAValue();
        break;
        default:
        wdk.cdbSet('$wl_bss_sec_type1', 0);
        break;
    }
    setCommonPartValue();
    setAdvancePartValue();
}

function setWEPValue() {
    //['$wl_bss_sec_type1', '$wl_bss_cipher1', '$wl_bss_wep_1key1']
    var security = Number(document.getElementById('security').value);
    if (!(security & WEP_SEC_MODE))
        return false;
    
    if(document.getElementById('wep_cipher_type1').checked)
        security += Number(WEP_OPEN_MODE);
    else
        security += Number(WEP_SHARED_MODE);
    
    wdk.cdbSet('$wl_bss_sec_type1', security);
    
    var cipher_length = WEP40_CIPHER_MODE;
    if(document.getElementById('wep_cipher_len2').checked)
        cipher_length = WEP104_CIPHER_MODE;
        
    wdk.cdbSet('$wl_bss_cipher1', cipher_length);
    
    wdk.cdbSet('$wl_bss_wep_1key1', document.getElementById('wl_bss_key').value);
}

function setWPAValue() {
    var security = Number(document.getElementById('security').value);
    if (!((security & WPA_SEC_MODE) || (security & WPA2_SEC_MODE)))
        return false;
    
    wdk.cdbSet('$wl_bss_sec_type1', security);
    
    cipher_mode = TKIP_CIPHER_MODE;
    if(document.getElementById('wap_pairwise8').checked)
        cipher_mode = CCMP_CIPHER_MODE;
    wdk.cdbSet('$wl_bss_cipher1', cipher_mode);
    wdk.cdbSet('$wl_bss_wpa_psk1', document.getElementById('wl_bss_key').value);
}

function setCommonPartValue() {
    //['$op_work_mode', '$wl_bss_ssid1']
    wdk.cdbSet('$op_work_mode', 9);
    wdk.cdbSet('$wl_bss_ssid1', document.getElementById('wl_bss_ssid1').value);
}

function setAdvancePartValue() {
    //['$wl_ampdu_tx_enable', '$wl_ampdu_rx_enable']
    if (document.getElementById('wl_ampdu_tx_enable').checked)
        wdk.cdbSet('$wl_ampdu_tx_enable', 1);
    else
        wdk.cdbSet('$wl_ampdu_tx_enable', 0);

    if (document.getElementById('wl_ampdu_rx_enable').checked)
        wdk.cdbSet('$wl_ampdu_rx_enable', 1);
    else
        wdk.cdbSet('$wl_ampdu_rx_enable', 0);
}

function checkSiteAvaliable() {
    var img = document.getElementById('check_online');
    img.onload = function()
    {
        document.location.href = "http://192.168.25.1/form/index.html";
    };
    img.onerror = function()
    {
        setTimeout("checkSiteAvaliable()", 1000);
    };
    img.src = "online_check.png";
}