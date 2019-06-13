# Copy dtb and uImage to another with user name
TFTP_PATH=$1

cd bin
pwd
username=$(whoami)
newfilename1=dtb_${username}
newfilename2=uImage_${username}
echo "The new file name of dtb is ${newfilename1}"
echo "The new file name of uImage is ${newfilename2}"
cp dtb ${newfilename1}
cp uImage ${newfilename2}

# Move these two files with user name to TFTP server's folder
echo "Copy ${newfilename1} and ${newfilename2} to ${TFTP_PATH}"
mv ${newfilename1} ${TFTP_PATH}
mv ${newfilename2} ${TFTP_PATH}
