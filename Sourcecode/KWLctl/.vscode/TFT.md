http://misc.ws/2013/11/08/touch-screen-shield-for-arduino-uno/
https://forum.arduino.cc/index.php?topic=403424.0


Read Registers on MCUFRIEND UNO shield
controllers either read as single 16-bit
e.g. the ID is at readReg(0)
or as a sequence of 8-bit values
in special locations (first is dummy)

reg(0x0000) 00 00	ID: ILI9320, ILI9325, ILI9335, ...
reg(0x0004) 00 00 80 00	Manufacturer ID
reg(0x0009) 00 00 61 00 00	Status Register
reg(0x000A) 00 08	Get Power Mode
reg(0x000C) 00 06	Get Pixel Format
reg(0x0061) 61 61	RDID1 HX8347-G
reg(0x0062) 62 62	RDID2 HX8347-G
reg(0x0063) 63 63	RDID3 HX8347-G
reg(0x0064) 64 64	RDID1 HX8347-A
reg(0x0065) 65 65	RDID2 HX8347-A
reg(0x0066) 66 66	RDID3 HX8347-A
reg(0x0067) 67 67	RDID Himax HX8347-A
reg(0x0070) 00 6F	Panel Himax HX8347-A
reg(0x00A1) 00 00 00 00 00	RD_DDB SSD1963
reg(0x00B0) B0 B0	RGB Interface Signal Control
reg(0x00B4) B4 B4	Inversion Control
reg(0x00B6) B6 B6 B6 B6 B6	Display Control
reg(0x00B7) B7 B7	Entry Mode Set
reg(0x00BF) BF BF BF BF BF BF	ILI9481, HX8357-B
reg(0x00C0) C0 C0 C0 C0 C0 C0 C0 C0 C0	Panel Control
reg(0x00C8) C8 C8 C8 C8 C8 C8 C8 C8 C8 C8 C8 C8 C8	GAMMA
reg(0x00CC) CC CC	Panel Control
reg(0x00D0) D0 D0 D0	Power Control
reg(0x00D2) D2 D2 D2 D2 D2	NVM Read
reg(0x00D3) D3 D3 D3 D3	ILI9341, ILI9488
reg(0x00D4) D4 D4 D4 D4	Novatek ID
reg(0x00DA) 00 00	RDID1
reg(0x00DB) 00 80	RDID2
reg(0x00DC) 00 00	RDID3
reg(0x00E0) E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0 E0	GAMMA-P
reg(0x00E1) E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1 E1	GAMMA-N
reg(0x00EF) EF EF EF EF EF EF	ILI9327
reg(0x00F2) F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2	Adjust Control 2
reg(0x00F6) F6 F6 F6 F6	Interface Control