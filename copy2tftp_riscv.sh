# Copy dtb, uImage and freertos.img to folder of tftp server with user name
TFTP_PATH=$1
FREERTOS_IMG=freertos.img

cd bin
pwd
username=$(whoami)
newfilename1=dtb_${username}
newfilename2=uImage_${username}
newfilename3=freertos_${username}
echo "The new file name of dtb is ${newfilename1}"
echo "The new file name of uImage is ${newfilename2}"
echo "The new file name of ${FREERTOS_IMG} is ${newfilename3}"
cp dtb ${newfilename1}
cp uImage ${newfilename2}

if [ -f ${FREERTOS_IMG} ]; then
	cp ${FREERTOS_IMG} ${newfilename3}
	# Move these three files (with user name) to tftp server's folder
	echo "Copy ${newfilename1}, ${newfilename2} and ${newfilename3} to ${TFTP_PATH}"
	mv ${newfilename1} ${TFTP_PATH}
	mv ${newfilename2} ${TFTP_PATH}
	mv ${newfilename3} ${TFTP_PATH}
else
	# Move these two files (with user name) to tftp server's folder
	echo "Copy ${newfilename1} and ${newfilename2} to ${TFTP_PATH}"
	mv ${newfilename1} ${TFTP_PATH}
	mv ${newfilename2} ${TFTP_PATH}
	rm -f ${TFTP_PATH}/${newfilename3}
fi
