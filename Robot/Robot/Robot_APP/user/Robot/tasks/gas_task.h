#ifndef _GAS_TAST_H_
#define _GAS_TAST_H_

//wkxxxx  Global rigister address defines
#define 	WK2204_GENA     0X00
#define 	WK2204_GRST     0X01
#define		WK2204_GMUT     0X02
#define 	WK2204_GIER     0X10
#define 	WK2204_GIFR     0X11
#define 	WK2204_GPDIR    0X21
#define 	WK2204_GPDAT    0X31
#define 	WK2204_GPORT    0//	/wkxxxx  Global rigister of PORT
//wkxxxx  slave uarts  rigister address defines

#define 	WK2204_SPAGE    0X03
//PAGE0
#define 	WK2204_SCR      0X04
#define 	WK2204_LCR      0X05
#define 	WK2204_FCR      0X06
#define 	WK2204_SIER     0X07
#define 	WK2204_SIFR     0X08
#define 	WK2204_TFCNT    0X09
#define 	WK2204_RFCNT    0X0A
#define 	WK2204_FSR      0X0B
#define 	WK2204_LSR      0X0C
#define 	WK2204_FDAT     0X0D
#define 	WK2204_FWCR     0X0D
#define 	WK2204_RS485    0X0F
//PAGE1
#define 	WK2204_BAUD1    0X04
#define 	WK2204_BAUD0    0X05
#define 	WK2204_PRES     0X06
#define 	WK2204_RFTL     0X07
#define 	WK2204_TFTL     0X08
#define 	WK2204_FWTH     0X09
#define 	WK2204_FWTL     0X0A
#define 	WK2204_XON1     0X0B
#define 	WK2204_XOFF1    0X0C
#define 	WK2204_SADR     0X0D
#define 	WK2204_SAEN     0X0D
#define 	WK2204_RRSDLY   0X0F

//WK串口拓展芯片的寄存器的位定义
//wkxxx register bit defines
// GENA
#define 	WK2204_UT4EN	0x08
#define 	WK2204_UT3EN	0x04
#define 	WK2204_UT2EN	0x02
#define 	WK2204_UT1EN	0x01
//GRST
#define 	WK2204_UT4SLEEP	0x80
#define 	WK2204_UT3SLEEP	0x40
#define 	WK2204_UT2SLEEP	0x20
#define 	WK2204_UT1SLEEP	0x10
#define 	WK2204_UT4RST	0x08
#define 	WK2204_UT3RST	0x04
#define 	WK2204_UT2RST	0x02
#define 	WK2204_UT1RST	0x01
//GIER
#define 	WK2204_UT4IE	0x08
#define 	WK2204_UT3IE	0x04
#define 	WK2204_UT2IE	0x02
#define 	WK2204_UT1IE	0x01
//GIFR
#define 	WK2204_UT4INT	0x08
#define 	WK2204_UT3INT	0x04
#define 	WK2204_UT2INT	0x02
#define 	WK2204_UT1INT	0x01
//SPAGE
#define 	WK2204_SPAGE0	0x00
#define 	WK2204_SPAGE1   0x01
//SCR
#define 	WK2204_SLEEPEN	0x04
#define 	WK2204_TXEN     0x02
#define 	WK2204_RXEN     0x01
//LCR
#define 	WK2204_BREAK	0x20
#define 	WK2204_IREN     0x10
#define 	WK2204_PAEN     0x08
#define 	WK2204_PAM1     0x04
#define 	WK2204_PAM0     0x02
#define 	WK2204_STPL     0x01
//FCR
//SIER
#define 	WK2204_FERR_IEN      0x80
#define 	WK2204_CTS_IEN       0x40
#define 	WK2204_RTS_IEN       0x20
#define 	WK2204_XOFF_IEN      0x10
#define 	WK2204_TFEMPTY_IEN   0x08
#define 	WK2204_TFTRIG_IEN    0x04
#define 	WK2204_RXOUT_IEN     0x02
#define 	WK2204_RFTRIG_IEN    0x01
//SIFR
#define 	WK2204_FERR_INT      0x80
#define 	WK2204_CTS_INT       0x40
#define 	WK2204_RTS_INT       0x20
#define 	WK2204_XOFF_INT      0x10
#define 	WK2204_TFEMPTY_INT   0x08
#define 	WK2204_TFTRIG_INT    0x04
#define 	WK2204_RXOVT_INT     0x02
#define 	WK2204_RFTRIG_INT    0x01


//TFCNT
//RFCNT
//FSR
#define 	WK2204_RFOE     0x80
#define 	WK2204_RFBI     0x40
#define 	WK2204_RFFE     0x20
#define 	WK2204_RFPE     0x10
#define 	WK2204_RDAT     0x08
#define 	WK2204_TDAT     0x04
#define 	WK2204_TFULL    0x02
#define 	WK2204_TBUSY    0x01
//LSR
#define 	WK2204_OE       0x08
#define 	WK2204_BI       0x04
#define 	WK2204_FE       0x02
#define 	WK2XXX_PE       0x01
//FWCR
//RS485

#define   	B600     600
#define 	B1200	 1200
#define 	B2400	 2400
#define 	B4800    4800
#define 	B9600	 9600
#define 	B19200	 19200
#define 	B38400	 38400
#define 	B1800	 1800
#define 	B3600	 3600
#define		B7200	 7200
#define 	B14400	 14400
#define  	B28800	 28800
#define	  	B57600	 57600
#define		B115200	 115200
#define		B230400	 230400

#define		GAS_PORT	1

#define xWk2204CmdStart()         	GPIO_ResetBits(GPIOE, GPIO_Pin_11)  /* 命令开始 */
#define xEndWk2204Cmd()            	GPIO_SetBits(GPIOE, GPIO_Pin_11)    /* 命令结束*/

#endif

