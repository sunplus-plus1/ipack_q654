#!/bin/bash

CFG=pack.conf

# source OLD config if availabe
if [ -f $CFG ];then
	. $CFG
fi

# import color CONSTANT
if [ -f colors.env ];then
	. colors.env
fi

# Print "(default)" if two strings are the same
# $1: string1 to match
# $2: string2 to match
append_default_str()
{
	[ "$1" = "$2" ] && echo -ne "${RED} (default)${NC}"
	echo ""
}

# CA9 boot or ARM926 boot
def_val=$pf_type
[ "$def_val" = "" ] && def_val=1
echo "* Select IC Type :"
echo "--------------------"
echo -n " [x] Use iBoot (internal ROM): for Zebu or ASIC"; append_default_str $def_val x
echo -n " [s] Use iBoot (external NOR): for EXT_BOOT"; append_default_str $def_val s

echo -n " -> "
read pf_type
[ "$pf_type" = "" ] && pf_type=$def_val
echo "$pf_type"
echo "pf_type=$pf_type" >$CFG

#
# DRAM param
#

pf_num=$pf_type
echo "pf_num=$pf_num" >>$CFG

echo "--------------"
echo "Configuration"
echo "--------------"
cat $CFG

echo ""
echo "Adjust boot image ..."
case "$pf_type" in
	s)
		iboot=
		;;
	x)
		iboot=bootRom.bin.zero
		;;
	*)
		echo "Error: Unknow type!!"
		exit 1
esac

case "$pf_num" in
	s | x)
		xboot=
		;;
	*)
		echo "Error: Unknow number!!"
		exit 1
esac

if [ "$iboot" != "" ];then
	ln -sf $iboot bin/bootRom.bin
else
	rm -f bin/bootRom.bin
fi
if [ "$xboot" != "" ];then
	ln -sf $xboot bin/xboot.img
else
	rm -f bin/xboot.img
fi

echo ""
