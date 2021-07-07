#$B2ZMEM  bin/$XBOOT       $ZMEM_HEX     0x0       0x0001000             $DXTOR # 4KB

# $1: input file
# $2: output file    not used
# $3: offset         not used
# $4: start addr
# $5: DXTOR			not used
 dec2hex(){
     printf "0x%x" $1
}
 
start_addr=$(dec2hex $4)

printf "[bin2zmem]input file: $1 \t" 
printf "  start addr: $start_addr \n"

hexdump -v -e '1/4 "%08x\n"' $1 > temp.hex

python ./tools/bin2zmem/bin2zmem_ddr4.py temp.hex $start_addr

rm -rf temp.hex

