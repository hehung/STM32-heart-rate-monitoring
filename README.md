# 前言
　　该文主要讲述了以及STM32的一个心率传感装置的制作过程和最终实现的效果，这是我在大四毕业的时候帮别人做的一个毕业设计的小作品，最初的文章是发表在电子发烧友上面的，现转载github。原链接请点击此处：[点我跳转](http://bbs.elecfans.com/forum.php?mod=viewthread&tid=1600343&page=1#pid7932932 "点我跳转")
  
# 介绍

　　最近做了一个机遇STM32的心率监测装置，使用了一块oled屏幕进行检测到的心率的波形的显示。
心率监测传感器使用的传感器是pulse sensor，是一款比较便宜的传感器。显示屏采用的是128X64的oled显示器。

　　首先放上成品图，不然你们是没有兴趣看的。^_^
  ![心率动态显示](https://github.com/hehung/STM32-heart-rate-monitoring/blob/master/img/Heart_rate_oscillogram.gif?raw=true "心率动态显示")
  
  ![心率数据](https://github.com/hehung/STM32-heart-rate-monitoring/blob/master/img/Heart_rate_data.gif?raw=true "心率数据")
  
# 开发环境与开发器件
　　开发环境：  
　　1、win10 +MDK keil5.24 
　　２、ST-LINK下载器（不一定需要这个，J-LINK和串口下载都是可以的）

　　我是用的是STM32F103C8T6最小系统板，价格比较便宜并且功能足够强大。
　　元器件有：
　　==>pulse sensor心率传感器
　　==>IIC OLED显示屏
　　==>按键2个
　　==>LED1个
　　==>蜂鸣器1个
  
# 电路设计
　　在硬件结构方面还是比较简单的，连线比较简单，就是在软件方面花费了我一些时间，算法处理和波形系那是还是花了一些时间采写好的。
　　放上电路连接图：
  ![电路图](https://github.com/hehung/STM32-heart-rate-monitoring/blob/master/img/Schematic.jpg?raw=true "电路图")
  
# 软件实现
## OLED
　　OLED方面我采用的是IIC操作的OLED，因为IIC操作简单，只需要两根线（我直接使用的PA7和PA8）。使用了软件IIC，直接移植的正点原子的官方例程的IIC，OLED显示也是移植的正点原子的程序，只是做了改进，可以显示汉字（正点原子的官方例程中OLED程序不能显示汉字，我对代码进行升级，可以显示汉字），还增加了自动在数字前面补0的功能。
  
　　由于程序比较大，这里只放了重点的代码，在后面也提供源码。
  
  ```c
//OLED波形显示函数
void OLED_Waveform_display(void)
{
        int i;        
        u8 n;
                  
        IF(waveform_flag == 1)
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
                                if(myabs((int)waveform-(int)waveform[i-1])>1)
                                {
                                        if(waveform > waveform[i-1])
                                        {
                                                for(n=waveform[i-1];n<waveform;n++)
                                                {
                                                        OLED_DrawPoint(i,n,1);                //在相应的像素点上打印
                                                }
                                        }else
                                        {
                                                for(n=waveform;n<waveform[i-1];n++)
                                                {
                                                        OLED_DrawPoint(i,n,1);                //在相应的像素点上打印
                                                }
                                        }
                                        
                                }                        
                        }
                        OLED_DrawPoint(i,waveform,1);                //在相应的像素点上打印
                        #endif
                }
                OLED_Refresh_Gram();                                                //更新显示到OLED
        }
}
```
## pulse sensor心率传感器
　　pulse sensor心率传感器采用ADC进行数据采集，使用到了ADC0（PA0）。通过配置ADC引脚完成ADC的采集，通过实验，可以正常的采集到数据。当人的手指放上之后用示波器可以看到心跳的电压变化。
### 操作说明书
　　说明书密码（xinghuidianzi）：
  Pulse Sensor使用说明书V5.5（密码xinghuidianzi）
  [点此处下载](https://raw.githubusercontent.com/hehung/STM32-heart-rate-monitoring/master/doc/Pulse%20Sensor%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%E4%B9%A6V5.5%EF%BC%88%E5%AF%86%E7%A0%81xinghuidianzi%EF%BC%89.pdf?token=AIHJEAQXAKGJ7R3PJBQMJBS52AKGM "点此处下载")

### 心率监测算法实现
　　心率算法就是检测心率波形的峰值，检测到  有60000ms 除以两个峰值时间（ms）就是心率值，说起来比较简单，但是操作起来有各种干扰，通过官方的例程上进行修改，最终成功测出心率值，还是比较准确的
  ```c
//心率采集与计算处理
void HeartRate_deal(void)
{
        unsigned int runningTotal;
        u8 i;

        Num = sampleCounter - lastBeattime;                         //监控最后一次节拍后的时间，以避免噪声
                
        //找到脉冲波的波峰和波谷
        if(Signal < thresh && Num > (IBI/5)*3)        //为了避免需要等待3/5个IBI的时间
        {       
                if(Signal < T)
                {                                                        //T是阈值
                        T = Signal;                         //跟踪脉搏波的最低点，改变阈值
                }
        }
        if(Signal > thresh && Signal > P)                //采样值大于阈值并且采样值大于峰值
        {          
                P = Signal;                             //P是峰值，改变峰值
        } 
        //现在开始寻找心跳节拍
        if (Num > 250)                                //避免高频噪声
        {                                   
                if ((Signal > thresh) && (Pulse == false) && (Num > (IBI/5)*3))
                {        
                        Pulse = true;                               //当有脉冲的时候就设置脉冲信号。
//                                LED_ON();                                                                        //打开LED，表示已经有脉冲了
                        IBI = sampleCounter - lastBeatTime;         //测量节拍的ms级的时间
                        lastBeatTime = sampleCounter;               //记录下一个脉冲的时间。
                        if(secondBeat)                        //如果这是第二个节拍，如果secondBeat == TRUE，表示是第二个节拍
                        {                        
                                secondBeat = false;                  //清除secondBeat节拍标志
                                for(i=0; i<=9; i++)                //在启动时，种子的运行总数得到一个实现的BPM。
                                {             
                                        rate = IBI;                      
                                }
                        }
                        if(firstBeat)                        //如果这是第一次发现节拍，如果firstBeat == TRUE。
                        {                         
                                firstBeat = false;                   //清除firstBeat标志
                                secondBeat = true;                   //设置secongBeat标志
                                return;                              //IBI值是不可靠的，所以放弃它。
                        }   
                        //保留最后10个IBI值的运行总数。
                        runningTotal = 0;                  //清除runningTotal变量

                        for(i=0; i<=8; i++)                                //转换数据到rate数组中
                        {                
                                rate = rate[i+1];                  //去掉旧的的IBI值。 
                                runningTotal += rate;              //添加9个以前的老的IBI值。
                        }

                        rate[9] = IBI;                          //将最新的IBI添加到速率数组中。
                        runningTotal += rate[9];                //添加最新的IBI到runningTotal。
                        runningTotal /= 10;                     //平均最后10个IBI值。
                        BPM = 60000/runningTotal;               //一分钟有多少拍。即心率BPM
                        QS = true;                              //设置量化自我标志Quantified Self标志
                        //在这个ISR（中断服务程序）中，QS标志没有被清除。
                }                      
        }

        if (Signal < thresh && Pulse == true)                //当值下降时，节拍就结束了。
        {   
//                        LED_OFF();                                                                //关闭LED
                Pulse = false;                         //重设脉冲标记，这样方便下一次的计数
                amp = P - T;                           //得到脉冲波的振幅。
                thresh = amp/2 + T;                    //设置thresh为振幅的50%。
                P = thresh;                            //重新设置下一个时间
                T = thresh;
        }

        if (Num > 2500)                                //如果2.5秒过去了还没有节拍
        {                           
                thresh = 512;                          //设置默认阈值
                P = 512;                               //设置默认P值
                T = 512;                               //设置默认T值
                lastBeatTime = sampleCounter;          //把最后的节拍跟上来。       
                firstBeat = true;                      //设置firstBeat为true方便下一次处理
                secondBeat = false;                    
        }
}
```


　　:fa-chevron-right:我将采集工作与算法处理都放在了定时器里面，初始化了一个2ms的定时器中断，这个工作都是每2ms进行一次

　　LED、按键和蜂鸣器就不用多说了，很简单，蜂鸣器需要一个三极管进行信号放大，不然的话是驱动不了的。

# 作品操作
　　最后说一下我所完成的作品的操作方式：
　　　　1、按下按键1，可以进行界面的切换，在界面1的时候回显示西女排的数据值，ADC值等信息。
　　　　2、在界面2的时候就是显示波形，在人的手没有放上的时候一般不会显示，手放上之后就会显示心率（人的手放上之后，芯片会有一段时间的空白期(采集到的数据为0)，我就是根据这段时间来判断时候有手放上）
　　　　3、当采集到的心率值不在规定的范围之内时蜂鸣器就会报警
　　　　4、按下按键2之后蜂鸣器停止报警，知道再次检测到人手放在传感器上面。

# 成品展示
 ![测试](https://github.com/hehung/STM32-heart-rate-monitoring/blob/master/img/Test.jpg?raw=true "测试")
  
