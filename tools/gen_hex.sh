BIN=$1
HEX=$2

if [ "$BIN" = "" -o "$HEX" = "" ];then
	echo "$0: bad args"
	exit 1
fi

perl -pe 'BEGIN{$\="\n";$/=\1};$_=unpack("H*",$_)' $BIN > $HEX
