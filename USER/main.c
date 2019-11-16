//头文件包含
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "myiic.h"
#include "adc.h"
#include "timer.h"
#include "gpio_init.h"

//宏定义
#define	TIMER		2				//定时器定时时间2ms
#define true 		1				//真
#define false 		0				//假
#define HEART_MAX	100				//心率上限
#define HEART_MIN	60				//心率下限
#define HEART_MAX_ERROR		160		//心率的不可到值，超过此值表示传感器出错
#define HEART_MIN_ERROR		40		//心率的不可到值，低于此值表示传感器出错

//函数声明
void ADC_deal(void);				//模数转换函数
void OLED_Main_display(void);		//OLED主显示函数
void OLED_Value_display(void);		//OLED值显示函数
void OLED_Waveform_display(void);	//OLED值显示函数
void KEY_deal(void);				//按键处理函数
void ALARM_deal(void);				//报警处理函数
void HeartRate_deal(void);			//心率值计算处理函数
void Waveform_deal(void);			//波形处理函数
int myabs(int a);					//取绝对值函数

//变量定义
float adcx;							//采集到的心率的原始ADC值，每次手放到传感器上的时候会输出一段时间的0
u8 display_flag = 0;				//显示界面标志  0--显示主界面	1--显示心率界面
u8 alarm = 0;						//警报变量：	0--不报警		1--报警
u8 alarm_en = 0;					//报警使能标志位，在手放上之后才使能

u8 waveform[128] = {0};				//波形采集数组，采集128个点，OLED的宽度为128个像素
u8 waveform_copy[128] = {0};		//波形采集数组，采集128个点，OLED的宽度为128个像素
u8 waveform_flag = 0;				//波形采样时间计数，采样128次之后才一次性显示出来

//心率采集相关变量
int BPM;                   			//脉搏率==就是心率
int Signal;                			//传入的原始数据。
int IBI = 600;             			//节拍间隔，两次节拍之间的时间（ms）。计算：60/IBI(s)=心率（BPM）
unsigned char Pulse = false;     	//脉冲高低标志。当脉波高时为真，低时为假。
unsigned char QS = false;        	//当发现一个节拍时，就变成了真实。
int rate[10];                    	//数组保存最后10个IBI值。
unsigned long sampleCounter = 0;    //用于确定脉冲定时。
unsigned long lastBeatTime = 0;     //用于查找IBI
int P =512;                      	//用于在脉冲波中寻找峰值
int T = 512;                     	//用于在脉冲波中寻找波谷
int thresh = 512;                	//用来寻找瞬间的心跳
int amp = 100;                   	//用于保持脉冲波形的振幅
int Num;
unsigned char firstBeat = true;     //第一个脉冲节拍
unsigned char secondBeat = false;   //第二个脉冲节拍，这两个变量用于确定两个节拍


//主函数
int main(void)
{  
	delay_init();	    	 		//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	 
	KEY_Init();						//开关初始化
	LED_Init();						//LED初始化
	BEEP_Init();					//蜂鸣器初始化
	IIC_Init();   					//IIC初始化
	OLED_Init();					//OLED初始化
	Adc_Init();		  				//ADC初始化	
	 
	TIM3_Int_Init(TIMER*10-1,7199);//定时器初始化。10Khz的计数频率，计数到2000为200ms   
	 
	while(1)
	{	
		switch(display_flag)
		{
			case 0:OLED_Main_display();break;			//主显示界面函数
			case 1:OLED_Value_display();break;			//值显示界面函数
			case 2:OLED_Waveform_display();break;		//波形显示界面函数
		}	
		
		ALARM_deal();
		KEY_deal();		//按键处理函数
	}
}
 
//OLED主显示函数
void OLED_Main_display(void)
{
	u8 i;
	for(i=0;i<6;i++)
	{
		OLED_ShowHz(16+16*i,0,i,1); 		//显示标题
	}
	for(i=6;i<12;i++)
	{
		OLED_ShowHz(16*(i-6),48,i,1); 		//显示作者
	}

	OLED_Refresh_Gram();					//更新显示到OLED
}

//OLED值显示函数
void OLED_Value_display(void)
{
	u8 temp;
	float xiao;
	
	temp = (u8)adcx/1;		//adc取整数部分
	xiao = adcx;			
	xiao -= temp;			//adc取小数部分
	xiao *= 1000;			//adc将小数部分转换成整数,放大一千倍，方便显示
	OLED_ShowString(32,0,">>",16);
	OLED_ShowString(80,0,"<<",16);
	OLED_ShowHz(48,0,12,1);					//数
	OLED_ShowHz(64,0,13,1);					//据
	OLED_ShowHz(0,16,14,1);					//电
	OLED_ShowHz(16,16,15,1);				//压
	OLED_ShowHz(32,16,8,1);					//：
	OLED_ShowNum(40,16,(int)temp,1,16);		//显示采集到的adc的整数部分	
	OLED_ShowChar(48,16,'.',16,1);
	OLED_ShowNum(56,16,(int)xiao,3,16);		//显示采集到的adc值的小数部分
	OLED_ShowString(88,16,"S:",16);
	OLED_ShowNum(104,16,(int)Signal,3,16);	//显示处理后的心率Signal值
	
	OLED_ShowString(0,32,"IBI:",16);
	OLED_ShowNum(32,32,(int)IBI,3,16);		//显示IBI值
	OLED_ShowString(0,48,"BPM:",16);
	OLED_ShowNum(32,48,(int)BPM,3,16);		//显示BPM值
	
	OLED_Refresh_Gram();					//更新显示到OLED
}

//取绝对值函数
int myabs(int a)
{
	if(a<0)
		return -a;
	else 
		return a;
}

//OLED波形显示函数
void OLED_Waveform_display(void)
{
	int i;	
	u8 n;
		  
	if(waveform_flag == 1)
	{
		waveform_flag = 0;
		for(i=127;i>=0;i--)
		{
			for(n=0;n<64;n++)
			{
				OLED_DrawPoint(i,n,0);
			}
			//波形线补点操作，如果不用，可以注释掉,将#if 1改为#if 0
			//引用波形补点函数可以是波形看起来是连续的
			#if 1
			if(i!=0)
			{
				if(myabs((int)waveform[i]-(int)waveform[i-1])>1)
				{
					if(waveform[i] > waveform[i-1])
					{
						for(n=waveform[i-1];n<waveform[i];n++)
						{
							OLED_DrawPoint(i,n,1);		//在相应的像素点上打印
						}
					}else
					{
						for(n=waveform[i];n<waveform[i-1];n++)
						{
							OLED_DrawPoint(i,n,1);		//在相应的像素点上打印
						}
					}
					
				}			
			}
			OLED_DrawPoint(i,waveform[i],1);		//在相应的像素点上打印
			#endif
		}
		OLED_Refresh_Gram();						//更新显示到OLED
	}
}
 
//ADC处理函数
void ADC_deal(void)
{
	u16 temp;
	temp = Get_Adc(ADC_Channel_0);	//采样AD
	adcx = (float)temp*(3.3/4096);				//进行ADC值的处理
	Signal = temp>>2;					//处理传感器的值
}

//按键处理函数
void KEY_deal(void)
{
	static u8 key1_state = 0;	//按键1的状态
	static u8 key2_state = 0;	//按键2的状态
	
	if(KEY1 == 0)
	{
		key1_state = 1;		//表示按键1按下了
	}else if(KEY2 == 0)
	{
		key2_state = 1;		//表示按键2按下了
	}
	
	if(key1_state == 1)		//此按键用来控制显示界面
	{
		if(KEY1 == 1)		//表示已经松手了
		{
			key1_state = 0;
			OLED_Clear(); //初始清屏
			display_flag = (display_flag==0)?1:(display_flag==1?2:0);		//显示界面切换函数
		}
	}
	if(key2_state == 1)		//此按键用来关闭蜂鸣器
	{
		if(KEY2 == 1)		//表示已经松手了
		{
			key2_state = 0;
			alarm = 0;		//关闭报警信号
			alarm_en = 0;	//关闭报警使能
		}
	}
}

//报警处理函数
void ALARM_deal(void)
{
	static u8 alarm_flag = 0;
	if(adcx == 0)		//表明此时已经有手接触到了传感器
	{
		alarm_flag = 1;		//表示有手了，等待传感器初始化完成以后再给信号
	}else
	{
		if(alarm_flag == 1)
		{
			alarm_flag = 0;
			alarm_en = 1;		//使能报警信号
		}
	}
	
	if(alarm_en == 1)		//只有在报警信号使能之后才能开始进行报警处理
	{
		if(display_flag==1 || display_flag==2)	//在数据显示界面和波形显示界面是启动报警功能
		{
			if((BPM>HEART_MAX && BPM<HEART_MAX_ERROR) || (BPM<HEART_MIN && BPM>HEART_MIN_ERROR))		//采集到的心率值超出范围但是在正常人范围之内
			{
				alarm = 1;							//产生报警信号
			}else
			{
				alarm = 0;							//关闭报警信号
			}
		}
		if(alarm == 1)
		{
			LED_ON();								//打开LED
			BEEP_ON();								//打开蜂鸣器
		}else
		{
			LED_OFF();
			BEEP_OFF();
		}
	}else
	{	//关闭报警信号
		LED_OFF();
		BEEP_OFF();
	}
}

//心率采集与计算处理
void HeartRate_deal(void)
{
	unsigned int runningTotal;
	u8 i;

	Num = sampleCounter - lastBeatTime; 			//监控最后一次节拍后的时间，以避免噪声
		
	//找到脉冲波的波峰和波谷
	if(Signal < thresh && Num > (IBI/5)*3)	//为了避免需要等待3/5个IBI的时间
	{       
		if(Signal < T)
		{                        				//T是阈值
			T = Signal;                         //跟踪脉搏波的最低点，改变阈值
		}
	}
	if(Signal > thresh && Signal > P)		//采样值大于阈值并且采样值大于峰值
	{          
		P = Signal;                             //P是峰值，改变峰值
	} 
	//现在开始寻找心跳节拍
	if (Num > 250)				//避免高频噪声
	{                                   
		if ((Signal > thresh) && (Pulse == false) && (Num > (IBI/5)*3))
		{        
			Pulse = true;                               //当有脉冲的时候就设置脉冲信号。
//				LED_ON();									//打开LED，表示已经有脉冲了
			IBI = sampleCounter - lastBeatTime;         //测量节拍的ms级的时间
			lastBeatTime = sampleCounter;               //记录下一个脉冲的时间。
			if(secondBeat)			//如果这是第二个节拍，如果secondBeat == TRUE，表示是第二个节拍
			{                        
				secondBeat = false;                  //清除secondBeat节拍标志
				for(i=0; i<=9; i++)		//在启动时，种子的运行总数得到一个实现的BPM。
				{             
					rate[i] = IBI;                      
				}
			}
			if(firstBeat)			//如果这是第一次发现节拍，如果firstBeat == TRUE。
			{                         
				firstBeat = false;                   //清除firstBeat标志
				secondBeat = true;                   //设置secongBeat标志
				return;                              //IBI值是不可靠的，所以放弃它。
			}   
			//保留最后10个IBI值的运行总数。
			runningTotal = 0;                  //清除runningTotal变量 

			for(i=0; i<=8; i++)				//转换数据到rate数组中
			{                
				rate[i] = rate[i+1];                  //去掉旧的的IBI值。 
				runningTotal += rate[i];              //添加9个以前的老的IBI值。
			}

			rate[9] = IBI;                          //将最新的IBI添加到速率数组中。
			runningTotal += rate[9];                //添加最新的IBI到runningTotal。
			runningTotal /= 10;                     //平均最后10个IBI值。
			BPM = 60000/runningTotal;               //一分钟有多少拍。即心率BPM
			QS = true;                              //设置量化自我标志Quantified Self标志
			//在这个ISR（中断服务程序）中，QS标志没有被清除。
		}                      
	}

	if (Signal < thresh && Pulse == true)		//当值下降时，节拍就结束了。
	{   
//			LED_OFF();								//关闭LED
		Pulse = false;                         //重设脉冲标记，这样方便下一次的计数
		amp = P - T;                           //得到脉冲波的振幅。
		thresh = amp/2 + T;                    //设置thresh为振幅的50%。
		P = thresh;                            //重新设置下一个时间
		T = thresh;
	}

	if (Num > 2500)				//如果2.5秒过去了还没有节拍
	{                           
		thresh = 512;                          //设置默认阈值
		P = 512;                               //设置默认P值
		T = 512;                               //设置默认T值
		lastBeatTime = sampleCounter;          //把最后的节拍跟上来。       
		firstBeat = true;                      //设置firstBeat为true方便下一次处理
		secondBeat = false;                    
	}
}



//波形处理函数，放在定时器中执行，2ms执行一次
void Waveform_deal(void)
{
	u8 temp;
	float xiao;
	static u8 waveSample_times = 0;
	
	int i;
	
	waveSample_times ++;
	if(waveSample_times == 10)
	{
		waveSample_times = 0;
		waveform_flag = 1;		
	
		temp = (u8)adcx/1;		//adc取整数部分
		xiao = adcx;			
		xiao -= temp;			//adc取小数部分
		xiao *= 100;			//adc将小数部分转换成整数，放大一百倍
		
		//下面开始进行数据处理，以便使用液晶显示
		if(xiao<45)				//将xiao的值限制在50-80之间
		{
			xiao = 45;
		}else if(xiao>80)
		{
			xiao = 80;
		}
		xiao -= 50;				
		xiao *= 1.8;		
		xiao = 64-xiao;			//使波形的宽度在0-60之间，OLED的宽度为64
		
		//下面是显示函数的处理，更新最后一个点的坐标显示出来
		waveform_copy[127] = waveform[127];
		for(i=126;i>=0;i--)
		{
			waveform_copy[i] = waveform[i];
			waveform[i] = waveform_copy[i+1];
		}
		waveform[127] = xiao;
	}		
}

//TIM3中断，TIMER（2ms）时间执行一次
void TIM3_IRQHandler(void)   
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) 	//检查指定的TIM中断发生与否:TIM 中断源 
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  	//清除TIMx的中断待处理位:TIM 中断源 
		sampleCounter += TIMER;							//计算CPU运行时间	
		
		ADC_deal();										//ADC处理函数	
		
		//心率值处理函数放在定时器中处理，每TIMER(2ms)处理一次
		HeartRate_deal();	
		
		if(display_flag == 2)//心率图形处理,当图形显示界面时，处理图形数据
		{
			Waveform_deal();		//波形数据处理函数，其显示函数在主函数中执行
		}								
	}
}


