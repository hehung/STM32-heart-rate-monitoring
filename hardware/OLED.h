#ifndef _OLED_H
#define _OLED_H

#define	Brightness	0xCF 
#define X_WIDTH 	128
#define Y_WIDTH 	64

void OLED_WrDat(unsigned char dat);// -- 向OLED屏写数据
void OLED_WrCmd(unsigned char cmd);// -- 向OLED屏写命令
void OLED_Init(void);// -- OLED屏初始化程序，此函数应在操作屏幕之前最先调用

void OLED_Fill(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char dot);
void OLED_Clear(void);

void OLED_Refresh_Gram(void);		   
void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void OLED_ShowHz(unsigned char x,unsigned char y,unsigned char chr,unsigned char mode);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size,unsigned char mode);
void OLED_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size);
void OLED_ShowString(unsigned char x,unsigned char y,const unsigned char *p,unsigned char size);	


#endif
