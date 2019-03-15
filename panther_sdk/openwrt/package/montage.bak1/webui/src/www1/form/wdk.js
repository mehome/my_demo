/*
	WEB SDK functions
*/
var wdk = (function() {
	var CGIFILE = '/cli.cgi';
	var LOAD_ATTRIBUTE = 'load';
	var SAVE_ATTRIBUTE = 'save';
	var W3C_COMPLIANCE = false;
	var RESP_SPLIT_WRAPPER = 'echo '
	var RESP_SPLIT = debugMode() ? 'AAA' : 'ZZ';
	var CMD_SPLIT = '%;'
	var STATE_DEFAULT = 0;
	var STATE_ONGOING = 1;
	var STATE_SUCCESS = 2;
	var STATE_FAIL = 3;
	var STATE_STARTUPGRADE = 4;
	var COMMIT_STATE_CMD = CGIFILE + '?cmd=web_commit state';
	var UPLOAD_STATE_CMD = CGIFILE + '?cmd=upload state ';
	var LANG_COOKIE_NAME = 'language';
	var UPLOAD_POLL_INR = 1000;

	/** An associate array which contains the cdb variable information. */
	var cdb = new Object();
	var tmp_status = STATE_DEFAULT;
	var thisPage = window.location.toString().replace(RegExp("/.*\//"), '').replace(RegExp("/\?.*/"), '');
	var resut_ret = '';
	var repost = '';
	/** Record help filename. e.g. "help_tc.htm" */
	var _help_file = '';
	/** Record translation filename. e.g. "message_tc.js" */
	var _translation_file = '';
	/** Indicate whether stop idle timer */
	var _stop_timeout = false;

	/**
	 * To update "cdb" associate array.
	 * @method cdbUpdate
	 * @param {object} cmds - cdb variale name array
	 * @param {string} resp - return string from CGI
	 * @param {string} split - A string as delimiter
	 * @return {undefined}
	 */
	function cdbUpdate(cmds, resp, split) {
		var s = resp.split(split + "\n");

		for (var i = 0; i < cmds.length; i++) {
			var n = cmds[i];
			if (s[i].match(/^!ERR/m)) s[i] = '';
			cdb[n] = {
				nval: '',
				oval: '',
				name: n
			};
			var ma = s[i].match(/^.+\n/mg)
			if (ma) {
				if (ma.length > 1)
					cdb[n].oval = ma;
				else {
					s[i] = s[i].replace(/\n/g, '');
					cdb[n].oval = s[i];
				}
			} else {
				s[i] = s[i].replace(/\n/g, '');
				cdb[n].oval = s[i];
			}
			cdb[n].nval = cdb[n].oval;
		}
	}

	/**
	 * To load the cdb variables and store into "cdb" associate array.
	 * @method cdbLoad
	 * @param {object} cdbNa - the variable name array which you want load
	 * @param {object} [cb] - if this parameter exist, cdbLoad function would request cdb value
	 *                        asynchronously and execute this function after server response.
	 * @return {undefined}
	 */
	function cdbLoad(cdbNa, cb) {
		var cmds = new Array();
		var respSplit;
		if (debugMode())
			respSplit = RESP_SPLIT;
		else
			respSplit = RESP_SPLIT + String(Math.random()).substr(3, 3);
		var cmdSplit = CMD_SPLIT + RESP_SPLIT_WRAPPER + respSplit + CMD_SPLIT;

		// remove null element in IE(version<9) when array include extra last commas
		while (cdbNa[cdbNa.length - 1] == null)
			cdbNa.length--; //cdbNa.pop();

		for (var i = 0; i < cdbNa.length; i++) {
			var name = cdbNa[i];
			var re = /(\$\w+)(\d{1,2})(-(\d{1,2}))?$/;
			if (re.test(name)) /* cdb arrary */ {
				var st = Number(RegExp.$2);
				var end;
				if (RegExp.$4)
					end = Number(RegExp.$4);
				else
					end = st;
				var n = RegExp.$1;
				for (var k = st; k <= end; k++) {
					name = n + k;
					cmds.push(name);
				}
			} else
				cmds.push(name);
		}
		// make a splite mark between each command
		var ca = cmds.join(cmdSplit);
		if (cb) {
			cliCmd(ca, function(resp) {
				cdbUpdate(cmds, resp, respSplit);
				cb();
			});
		} else {
			var v = cliCmd(ca);
			cdbUpdate(cmds, v, respSplit);
		}
	}

	/**
	 * Return the current value of cdb variable.
	 * @method cdbVal
	 * @param {string} n - the name of cdb variable
	 * @return {string} the value of cdb variable
	 */
	function cdbVal(n) {
		try {
			return cdb[n].nval;
		} catch (e) {
			alert("don't exist variable(" + n + ")");
			return '';
		}
	}

	/**
	 * Create a cdb variable into "cdb" associate array.
	 * @method cdbNew
	 * @param {string} n - the name of cdb variable
	 * @param {string} v - the value of cdb variable
	 * @return {undefined}
	 */
	function cdbNew(n, v) {
		cdb[n] = {
			nval: v,
			oval: v,
			name: n
		};
	}

	/**
	 * Set the new value for a cdb variable.
	 * @method cdbSet
	 * @param {string} n - the name of cdb variable
	 * @param {string} v - the value of cdb variable
	 * @return {undefined}
	 */
	function cdbSet(n, v) {
		cdb[n].nval = v;
	}

	/**
	 * Check whether the value of cdb variable has changed.
	 * @method cdbChg
	 * @param {string} n - the name of cdb variable
	 * @return {boolean} changed or not changed
	 */
	function cdbChg(n) {
		return (cdb[n].oval != cdb[n].nval);
	}

	/**
	 * Delete a member from the group which includes the same prefix name.
	 * These cdb variables be distinguished by appending index number.
	 * @method cdbDel
	 * @param {string} n - the name of cdb variable
	 * @param {number} max - the number of group member
	 * @return {undefined}
	 */
	function cdbDel(n, max) {
		var id = n.replace(/\d+$/, '');
		var i = Number(n.match(/\d+$/));
		for (var k = i; k < max; k++)
			cdbSet(id + k, cdbVal(id + (k + 1)));
		cdbSet(id + k, '');
	}

	/**
	 * Return the member number of the same pre-name cdb variables.
	 * @method cdbLen
	 * @param {string} pn - the prefix part of cdb name
	 * @param {number} max - the default maximum number we loaded in webpage.
	 * @return {number} the member number
	 */
	function cdbLen(pn, max) {
		for (var i = 1; i <= max; i++)
			if ('' == cdbVal(pn + i)) break;
		return i - 1;
	}

	/**
	 * Return the value in array format even the original format is string.
	 * You can get how many rows by "array.length".
	 * @method cdbValA
	 * @param {string} name - the name of cdb variable
	 * @return {object} the value of cdb variable in the array format
	 */
	function cdbValA(name) {
		try {
			if (typeof(cdb[name].oval) == 'string') {
				var s = [];
				if (cdb[name].oval != '')
					s[0] = cdb[name].oval;
				return s;
			} else
				return cdb[name].oval;
		} catch (e) {
			var s = [];
			return s;
		}
	}

	/**
	 * Check whether webpage is in debug mode
	 * @method debugMode
	 * @return {number} 1 or 0
	 */
	function debugMode() {
		/*WSIM_BEGIN*/
		return 1;
		/*WSIM_END*/
		return 0;
	}

	/**
	 * Return all child elements which tag name is "input", "select" or "td".
	 * @method findChildNode
	 * @param {object} t - the parent element
	 * @return {object} the child elements in array format
	 */
	function findChildNode(t) {
		var x = t.getElementsByTagName('input');
		var s = t.getElementsByTagName('select');
		var td = t.getElementsByTagName('td');
		var y = [];
		for (var i = 0; i < x.length; i++) y.push(x[i]);
		for (var i = 0; i < s.length; i++) y.push(s[i]);
		for (var i = 0; i < td.length; i++) y.push(td[i]);
		return y;
	}

	/**
	 * Fill the form entry.
	 * @method fill_form_entry
	 * @param {object} t - the target element
	 * @param {string} v - the filled value
	 * @param {number} [def] - if it be defined, fill default value
	 * @return {undefined}
	 */
	function fill_form_entry(t, v, def) {
		if ((t.tagName == 'INPUT' || t.tagName == 'SELECT' || t.tagName == 'TD')) {
			_fill_field(t, v, def);
			return;
		} else if (typeof(v) == 'string') {
			var mf = v.split('&');
			if (mf.length && mf.length > 1) {
				var e = findChildNode(t);
				if (e.length)
					for (var j = 0; j < mf.length; j++)
						_load_field(e, mf[j]);
			}
			return;
		}
		alert("wrong argument of fill_form_entry function");
	}

	function fill_survey_entry(t, v, def) {
		if ((t.tagName == 'INPUT' || t.tagName == 'SELECT' || t.tagName == 'TD')) {
			_fill_field(t, v, def);
			return;
		} else if (typeof(v) == 'string') {
			var mf = v.split('&');
			if (mf[1].search("ssid=") != -1) {
				mf[1] = v.substring(v.indexOf("&ssid=") + 1, v.indexOf("&ch="));
			}
			if (mf.length && mf.length > 1) {
				var e = findChildNode(t);
				if (e.length)
					for (var j = 0; j < mf.length; j++) {
                         if (j == 1)
                             _load_field_ssid(e,mf[j]);
                         else
						     _load_field(e, mf[j]);
                    }
			}
			return;
		}
		alert("wrong argument of fill_form_survey_entry function");
	}

	/**
	 * Return the name of function in string format.
	 * @method getFuncName
	 * @param {object} func - the target function
	 * @return {string} the name of function
	 */
	function getFuncName(func) {
		// Find zero or more non-paren chars after the function start
		var pat = new RegExp("function ([^(]*)");
		return pat.exec(func + "")[1];
	}

	/**
	 * Add a custom function to fill value when calling init_form().
	 * @method addLoadAttributeByID
	 * @param {string} id - the name of cdb variable
	 * @param {object} func - call this function when calling init_form()
	 * @return {object} target element
	 */
	function addLoadAttributeByID(id, func) {
		var node = cdbGetElement(id);
		node.setAttribute(LOAD_ATTRIBUTE, getFuncName(func));
		return node;
	}

	/**
	 * Add a custom function to load value when calling save_form().
	 * @method addSaveAttributeByID
	 * @param {string} id - the name of cdb variable
	 * @param {object} func - call this function when calling save_form()
	 * @return {object} target element
	 */
	function addSaveAttributeByID(id, func) {
		var node = cdbGetElement(id);
		node.setAttribute(SAVE_ATTRIBUTE, getFuncName(func));
		return node;
	}

	/**
	 * To search a function by function name then executing it.
	 * @method executeFunctionByName
	 * @param {string} functionName - the target function name
	 * @param {object} context - search in this context
	 * @return the return of executed function
	 */
	function executeFunctionByName(functionName, context) {
		var args = Array.prototype.slice.call(arguments, 2); //get arguments from third
		var namespaces = functionName.split(".");
		var func = namespaces.pop();
		for (var i = 0; i < namespaces.length; i++) {
			context = context[namespaces[i]];
		}
		return context[func].apply(context, args);
	}

	/**
	 * To set a value into a TD element.
	 * @method setTD
	 * @param {object} t - the target element
	 * @param {string} v - the filled value
	 * @return {undefined}
	 */
	function setTD(t, v) {
		if ('textContent' in document.body)
			t.textContent = v;
		else //IE8 and below
			t.innerText = v;
	}

	/**
	 * To get the value from a TD element.
	 * @method getTD
	 * @param {object} t - the target element
	 * @return {string} the value of the TD element
	 */
	function getTD(t) {
		if ('textContent' in document.body)
			return t.textContent;
		else //IE8 and below
			return t.innerText;
	}

	/**
	 * To fill value for "td", "select" or "input" (type is "radio", "select", checkbox").
	 * @method _fill_field
	 * @param {object} t - the target element
	 * @param {string} v - the filled value
	 * @param {number} [def] - if it be defined, fill default value
	 * @return {undefined}
	 */
	function _fill_field(t, v, def) {
		if (t.attributes[LOAD_ATTRIBUTE]) {
			executeFunctionByName(t.attributes[LOAD_ATTRIBUTE].value, window, t, v);
			return;
		}

		if (t.tagName == 'TD')
			setTD(t, v);
		else if (t.tagName == 'SELECT') {
			for (var k = 0; k < t.options.length; k++) {
				t.options[k].selected = (t.options[k].value == v);
				if (def) t.options[k].defaultSelected = t.options[k].selected;
			}
		} else if (t.type == 'radio') { //t.tagName == 'INPUT';type=='radio'
			t.checked = (t.value == v);
			if (def) t.defaultChecked = t.checked;
		} else if (t.type == 'checkbox') { //t.tagName == 'INPUT';type=='checkbox'
			t.checked = Number(v);
			if (def) t.defaultChecked = t.checked;
		} else { //t.tagName == 'INPUT';type=='text' or 'password'
			t.value = v;
			if (def && t.defaultValue != 'undefined')
				t.defaultValue = t.value;
		}
	}

	/**
	 * Search one element by name or headers then fill the value of element.
	 * @method _load_field
	 * @param {object} childs - all candidate elements
	 * @param {string} set - a string like "name=value", if matching will fill the value
	 * @return {undefined}
	 */
	function _load_field(childs, set) {
		var n = set.split('=');
		if (n.length < 1) return;
		var e = getByName(childs, n[0]);
		if (e)
			_fill_field(e, n[1]);
		var e = getByHeaders(childs, n[0]);
		if (e)
			_fill_field(e, n[1]);
	}

    function _load_field_ssid(childs, set) {
        var n = set.split('=');
        n[1] = set.substring(5);
        if (n.length < 1) return;
        var e = getByName(childs, n[0]);
		if (e)
		    _fill_field(e, n[1]);
		var e = getByHeaders(e, n[0]);
		if (e)
		    _fill_field(e, n[1]);

    }

	/**
	 * Load the current value of those variable listed in cdbLoad([variables list]) and
	 * fill the corresponding form entry.
	 * @method init_form
	 * @return {undefined}
	 */
	function init_form() {
		for (var key in cdb) {
			var t, v;
			var n = cdb[key].name;
			t = cdbGetElement(n);
			if (!t) continue;
			v = cdbVal(n);
			fill_form_entry(t, v, 1);
		}
	}

	/**
	 * To scan all "cdb" elements and update "cdb" associate array.
	 * @method presubmit_form
	 * @return {number} 0
	 */
	function presubmit_form() {
		for (var key in cdb) {
			var c = cdb[key];
			var t = cdbGetElement(c.name);
			if (!t) continue;
			if (t.attributes[SAVE_ATTRIBUTE]) {
				c.nval = executeFunctionByName(t.attributes[SAVE_ATTRIBUTE].value, window, t);
				continue;
			}
			try {
				if (t.tagName == 'INPUT' || t.tagName == 'SELECT') {
					if (t.type == 'checkbox')
						c.nval = Number(t.checked);
					else
						c.nval = t.value;
				}
			} catch (ex) {
				alert("[" + c.name + "] " + msg("not exist"));
				continue;
			}
		}
		return 0;
	}

	/**
	 * Scan the form entry listed in cdbLoad([variables list]), compare the current
	 * value whether it is update and submit those changed entry back to server in the
	 * cli command format.
	 * @method save_form
	 * @param {number} ltime - a fixed delay time
	 * @param {number} not_show_result - whether calling showResultBG() and showResult()
	 * @return {undefined}
	 */
	function save_form(ltime, not_show_result) {
		var cmds = '';
		var cnum = 0;

		/*if (presubmit_form()) return;*/
		for (var key in cdb) {
			if (cdbChg(key)) {
				cmds += cdb[key].name + '=' + '\'' + cdb[key].nval + '\'' + CMD_SPLIT;
				cnum++;
			}
		}
		if (cmds == '') {
			if (!not_show_result)
				alert(msg('The settings are not changed!!'));
			return;
		} else {
			cnum++;
			if (!not_show_result)
				cmds += 'web_commit';
			else
				cmds += 'save';
		}
		var post = "cnum=" + cnum;
		post += "&cmd=" + encodeURI(cmds).replace(/&/g, '%26')
		repost = post;
		/*WSIM_BEGIN*/
        alert(post);
		alert(decodeURI(post));
		/*WSIM_END*/
        /*
		if (!not_show_result)
			showResultBG();
        */
		if (debugMode())
			resut_ret = 'debug_return';
		else
			resut_ret = httpReq(CGIFILE, post);
        /*
		saveFormAutoRetry();
		if (!not_show_result)
			showResult(ltime);
        */
	}

	/**
	 * To check the state of DUT.
	 * @method checkState
	 * @param {string} url - the CGI path and command
	 * @param {object} cb - the callback function
	 * @return {undefined}
	 */
	function checkState(url, cb) {
		if (debugMode()) {
			tmp_status = STATE_SUCCESS;
			setTimeout(function() {
				cb();
			}, 1000);
		} else {
			ashttpReq(url, '', function(txt) {
				tmp_status = txt;
				cb();
			});
		}
	}

	/**
	 * To check commit state and show the result at success and fail case.
	 * @method uploadCheckComplete
	 * @param {object} id - a session ID
	 * @param {object} success - the callback function at success case
	 * @param {object} fail - the callback function at fail case
	 * @return {undefined}
	 */
	function uploadCheckComplete(id, success, fail, firmwareUp) {
		var upgradeState = (firmwareUp) ? STATE_STARTUPGRADE : STATE_SUCCESS;
		
		if (tmp_status <= STATE_ONGOING) {
			setTimeout(function() {
				checkState(UPLOAD_STATE_CMD + id, function() {
					uploadCheckComplete(id, success, fail, firmwareUp);
				});
			}, UPLOAD_POLL_INR);
		} else if (tmp_status == upgradeState) {
			success();
		} else if (tmp_status == STATE_FAIL) {
			fail();
		}
		return;
	}

	/**
	 * To bring up the state checking about uploading file.
	 * @method showUpgradeResult
	 * @param {object} id - a session ID
	 * @param {object} success - the callback function at success case
	 * @param {object} fail - the callback function at fail case
	 * @return {undefined}
	 */
	function showUpgradeResult(id, success, fail, firmwareUp) {
		checkState(UPLOAD_STATE_CMD + id, function() {
			uploadCheckComplete(id, success, fail, firmwareUp);
		});
	}

	/**
	 * To display the "resultbg" div element.
	 * @method showResultBG
	 * @return {undefined}
	 */
	function showResultBG() {
        /*
		getById("resultbg").style.display = "block";
		getById("resultbg").style.visibility = "visible";
        */
	}

	/**
	 * hideResultBG Hide the "resultbg" div element
	 * @method hideResultBG
	 * @return {undefined}
	 */
	function hideResultBG() {
        /*
		getById("resultbg").style.display = "none";
		getById("resultbg").style.visibility = "hidden";
        */
	}

	/**
	 * To change the content of the "resultbg" div element.
	 * @method showResult
	 * @param {number} ltime - a fixed delay time
	 * @return {undefined}
	 */
	function showResult(ltime) {
		if (typeof ltime != 'undefined') {
			tmp_status = STATE_SUCCESS;
			setTimeout(function() {
				saveFormResult();
			}, ltime);
		} else
			checkState(COMMIT_STATE_CMD, function() {
				saveFormResult();
			});
	}

	/**
	 * To check commit state.
	 * @method cdbCheckComplete
	 * @return {boolean} true or false
	 */
	function cdbCheckComplete() {
		if (tmp_status <= STATE_ONGOING) {
			checkState(COMMIT_STATE_CMD, function() {
				saveFormResult();
			});
			return false;
		} else if (tmp_status == STATE_SUCCESS) {
			return true;
		} else if (tmp_status == STATE_FAIL) {
			alert("commit state not yet implemented");
			return true;
		}
		return false;
	}

	/**
	 * To re-submit the updated cdb value if it failed before.
	 * @method saveFormAutoRetry
	 * @return {undefined}
	 */
	function saveFormAutoRetry() {
		var retry = 10; //retry 10 times
		while (!resut_ret && retry > 0) {
			if (repost)
				resut_ret = httpReq(CGIFILE, repost);
			retry--;
		}
		if (retry == 0)
			resut_ret = msg("Error");
	}

	/**
	 * To update the content of "result" div element.
	 * @method saveFormResult
	 * @return {undefined}
	 */
	function saveFormResult() {
		var result = cgiCode(resut_ret);

		if (!cdbCheckComplete())
			return;

		if (result) {
			var m = result + '<br><br><input class=button name=apply_button value=' + msg('Continue') + ' type=button onclick="wdk.goThisPage()">';
			getById("result").innerHTML = '<center><b>' + m + '</b></center><br>';
		}
		return;
	}

	/**
	 * To update the private variable "resut_ret".
	 * @method set_resut_ret
	 * @return {undefined}
	 */
	function set_resut_ret(rc) {
		resut_ret = rc;
		return;
	}

	/**
	 * To check the return of submitting.
	 * @method cgiCode
	 * @param {string} rc - the return of CGI
	 * @return {string} success or error message
	 */
	function cgiCode(rc) {
		if (rc.match(/!ERR/))
			return msg("Error");
		else
			return msg("Success");
	}

	/**
	 * To redirect page according to "thisPage" variable.
	 * @method goThisPage
	 * @return {undefined}
	 */
	function goThisPage() {
		try {
			window.location.href = thisPage;
		} catch (e) {
			alert(msg("Please click 'Continue' button again!"));
		}
	}

	/**
	 * To remove the prefix '$' if it exists.
	 * @method w3c_id
	 * @param {string} old - the ID of element
	 * @return {string} the processed ID
	 */
	function w3c_id(old) {
		var n = old.replace(/ +/g, '');
		if (n.charAt(0) == '$')
			n = n.substr(1);
		return n;
	}
	/**
	 * To get the element by ID which is cdb variable name.
	 * If 'W3C_COMPLIANCE' is true, start to remove all the char '$' in ID.
	 * And the corresponding html DOM ID should be changed!
	 * @method cdbGetElement
	 * @param {string} id - the ID of target element
	 * @return {object} the target element
	 */
	function cdbGetElement(id) {
		if (W3C_COMPLIANCE) {
			var t = getById(w3c_id(id));
			if (t)
				return t;
		}
		return getById(id);
	}

	/**
	 * To get the element by ID.
	 * @method getById
	 * @param {string} id - the ID of target element
	 * @return {object} the target element
	 */
	function getById(id) {
		return document.getElementById(id);
	}

	/**
	 * To search all elements according to ID.
	 * @method getByHeaders
	 * @param {object} all - all possible elements
	 * @param {string} id - the ID of target element
	 * @return {object}
	 */
	function getByHeaders(all, id) {
		for (var i = 0; i < all.length; i++) {
			if (all[i].headers) {
				if (all[i].headers != 'th_' + id)
					continue;
				else
					return all[i];
			}
		}
		return null;
	}

	/**
	 * To get first element which have the specified name.
	 * @method getByName
	 * @param {object} all - all possible elements
	 * @param {string} name - the name of target element
	 * @return {object}
	 */
	function getByName(all, name) {
		for (var i = 0; i < all.length; i++) {
			if (all[i].name) {
				if (all[i].name != name)
					continue;
				else
					return all[i];
			}
		}
		return null;
	}

	/**
	 * To get all elements which have the specified name.
	 * @method getAllByName
	 * @param {object} all - all possible elements
	 * @param {string} name - the name of target element
	 * @return {object}
	 */
	function getAllByName(all, name) {
		var e = new Array();
		for (var i = 0; i < all.length; i++) {
			if (all[i].name) {
				if (all[i].name != name)
					continue;
				else
					e.push(all[i])
			}
		}
		if (e.length == 0)
			return null;
		else if (e.length == 1)
			return all[0];
		else
			return e;
	}

	/**
	 * Create an XMLHttpRequest object.
	 * @method getHTTPObject
	 * @return {object} an XMLHttpRequest object
	 */
	function getHTTPObject() {
		var xmlHttp;
		if (window.XMLHttpRequest) { // code for IE7+, Firefox, Chrome, Opera, Safari
			try {
				xmlHttp = new XMLHttpRequest();
			} catch (e) {
				xmlHttp = false;
			}
		} else { // code for IE6, IE5
			try {
				xmlHttp = new ActiveXObject("MSXML2.XMLHTTP");
			} catch (e) {
				try {
					xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
				} catch (e) {
					xmlHttp = false;
				}
			}
		}
		if (!xmlHttp) {
			alert(msg("Your browser does not support AJAX!"));
		}
		return xmlHttp;
	}

	/**
	 * Send a request to a server synchronously.
	 * @method httpReq
	 * @param {string} url - the location of the file on the server
	 * @param {string} parm - the content of request
	 * @return {string} null string
	 */
	function httpReq(url, parm) {
		var http = getHTTPObject();
		http.open(((parm == '') ? 'GET' : 'POST'), url, false);
		try {
			http.send(parm);
		} catch (e) {
			console.log("XMLHttpRequest send " + parm + " to " + url + " error:" + e);
		}
		if (debugMode()) {
			if (http.readyState == 4)
				return http.responseText;
		} else {
			if (http.readyState == 4 && http.status == 200)
				return http.responseText;
		}
		return '';
	}

	/**
	 * Send a request to a server asynchronously.
	 * @method ashttpReq
	 * @param {string} url - the location of the file on the server
	 * @param {string} parm - the content of request
	 * @param {object} cb - callback function which execute at request finished
	 * @return {string} null string
	 */
	function ashttpReq(url, parm, cb) {
		var http = getHTTPObject();
		http.onreadystatechange =
			function getresp() {
				if (debugMode()) {
					if (http.readyState == 4)
						cb(http.responseText);
				} else {
					if (http.readyState == 4 && http.status == 200)
						cb(http.responseText);
				}
		}
		http.open(((parm == '') ? 'GET' : 'POST'), url, true);
		try {
			http.send(parm);
		} catch (e) {
			alert("XMLHttpRequest send " + parm + " to " + url + " error:" + e);
		}
		return '';
	}

	/**
	 * The basic routine to communicate with the server's cgi/cli handler, which send
	 * the cli to server and get the response in the return value.
	 * @method cliCmd
	 * @param {string} cmd - the command string
	 * @param {object} cb - the callback function
	 * @return {string} the return of command or null string
	 */
	function cliCmd(cmd, cb) {
		var url;
		var reg;
		//alert("cmd = "+cmd);
		if (debugMode()) {
			var s = new Array();
			var all = httpReq(CGIFILE, '');
			s = cmd.split(CMD_SPLIT);
			//alert('Q => '+s);
			if (!s.length)
				s[0] = cmd;
			var ans = [];
			for (var i = 0; i < s.length; i++) {
				var key;
				key = s[i];
				//escape special characters at regex pattern
				key = key.replace(/[\-\[\]\/\{\}\(\)\*\+\?\.\\\^\$\|]/g, "\\$&");
				if (key == '') break;
				var patt = new RegExp('^' + key + '=(.*)$', 'gm');
				var aa = all.match(patt);
				if (aa) {
					patt = new RegExp('^' + key + '=(.*)$', 'g');
					for (var j = 0; j < aa.length; j++)
						ans.push(aa[j].replace(patt, "$1"));
					//add a null element to generate '\n' in last line for simulating real case
					if (aa.length > 1)
						ans.push('');
				} else
					ans.push('!ERR');

			}
			//alert('A => '+ans);
			if (cb) {
				cb(ans.join("\n"));
				return '';
			} else
				return ans.join("\n");
		} else {
			//url=CGIFILE+'?cnum=1&cmd='+encodeURI(cmd);
			url = CGIFILE + '?cmd=' + cmd;
			if (cb) {
				ashttpReq(url, '', cb);
				return '';
			} else
				return httpReq(url, '');
		}
	}

	/**
	 * To get a cookie value.
	 * @method getCookie
	 * @param {string} c_name - the name of cookie
	 * @return {string} the value of cookie
	 */
	function getCookie(c_name) {
		if (top.document.cookie.length > 0) {
			var c_start = top.document.cookie.indexOf(c_name + "=");
			if (c_start != -1) {
				c_start = c_start + c_name.length + 1;
				var c_end = top.document.cookie.indexOf(";", c_start);
				if (c_end == -1) {
					c_end = top.document.cookie.length;
				}
				return decodeURI(top.document.cookie.substring(c_start, c_end));
			}
		}
		return "";
	}

	/**
	 * To set value into cookie.
	 * @method setCookie
	 * @param {string} c_name - the name of cookie
	 * @param {string} value - the value of cookie
	 * @param {number} [expiredays] - the expire days
	 * @return {undefined}
	 */
	function setCookie(c_name, value, expiredays) {
		var exdate = new Date();
		exdate.setDate(exdate.getDate() + expiredays);
		top.document.cookie = c_name + "=" + encodeURI(value) + ((expiredays == null) ? "" : "; expires=" + exdate.toGMTString());
	}

	/**
	 * Get the current language setting from cli.cgi ($sys_language) and
	 * update the recorded translation and help filename.
	 * @method _langInit
	 * @return {string} the language ID
	 */
	function _langInit() {
		var _lid = cdbVal('$sys_language');
		var lid = _lid.replace(/'/g, "");
		setCookie(LANG_COOKIE_NAME, lid);
		if (lid.match(/!ERR/mg) || lid.match(/^en$/mg))
			_help_file = 'help_en.htm';
		else {
			_help_file = 'help_' + lid + '.htm';
			_translation_file = 'message_' + lid + '.js';
		}

		return lid;
	}

	/**
	 * The output filter of message to display. it try to translate the input string
	 * normally in English and do a mapping to return the corresponding string of
	 * the current language.
	 * @method msg
	 * @param {string} key - the key of message associate array
	 * @return {undefined}
	 */
	function msg(key) {
		var lid = getCookie(LANG_COOKIE_NAME);
		try {
			if (lid.match(/!ERR/mg) || lid.match(/^en$/mg))
				return key;
			else {
				var s = "_msgTable_" + lid + "['" + key + "']";
				var m = eval(s);
				if (!m) {
					/*WSIM_BEGIN*/
					alert('MSG:[' + key + '] not mapping, check file:[' + _msg_file + ']');
					/*WSIM_END*/
					return key;
				}
				return m;
			}
		} catch (e) {
			return key;
		}
	}

	/**
	 * Translate the key(English) by msg(), then calling document.write()
	 * @method putmsg
	 * @param {string} key - an English message also is the key of translation associate array
	 * @return {undefined}
	 */
	function putmsg(key) {
		document.write(msg(key));
	}

	/**
	 * To check whether any cdb variable has changed.
	 * @method is_form_update
	 * @return {boolean} true or false
	 */
	function is_form_update() {
		if (presubmit_form()) return false;
		for (var key in cdb) {
			if (cdbChg(key)) {
				return true;
			}
		}
		return false;
	}

	/**
	 * To show a prompt and reboot DUT, then redirecting page after a specific period.
	 * @method redirect_restore
	 * @param {number} td - total delay time (unit:second)
	 * @param {number} rd - reboot delay time
	 * @return {undefined}
	 */
	function redirect_restore(td, rd) {
		if (td == rd)
			cliCmd('reboot');

		getById("titleupwarng").innerHTML = msg("Rebooting! Please wait ") + td + msg(" seconds");
		if (td == 0)
			top.location.reload(true);
		else
			setTimeout(function() {
				redirect_restore(--td, rd)
			}, 1000);
	}

	/**
	 * To show a prompt, then redirecting page after a specific period.
	 * @method redirect
	 * @param {number} td - total delay time (unit:second)
	 * @return {undefined}
	 */
	function redirect(td) {
		getById("titleupwarng").innerHTML = msg("Processing! Please wait ") + td + msg(" seconds");
		if (td == 0)
			goUrl(thisPage);
		else
			setTimeout(function() {
				redirect(--td)
			}, 1000);
	}

	/**
	 * Set stop_timeout indicator
	 * @method stopTimeout
	 * @return {undefined}
	 */
	function stopTimeout() {
		_stop_timeout = true;
	}

	/**
	 * If a idle timeout happened, show the redirected button on "result" div element.
	 * @method showTimeout
	 * @return {undefined}
	 */
	function showTimeout() {
		if (_stop_timeout)
			return;
		showResultBG();
		var m = msg('Login timeout ! Please login again !') + '<br><br><input class=button name=apply_button value=' + msg('OK') + ' type=button onclick=javascript:onClick=wdk.goThisPage()>';
		getById("result").innerHTML = '<center><b>' + m + '</b></center><br>';
	}

	/**
	 * Get the current idle time setting($sys_idle) and
	 * initialize the watchdog.
	 * @method _idleInit
	 * @return {undefined}
	 */
	function _idleInit() {
		var idle = cdbVal('$sys_idle');
		if (idle && Number(idle))
			setTimeout(function() {
				showTimeout();
			}, 1000 * idle);
	}

	/**
	 * Return "thisPage" variable.
	 * @method getThisPage
	 * @return {string} thisPage
	 */
	function getThisPage() {
		return thisPage;
	}

	/**
	 * Change "thisPage" variable.
	 * @method setThisPage
	 * @param {string} newpage - new this page
	 * @return {undefined}
	 */
	function setThisPage(newpage) {
		thisPage = newpage;
	}

	/**
	 * Return "_help_file" variable.
	 * @method getHelpFile
	 * @return {string} _help_file
	 */
	function getHelpFile() {
		return _help_file;
	}

	/**
	 * Return "_translation_file" variable.
	 * @method getTranslationFile
	 * @return {string} _translation_file
	 */
	function getTranslationFile() {
		return _translation_file;
	}

	return {
		_fill_field: _fill_field,
		_idleInit: _idleInit,
		_langInit: _langInit,
		_load_field: _load_field,
		addLoadAttributeByID: addLoadAttributeByID,
		addSaveAttributeByID: addSaveAttributeByID,
		ashttpReq: ashttpReq,
		cdbCheckComplete: cdbCheckComplete,
		cdbChg: cdbChg,
		cdbDel: cdbDel,
		cdbGetElement: cdbGetElement,
		cdbLen: cdbLen,
		cdbLoad: cdbLoad,
		cdbNew: cdbNew,
		cdbSet: cdbSet,
		cdbUpdate: cdbUpdate,
		cdbVal: cdbVal,
		cdbValA: cdbValA,
		cgiCode: cgiCode,
		checkState: checkState,
		cliCmd: cliCmd,
		debugMode: debugMode,
		executeFunctionByName: executeFunctionByName,
		fill_form_entry: fill_form_entry,
		fill_survey_entry: fill_survey_entry,
		findChildNode: findChildNode,
		getAllByName: getAllByName,
		getByHeaders: getByHeaders,
		getById: getById,
		getByName: getByName,
		getCookie: getCookie,
		getFuncName: getFuncName,
		getHTTPObject: getHTTPObject,
		getHelpFile: getHelpFile,
		getTD: getTD,
		getThisPage: getThisPage,
		getTranslationFile: getTranslationFile,
		goThisPage: goThisPage,
		hideResultBG: hideResultBG,
		httpReq: httpReq,
		init_form: init_form,
		is_form_update: is_form_update,
		msg: msg,
		presubmit_form: presubmit_form,
		putmsg: putmsg,
		redirect: redirect,
		redirect_restore: redirect_restore,
		saveFormAutoRetry: saveFormAutoRetry,
		saveFormResult: saveFormResult,
		save_form: save_form,
		setCookie: setCookie,
		setTD: setTD,
		setThisPage: setThisPage,
		set_resut_ret: set_resut_ret,
		showResult: showResult,
		showResultBG: showResultBG,
		showTimeout: showTimeout,
		showUpgradeResult: showUpgradeResult,
		stopTimeout: stopTimeout,
		uploadCheckComplete: uploadCheckComplete,
		w3c_id: w3c_id
	};
})();
