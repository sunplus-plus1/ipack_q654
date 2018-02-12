# $1: input image to update

input=$1

if [ ! -f "$input" ];then
	echo "Error: non-exist arg1 $input"
	exit 1
fi

abs_input="`pwd -P`/$input"



pushd `dirname $0` > /dev/null
abs_scriptdir=`pwd`
popd > /dev/null



# Working dir is Caller's dir!
# Cd script's dir

cd $abs_scriptdir
if [ $? -ne 0 ];then
	echo "Error: can't cd script dir"
	exit 1
fi

cp -v $abs_input ./bin
if [ $? -ne 0 ];then
	echo "Error: copy failed"	
	exit 1
fi
