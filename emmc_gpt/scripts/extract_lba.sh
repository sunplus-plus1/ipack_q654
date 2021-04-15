IN=emmc_gpt.bin

i=0 
while [ $i -lt 5 ];
do
	dd if=$IN of=lba$i.bin bs=512 count=1 skip=$i
	i=$((i+1))
done
