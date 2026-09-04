#include "STC15F2K60S2.H"
#include "sys.H"
#include "hall.H"
#include "displayer.H"

code unsigned long SysClock = 11059200;

#ifdef _displayer_H_
code char decode_table[]={0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00, 0x08, 0x40, 0x01, 0x76, 0x38};
                   /*序号： 0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15   */
                   /*显示： 0     1     2     3     4     5     6     7     8     9   （无）  下-   中-   上-    H     L    */
#endef

char ch[8]={0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe1, 0xc3, 0x87};
int sum = 0;
char flag = 0;

void my100ms_callback()
{
	if(flag)
	{
		if(sum == 8) sum = 0;
		LedPrint(ch[sum]);
		sum++;
	}
	else
	{
		LedPrint(0);
		sum = 0;
	}
}

void myhall()
{
	if(GetHallAct() == enumHallGetClose) flag = 1;
	else flag = 0;
}

void main()
{
	DisplayerInit();
	HallInit();
	SetDisplayerArea(0,7);
	Seg7Print(10,10,10,10,10,10,10,10);
	LedPrint(0);
	SetEventCallBack(enumEventSys100mS,my100ms_callback);
	SetEventCallBack(enumEventHall,myhall);
	MySTC_Init();
	while(1)
	{
		MySTC_OS();
	}
}