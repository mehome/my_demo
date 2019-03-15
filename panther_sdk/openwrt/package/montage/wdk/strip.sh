#!/bin/bash
[ -f ${KCONF} ] && {
	. ${KCONF} 2> /dev/null
} || {
	echo "ERR: no kernel config!!"
	return 1
}
[ -f ${ACONF} ] && {
	. ${ACONF} 2> /dev/null
} || {
	echo "ERR: no application config!!"
	return 1
}
stripdir=$1

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

	sed -i -f _sed.cmd $1

	rm -f t.tmp t1.txt t2.txt t3.txt _sed.cmd
}
#
## main process
#
(cd $stripdir; find -follow -type f -printf "%P\n" | grep -Ev \.svn\|\..*\.sw[op]\$\|.*\~\|Thumbs\.db) | sort > pages_files

for i in `cat pages_files`; do
	process_tag $stripdir/$i
done
rm -f pages_files

