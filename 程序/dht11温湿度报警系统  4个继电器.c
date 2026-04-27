#include <reg52.h>	         //调用单片机头文件
#define uchar unsigned char  //无符号字符型 宏定义	变量范围0~255
#define uint  unsigned int	 //无符号整型 宏定义	变量范围0~65535

sbit beep = P1^5;    //蜂鸣器IO口定义
sbit relay1 = P3^4;  //温度上限继电器IO口定义
sbit relay2 = P3^5;  //温度下限继电器IO口定义	     
sbit relay3 = P1^3;  //湿度上限继电器IO口定义
sbit relay4 = P1^4;  //湿度下限继电器IO口定义
uchar flag_en = 1;	 //手动取消报警的变量

uint flag_300ms ;	 //300毫秒的变量	   
 
uchar menu_1;        //设置不同报警参数的变量
uchar t_high = 35,t_low = 10;	//温度上下限报警值 
uchar s_high = 80,s_low = 10;	//湿度上下限报警值

sbit dht11=P2^7;	 //温湿度传感器IO口定义
uchar table_dht11[5]={0};	 //温湿度值 放到这个数组里面

sbit rs=P1^0;	 //寄存器选择信号 H:数据寄存器  	L:指令寄存器
sbit rw=P1^1;	 //寄存器选择信号 H:数据寄存器  	L:指令寄存器
sbit e =P1^2;	 //片选信号   下降沿触发

sbit key1 = P2^0;	    //按键IO口定义
sbit key2 = P2^1;		//按键IO口定义
sbit key3 = P2^2;		//按键IO口定义

/***************************************************************
* 名称 : delay_1ms()
* 功能 : 延时1ms函数
* 输入 : q
* 输出 : 无
****************************************************************/
void delay_1ms(uint q)
{
	uint i,j;
	for(i=0;i<q;i++)
		for(j=0;j<120;j++);
}

/********************************************************************
* 名称 : delay_uint()
* 功能 : 小延时。
* 输入 : 无
* 输出 : 无
***********************************************************************/
void delay_uint(uint q)
{
	while(q--);
}

/********************************************************************
* 名称 : write_com(uchar com)
* 功能 : 1602指令函数
* 输入 : 输入的指令值
* 输出 : 无
***********************************************************************/
void write_com(uchar com)
{
 	rs=0;	     //写指令
	rw=0;	     //对1602写操作
	P0=com;	     //P0口对1602写指令 
	delay_uint(25);
	e=1;		 //e=1使能信号 
	delay_uint(100);   //延时一下等1602完成操作
	e=0;
}

/********************************************************************
* 名称 : write_data(uchar dat)
* 功能 : 1602写数据函数
* 输入 : 需要写入1602的数据
* 输出 : 无
***********************************************************************/
void write_data(uchar dat)
{
 	rs=1;	     //写数据
	rw=0;	     //对1602写操作
	P0=dat;	 	 //P0口对1602写数据 
	delay_uint(25);
	e=1;		 //e=1使能信号 
	delay_uint(100);   //延时一下等1602完成操作
	e=0;	
}

/***********************lcd1602上显示特定的字符****0XDF 度********************/
void write_zifu(uchar hang,uchar add,uchar date)
{
	if(hang==1)   
		write_com(0x80+add);	   //写1602第一行的地址 
	else
		write_com(0x80+0x40+add);  //写1602第二行的地址 
	write_data(date);		  //写数据 
}

/***********************lcd1602上显示两位十进制数************************/
void write_lcd2(uchar hang,uchar add,uint date)
{
	if(hang==1)   
		write_com(0x80+add);	   //写1602第一行的地址 
	else
		write_com(0x80+0x40+add);  //写1602第二行的地址 
	write_data(0x30+date/10%10);	//显示十位数
	write_data(0x30+date%10);   	//显示个位数
}

/***********************lcd1602上显示这字符函数************************/
void write_string(uchar hang,uchar add,uchar *p)
{
	if(hang==1)   
		write_com(0x80+add);	   //写1602第一行的地址 
	else
		write_com(0x80+0x40+add);   //写1602第二行的地址 
	while(1)														 
	{
		if(*p == '\0')  break;	//\0字符串的结尾标志  break结束while循环 结束写字符
		write_data(*p);	    //写数据 
		p++;	            //指针地址加1  
	}	
}

/***********************lcd1602初始化设置************************/
void init_1602()	//lcd1602初始化
{
	write_com(0x38);  //显示模式设置：16×2显示，5×7点阵，8位数据接口 不检测忙信号
	write_com(0x0c);  //开显示 不显示光标
	write_com(0x06);  //当写一个字符是，地址指针加 1
 	write_string(1,0,"Wd:00  H00  L00  ");	//初始化1602显示 
	write_string(2,0,"Sd:00% H00% L00% ");	//初始化1602显示 
	write_zifu(1,5,0xdf);	//显示温度单位度		
	write_zifu(1,10,0xdf);	//显示温度单位度		
	write_zifu(1,15,0xdf);	//显示温度单位度		
	write_lcd2(1,3,table_dht11[2]);	   //显示温度
	write_lcd2(2,3,table_dht11[0]);   //显示湿度	
	write_lcd2(1,8,t_high);	  //显示温度上限
	write_lcd2(2,8,s_high);   //显示湿度上限		
	write_lcd2(1,13,t_low);	  //显示温度下限
	write_lcd2(2,13,s_low);   //显示湿度下限	
}

/********************独立按键程序*****************/
uchar key_can;	 //按键值

void key()	 //独立按键程序
{
	key_can = 0;                //按键值还原成0
 	if(key1 == 0 || key2 == 0 || key3 == 0)		//有按键按下 
	{
		delay_1ms(1);	     	//按键延时消抖动
		if(key1 == 0)		//确认是按键按下
			key_can = 1; 	//得到按键值 
		if(key2 == 0)		//确认是按键按下
			key_can = 2; 	//得到按键值 
		if(key3 == 0)		//确认是按键按下
			key_can = 3; 	//得到按键值 
		beep = 0;			//蜂鸣器叫一声
		delay_1ms(50);
		beep = 1;
	}
}
 
/****************按键显示函数***************/
void key_with()
{
	if(key_can == 1)	   //设置键
	{
		menu_1 ++;
		if(menu_1 > 4)
		{
			menu_1 = 0;	  //menu_1 = 0 退出设置了，在正常显示界面下
			init_1602();  //lcd1602初始化显示 			
		}
	}
	if(menu_1 == 1)			//设置温度上限报警值 
	{
		if(key_can == 2)   //加键 
		{
			t_high ++ ;		//设置温度上限报警值 加1 	
			if(t_high > 99)
				t_high = 99;
		}
		if(key_can == 3)
		{
			t_high -- ;		//设置温度上限报警值 减1 	
			if(t_high <= t_low)
				t_high = t_low + 1;	 //限制温度上限不能低于温度下限
		}
		write_lcd2(1,8,t_high);	   //显示上限报警值
		write_com(0x80+8);         //将光标移动到第1行第8位
		write_com(0x0f);           //显示光标并且闪烁	
	}	
	if(menu_1 == 2)			//设置温度下限报警值 
	{
		if(key_can == 2)	//加键
		{
			t_low ++ ;			//设置温度下限报警值 加1 
			if(t_low >= t_high)
				t_low = t_high - 1;	  //限制温度下限不能高于温度上限
		}
		if(key_can == 3)			 
		{
			t_low -- ;			//设置温度下限报警值 减1 
			if(t_low <= 1)
				t_low = 1;
		}
		write_lcd2(1,13,t_low);   //显示温度下限报警值	
		write_com(0x80+13);         //将光标移动到第1行第13位
		write_com(0x0f);          //显示光标并且闪烁	
	}
	if(menu_1 == 3)			//设置湿度上限报警值
	{
		if(key_can == 2)	//加键
		{
			s_high ++ ;		//设置湿度上限报警值加1 	
			if(s_high > 99)
				s_high = 99;
		}
		if(key_can == 3)
		{
			s_high -- ;		//设置湿度上限报警值减1 	
			if(s_high <= s_low)
				s_high = s_low + 1;	   //限制湿度上限不能低于湿度下限
		}
		write_lcd2(2,8,s_high);	   //显示湿度上限报警值
		write_com(0x80+0x40+8);    //将光标移动到第2行第8位
		write_com(0x0f);           //显示光标并且闪烁	
	}	
	if(menu_1 == 4)			//设置湿度下限报警值
	{
		if(key_can == 2)	//加键
		{
			s_low ++ ;			//设置湿度下限报警值 加1 
			if(s_low >= s_high)
				s_low = s_high - 1;	  //限制湿度下限不能高于湿度上限
		}
		if(key_can == 3)	//减键		
		{
			s_low -- ;			 //设置湿度下限报警值 减1 
			if(s_low <= 1)
				s_low = 1;
		}
		write_lcd2(2,13,s_low);	     //显示湿度下限报警值
		write_com(0x80+0x40+13);     //将光标移动到第2行第13位
		write_com(0x0f);             //显示光标并且闪烁	
	}	
} 

/*****************读温湿度传感器程序****************/
void dht11_dis()
{
	uchar i,j;			 
	dht11 = 0;	   //DHT11端口复位，发出起始信号 
	delay_1ms(18);
	dht11 = 1;
	delay_uint(4);	  
	if(dht11 == 0)	  //判断是否响应    
	{
		while(dht11 == 0);	  //等待低电平时间过完
		while(dht11 == 1);	  //等待高电平时间过完
		for(i=0;i<5;i++)
		{
			for(j=0;j<8;j++)
			{
				table_dht11[i] <<= 1;  //数据左移一位低位自动补0
				while(dht11 == 0);  //等待低电平时间过完
				delay_uint(3);	    //数据1的高电平时间 
				if(dht11 == 1)
				{
					table_dht11[i] |= 0x01;
					while(dht11 == 1);	  //等待高电平时间过完
				}			
			}			
		}			
	}
	dht11 = 1;	 //释放总线 
}
 
/****************报警函数***************/
void clock_h_l()
{
	if((table_dht11[2] >= t_high))	    //温度大于等于上限
	{			 
		relay1 = 0;		  //打开继电器
	}
	else 
	{
		relay1 = 1;		  //关闭继电器
	}

	if((table_dht11[2] <= t_low))	   //温度小于等于下限
	{			 
		relay2 = 0;		  //打开继电器
	}
	else 
	{
		relay2 = 1;		  //关闭继电器
	}

	if((table_dht11[0] >= s_high))	   //湿度大于等于上限
	{
		relay3 = 0;		  //打开继电器
	}else 
	{
		relay3 = 1;		  //关闭继电器
	}

	if((table_dht11[0] <= s_low))	  //湿度小于等于下限
	{
		relay4 = 0;		  //打开继电器
	}else 
	{	
		relay4 = 1;		  //关闭继电器
	}

	if((relay1 == 0) || (relay2 == 0) || (relay3 == 0) || (relay4 == 0))
	{
		if(flag_en == 1)
			beep = ~beep; 	  //蜂鸣器报警
	}
	else 
	{
		beep = 1;
		flag_en = 1;
	}
	
}


/******************主程序**********************/	   
void main()
{		
	init_1602();	//lcd1602初始化
	while(1)
	{
		flag_300ms ++;
		if(flag_300ms >= 300)    //300毫秒执行一次里面的程序 
		{
			flag_300ms = 0;
			dht11_dis();	        //先读出温湿度的值
			write_lcd2(2,3,table_dht11[0]);   //显示湿度	
			write_lcd2(1,3,table_dht11[2]);	   //显示温度
			clock_h_l();   //报警函数
		}
		key();					//按键程序 
		key_with();			    //设置报警温度	
		if(menu_1 == 0)
		{	
			if(key_can == 3)
			{
				flag_en = 0;   //手动取消报警
				beep = 1;      //关闭蜂鸣器
			}
		}
 		delay_1ms(1);
	}
}

