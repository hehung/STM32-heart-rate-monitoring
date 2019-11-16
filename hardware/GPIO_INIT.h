#ifndef __GPIO_INIT_H
#define __GPIO_INIT_H
#include "sys.h"

//宏定义
#define BEEP PBout(15)			//定义了蜂鸣器的引脚
#define LED  PBout(12)			//定义了led灯的引脚
#define KEY1 GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)			//KEY1按键输入引脚
#define KEY2 GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_10)			//KEY2按键输入引脚

//函数声明
void LED_Init(void);			//LED引脚初始化
void BEEP_Init(void);			//蜂鸣器引脚初始化
void LED_ON(void);				//打开LED
void LED_OFF(void);				//关闭LED
void BEEP_ON(void);				//打开蜂鸣器
void BEEP_OFF(void);			//关闭蜂鸣器
void KEY_Init(void);			//按键引脚初始化

#endif
