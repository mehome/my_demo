// var
var viewState0Title = "Copy the station lists to the clipboard";
var viewState1Title = "Processing... Please wait";
var viewState2Title = "State: 0->No error  -1-> Contain unsupported character -2-> Duplicated station name";
var viewState3Title = "Press OK to commit";
var viewExportTitle = "Copy the station lists to your file";


var viewState2msg = "Only alphaberic, numbers, hyphen, underscore, comma and exclamation(!) are accepted.";
var centerDomIdlist = ['yourRS', 'importWaiting', 'RSNameTable'];
var footerDomIdlist = ['boxCommit', 'boxPreview', 'boxCancel', 'boxTableClose'];

var warnMsg0 = "";
var warnMsg1 = "The input can't be empty";
var warnMsg2 = "The number of station and url pair should be even" + "\n"
"You can examine the new line in the file end.";

var containError = false;
var errorChkDone = false;


function updateState() {
	containError = false;
	errorChkDone = false;
}

// Box View Transition
function viewOpt0() {
	updateState();

	RSNameInnerTbl = wdk.getById('RSNameInnerTable');
	clearTbl(RSNameInnerTbl); // For

	wdk.getById("boxTitle").innerHTML = viewState0Title;

	showElement(centerDomIdlist[0], "inline-block");
	showElement(centerDomIdlist[1], "none");
	showElement(centerDomIdlist[2], "none");
	wdk.getById(centerDomIdlist[0]).value = ""

	showElement(footerDomIdlist[1], "inline-block");
	showElement(footerDomIdlist[2], "inline-block");
	setBtnEnable(footerDomIdlist[1], true);
	setBtnEnable(footerDomIdlist[2], true);

	showElement(footerDomIdlist[3], "none");
	showElement(footerDomIdlist[0], "none");
}

function viewOpt1() {
	wdk.getById("boxTitle").innerHTML = viewState1Title;

	showElement(centerDomIdlist[0], "none");
	showElement(centerDomIdlist[1], "inline-block");
	showElement(centerDomIdlist[2], "none");

	showElement(footerDomIdlist[1], "inline-block");
	showElement(footerDomIdlist[2], "inline-block");
	wdk.getById(footerDomIdlist[1])
	setBtnEnable(footerDomIdlist[1], false);
	setBtnEnable(footerDomIdlist[2], false);

	showElement(footerDomIdlist[3], "none");
	showElement(footerDomIdlist[0], "none");
}

function viewOpt2() {
	wdk.getById("boxTitle").innerHTML = viewState2Title;
	// alert(viewState2msg);

	showElement(centerDomIdlist[0], "none");
	showElement(centerDomIdlist[1], "none");
	wdk.getById("RSNameTable").style.display = "table";

	showElement(footerDomIdlist[1], "none");
	showElement(footerDomIdlist[2], "none");
	showElement(footerDomIdlist[3], "inline-block");
	showElement(footerDomIdlist[0], "none");
}

function viewOpt3() {
	wdk.getById("boxTitle").innerHTML = viewState3Title;
	// alert(viewState2msg);

	showElement(centerDomIdlist[0], "none");
	showElement(centerDomIdlist[1], "none");
	wdk.getById("RSNameTable").style.display = "table";
	clearTbl(wdk.getById('RSNameInnerTable'));
	fillOutTable();

	showElement(footerDomIdlist[0], "inline-block");
	showElement(footerDomIdlist[1], "none");
	showElement(footerDomIdlist[2], "none");
	showElement(footerDomIdlist[3], "none");

}

// Table API
function insertRSNameToTable(rsLineName) {
	RSNameInnerTbl = wdk.getById('RSNameInnerTable');
	var newRow = RSNameInnerTbl.insertRow(-1);
	var errorCode = newRow.insertCell(0);
	var lineNum = newRow.insertCell(1);
	var stnName = newRow.insertCell(2);

	errorCode.width = "10%";
	lineNum.width = "10%";
	stnName.width = "80%";

	lineNum.innerHTML = rsLineName.lineNum;
	stnName.innerHTML = rsLineName.stnName;
	errorCode.innerHTML = rsLineName.errorCode;
}

function clearTbl(ele) {
	if (ele != null) {
		while (ele.firstChild) {
			ele.removeChild(ele.firstChild);
		}
	}
}

function fillOutTable() {
	for (var i = 0; i < stnObjArr.length - 1; i++)
		insertRSNameToTable(stnObjArr[i]);
}

// Open the playlist import window
function setModalWindow(open) {

	var playlistWindow = document.getElementById('playlistWindow');
	playlistWindow.style.display = (open) ? "block" : "none";
	//clear textarea
	document.getElementById("yourRS").value = "";
	viewOpt0();

}

// Extraction for avoiding C&P
function showElement(eleId, type) {
	var ele = wdk.getById(eleId);
	if (ele != null)
		ele.style.display = type;
}

function setBtnEnable(eleId, enable) {
	wdk.getById(eleId).disabled = !enable;
	wdk.getById(eleId).style.background = (enable) ? "#496fc7" : "grey";
}

// Core function
var stnObjArr = [];

function rsLineName(lineNum, stnName, errorCode, url) {
	this.lineNum = lineNum;
	this.stnName = stnName;
	this.errorCode = errorCode;
	this.url = url;
	this.playkey = null;
	this.createPattern = function() {
		return this.stnName + '&' + this.url + '&' + this.playkey + ";";
	}
}

function chkIllegalChar(str) {
	var reg = /[$%^&*+|~=`\[\]:";'<>?,\/@\\\(\)\{\}]/;
	if (reg.test(str)) {
		return true;
	}
	return false;
}

function chkEmptyWhiteSpaceString(input) {
	if (!input || input.length === 0 || /^\s*$/.test(input)) return true;
	else return false;
}

function chkPlaylistDuplicate(stnObjArr, stnName) {
	if (stnObjArr == null || stnName == null) {
		console.log("stnObjArr ==null || stnName ==null");
		return;
	}
	for (i in stnObjArr) {
		if (stnObjArr[i].stnName == stnName) {
			return true;
		}
	}
	return false;
}

function chkTblRSDuplicate(stnName) {
	// Get current playlist in table
	var currentRSList = [];
	for (i in RSList) {
		currentRSList.push(RSList[i].RSName);
	}

	for (i in currentRSList) {
		if (currentRSList[i] == stnName) {
			return true;
		}
	}
	return false;
}

function previewPlaylist() {
	stnObjArr = [];
	errorChkDone = false;

	var yourRS = wdk.getById('yourRS');
	if (yourRS == null) {
		console.log("Fatal error: DOM \'yourRS\' doesn\'t exist");
	} else {

		var tmpArr = [];
		tmpArr = yourRS.value.split("\n");
		if (tmpArr.length == 1 && tmpArr[0] == "") {
			alert(warnMsg1);
			return;
		}
		if (tmpArr.length % 2 == 1) {
			alert(warnMsg2);
			return;
		}

		for (var i = 0; i < tmpArr.length; i += 2) {
			var correct = 0;
			if (chkEmptyWhiteSpaceString(tmpArr[i]) || chkIllegalChar(tmpArr[i]))
				correct = -1;

			if (chkPlaylistDuplicate(stnObjArr, tmpArr[i]) || chkTblRSDuplicate(tmpArr[i]))
				correct = -2;

			var rsLN = new rsLineName(Number(i) + 1, tmpArr[i], correct, tmpArr[i + 1]);
			stnObjArr.push(rsLN);
			if (correct == -1 || correct == -2) {
				insertRSNameToTable(rsLN);
				containError = true;
			}
		}
		var rsLN = new rsLineName(-1, -1, 0, null);
		stnObjArr.push(rsLN);
		errorChkDone = true;
	} // else
	if (errorChkDone) {
		if (containError) {
			viewOpt2();
			// return;
		} else {
			viewOpt3();
			// return;
		}
	}
} // previewPlaylist

function exportPlaylistFunc() {
	setModalWindow(true);
	var tmpStr = wdk.cliCmd("mpc rs load_playlist");
	var stnArr = tmpStr.split(';');
	var outputStr = "";
	for (i in stnArr) {
		var str = stnArr[i].split('&')[0] + "\n" + stnArr[i].split('&')[1];
		outputStr += (str + "\n");
	}
	outputStr = outputStr.substring(0, outputStr.length - 1);
	document.getElementById("yourRS").value = outputStr;
	wdk.getById("boxTitle").innerHTML = viewExportTitle;
	showElement(footerDomIdlist[1], "none");
}
