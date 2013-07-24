#define GPA0CON 		( *((volatile unsigned long *)0xE0200000) )		
#define GPA1CON 		( *((volatile unsigned long *)0xE0200020) )

// UART相关寄存器
#define ULCON0 			( *((volatile unsigned long *)0xE2900000) )		
#define UCON0 			( *((volatile unsigned long *)0xE2900004) )
#define UFCON0 			( *((volatile unsigned long *)0xE2900008) )
#define UMCON0 			( *((volatile unsigned long *)0xE290000C) )
#define UTRSTAT0 		( *((volatile unsigned long *)0xE2900010) )
#define UERSTAT0 		( *((volatile unsigned long *)0xE2900014) )
#define UFSTAT0 		( *((volatile unsigned long *)0xE2900018) )
#define UMSTAT0 		( *((volatile unsigned long *)0xE290001C) )
#define UTXH0 			( *((volatile unsigned long *)0xE2900020) )
#define URXH0 			( *((volatile unsigned long *)0xE2900024) )
#define UBRDIV0 		( *((volatile unsigned long *)0xE2900028) )
#define UDIVSLOT0 		( *((volatile unsigned long *)0xE290002C) )
#define UINTP 			( *((volatile unsigned long *)0xE2900030) )
#define UINTSP 			( *((volatile unsigned long *)0xE2900034) )
#define UINTM 			( *((volatile unsigned long *)0xE2900038) )

#define UART_UBRDIV_VAL		35
#define UART_UDIVSLOT_VAL	0x1

// 初始化串口 
void uart_init()
{
	// 1 配置引脚用于RX/TX功能
	GPA0CON = 0x22222222;
	GPA1CON = 0x2222;

	// 2 设置数据格式等
	// 使能fifo
	UFCON0 = 0x1;
	// 无流控
	UMCON0 = 0x0;
	// 数据位:8, 无校验, 停止位: 1
	ULCON0 = 0x3;
	// 时钟：PCLK，禁止中断，使能UART发送、接收
	UCON0  = 0x5;  				

	// 3 设置波特率
	UBRDIV0 = UART_UBRDIV_VAL;
	UDIVSLOT0 = UART_UDIVSLOT_VAL;
}

// 接收一个字符 
char getc(void)
{
	// 如果RX FIFO空，等待
	while (!(UTRSTAT0 & (1<<0)));
	// 取数据
	return URXH0;                   	
}

// 发送一个字符
void putc(char c)
{
	// 如果TX FIFO满，等待
	while (!(UTRSTAT0 & (1<<2)));
	// 写数据
	UTXH0 = c;                      	
}

/*
 * 打印16进制数
 * 例如：int a = 0x68; puthex(a);输出"68";
 */
int puthex(volatile unsigned char a)
{
    volatile unsigned char cTmp;

    //putchar(a);
    //方法一printf
    //printf("\n%x\n", a);

    //方法二，计算
    cTmp=a>>4;//右移4位，把高位放到低位
    if(cTmp<10)//如果是小于10的数
    {
        cTmp+='0';//以0-9输出
    }
    else
    {
        cTmp+='A'-10;  //大于等于10的就是ABCDEF了。
    }
    putc(cTmp);
    cTmp=a&0x0f;
    if(cTmp<10)
    {
        cTmp+='0';
    }
    else
    {
        cTmp+='A'-10;
    }
    putc(cTmp);

    return 0;
}

int put32bits(volatile unsigned int a)
{
	unsigned char c = 0;
	int i = 0;
	for ( i=3; i>=0; i-- )
	{
		c = ( (a>>(8*i)) & 0xff );
		puthex(c);
	}

	return 0;
}