# Copy dtb and uImage to another with user name
TFTP_PATH=$1
NONOS_B_PATH=nonos/Bchip-non-os
NONOS_B_IMG=rom.img

cd bin
pwd
username=$(whoami)
newfilename1=dtb_${username}
newfilename2=uImage_${username}
newfilename3=a926_${username}
echo "The new file name of dtb is ${newfilename1}"
echo "The new file name of uImage is ${newfilename2}"
echo "The new file name of ${NONOS_B_IMG} is ${newfilename3}"
cp dtb ${newfilename1}
cp uImage ${newfilename2}

if [ -f ../../${NONOS_B_PATH}/bin/${NONOS_B_IMG} ]; then
	cp ../../${NONOS_B_PATH}/bin/${NONOS_B_IMG} ${newfilename3}
	# Move these two files with user name to TFTP server's folder
	echo "Copy ${newfilename1}, ${newfilename2} and ${newfilename3} to ${TFTP_PATH}"
	mv ${newfilename1} ${TFTP_PATH}
	mv ${newfilename2} ${TFTP_PATH}
	mv ${newfilename3} ${TFTP_PATH}
else
	# Move these two files with user name to TFTP server's folder
	echo "Copy ${newfilename1} and ${newfilename2} to ${TFTP_PATH}"
	mv ${newfilename1} ${TFTP_PATH}
	mv ${newfilename2} ${TFTP_PATH}
	rm -f ${TFTP_PATH}/${newfilename3}
fi
