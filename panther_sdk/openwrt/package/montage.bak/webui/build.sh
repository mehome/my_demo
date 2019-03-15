#!/bin/bash
. ${TOPDIR}/.config

process_tag()
{
	grep -nr "ifn*def\|endif" $1 > t.tmp
	sed -n "s/\([0-9]*\).*/\1/w t2.txt" t.tmp 
	sed -n "s@.*--ifn*def\s\(CONFIG_.*\)--.*\|.*\(endif\).*@\1\2@w t1.txt" t.tmp 
	sed -n "s@.*--\(ifn*def\)\s.*@\1@w t3.txt" t.tmp 

	num=0
	for w in `cat t3.txt`; do
		defArray[$((num++))]=$w
	done
	num=0
	for w in `cat t1.txt`; do 
		nameArray[$((num++))]=$w
	done
	num=0
	for w in `cat t2.txt`; do
		lineArray[$((num++))]=$w
	done

	echo -n "" >  _sed.cmd
	final=0
	for ((j=0,w=0; w<num; w++)); do
		if [ "${nameArray[w]}" = "endif" ]; then
			continue;
		fi
		for ((k=1,n=$w+1;n<num; n++)); do
			if [ "${nameArray[n]}" != "endif" ]; then
				let k++;
			else
				let	k--;
				if [ $k -lt 0 ]; then
					break;
				fi
				if [ $k -eq 0 ]; then
					if [ "${lineArray[n]}" -lt "$final" ]; then
						break;
					fi
					if [ "${defArray[j]}" = "ifdef" ]; then
						if [ "${!nameArray[w]}" = "" ] ; then
							echo "${lineArray[w]},${lineArray[n]}d" >> _sed.cmd
							final=${lineArray[n]}
						fi
					
					else
						if [ "${defArray[j]}" = "ifndef" ]; then
							if [ "${!nameArray[w]}" != "" ] ; then
								echo "${lineArray[w]},${lineArray[n]}d" >> _sed.cmd 
								final=${lineArray[n]}
							fi
						fi
					fi
					let j++
					break;
				fi	
			fi
		done
	done	
	echo "/WSIM_BEGIN/,/WSIM_END/d" >> _sed.cmd 
	echo "/\/\*WSIM.*/d" >> _sed.cmd
	echo "/--WSIM--.*/d" >> _sed.cmd
	echo "/--\(ifdef\|ifndef\|endif\).*--/d" >> _sed.cmd

	sed -f _sed.cmd $1 > t4.txt

	#Return the content of file after removing undefined code
	cat t4.txt

	rm -f t.tmp t1.txt t2.txt t3.txt t4.txt _sed.cmd
}

page_minification()
{
	TMP=_minification
	if [ "$CONFIG_COMPRESS_HTML_BY_HTMLMINIFIER" == "y" ] && [ ${i: -4} == ".htm" ] ; then
		HANDLED_TOOL=`which html-minifier`
		if [ -x "$HANDLED_TOOL" ]; then
			html-minifier --collapse-whitespace --minify-js --minify-css --remove-comments -o $TMP $1
			mv -f $TMP $1
		fi
	elif [ "$CONFIG_COMPRESS_HTML_BY_PYTHONSLIMMER" == "y" ] && [ ${i: -4} == ".htm" ] ; then
		HANDLED_TOOL=`which slimmer`
		if [ -x "$HANDLED_TOOL" ]; then
			slimmer $1 HTML --output=$TMP
			mv -f $TMP $1
		fi
	elif [ "$CONFIG_COMPRESS_JS_BY_UGLIFYJS" == "y" ] && [ ${i: -3} == ".js" ] ; then
		HANDLED_TOOL=`which uglifyjs`
		if [ -x "$HANDLED_TOOL" ]; then
			uglifyjs $1 -o $TMP
			mv -f $TMP $1
		fi
	elif [ "$CONFIG_COMPRESS_JS_BY_YUICOMPRESSOR" == "y" ] && [ ${i: -3} == ".js" ] ; then
		HANDLED_TOOL=`which yui-compressor`
		if [ -x "$HANDLED_TOOL" ]; then
			yui-compressor $1 --type js --charset utf-8 -o $TMP
			mv -f $TMP $1
		fi
	elif [ "$CONFIG_COMPRESS_CSS_BY_YUICOMPRESSOR" == "y" ] && [ ${i: -4} == ".css" ] ; then
		HANDLED_TOOL=`which yui-compressor`
		if [ -x "$HANDLED_TOOL" ]; then
			yui-compressor $1 --type css --charset utf-8 -o $TMP
			mv -f $TMP $1
		fi
	fi
}
#
## main process
#
find $1 -follow -type f -printf "%P\n" | grep -Ev \.svn\|\..*\.sw[op]\$\|.*\~\|Thumbs\.db | sort > pages_files
for i in `cat pages_files`; do
	[ -d $1/$i ] &&	continue
	process_tag $1/$i > $1/_page.tmp
		page_minification $1/_page.tmp
	cat $1/_page.tmp > $1/$i
done
rm -f ./pages_files
rm -f $1/_page.tmp
