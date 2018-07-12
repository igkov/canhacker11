gcc -s -Os -o canopen.exe canopen.c comn.c
hex2bin.exe -B0x00000000 -S0x00008000 -Oecu11.bin ecu11.hex