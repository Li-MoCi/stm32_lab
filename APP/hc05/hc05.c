#include "SysTick.h"
#include "usart.h"
#include "usart3.h"
#include "hc05.h"
#include "led.h"
#include "string.h"
#include "math.h"


#define HC05_KEY PAout(4)    
#define HC05_LED PAin(15)    


u8 HC05_Init(void)
{
    u8 retry=10,t;         
    u8 temp=1;
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);    //使能PORTA

    // 先将KEY脚拉高，确保进入AT模式
    HC05_KEY = 1;
    delay_ms(100); // 延时确保进入AT模式

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;              // 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;           //输入上拉
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       //IO口速度为50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  //根据设定参数初始化GPIOA15
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;               // 端口配置
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;        //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       //IO口速度为50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  //根据设定参数初始化GPIOA4

    GPIO_SetBits(GPIOA,GPIO_Pin_4);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
    HC05_LED=1; 
    
    //初始化串口3为:9600,与蓝牙模块一致
    USART3_Init(9600);

    while(retry--)
    {
        HC05_KEY = 1; // ??AT??
        delay_ms(100); // ??????AT??
        
        USART3_RX_STA = 0; // ??????
        u3_printf("AT\r\n"); // ??AT????
        
        for(t=0; t<20; t++) // ????,??100ms
        {
            if(USART3_RX_STA & 0X8000) break;
            delay_ms(5);
        }
        
        HC05_KEY = 0; // ??AT??
        
        if(USART3_RX_STA & 0X8000) // ?????
        {
            temp = USART3_RX_STA & 0X7FFF; // ??????
            USART3_RX_BUF[temp] = 0; // ????????
            
            // ??????"OK"??
            if(strstr((char*)USART3_RX_BUF, "OK") != NULL)
            {
                temp = 0; // ?????
                break;
            }
        }
        delay_ms(100);
    }
    
    if(retry == 0) temp = 1; // ????
    return temp;
}

// ??HC05?????
// ???:0,??;1,??;0XFF,????.
u8 HC05_Get_Role(void)
{
    u8 retry = 5;
    u8 temp = 0xFF;
    u8 t;  // ???????????
    char *p;  // ???????????
    
    while(retry--)
    {
        HC05_KEY = 1; // ??AT??
        delay_ms(100);
        
        USART3_RX_STA = 0;
        u3_printf("AT+ROLE?\r\n"); // ????
        
        for(t=0; t<20; t++) // ????,??200ms
        {
            if(USART3_RX_STA & 0X8000) break;
            delay_ms(10);
        }
        
        HC05_KEY = 0; // ??AT??
        
        if(USART3_RX_STA & 0X8000) // ?????
        {
            temp = USART3_RX_STA & 0X7FFF;
            USART3_RX_BUF[temp] = 0; // ????????
            
            // ???? "+ROLE:x"
            p = strstr((char*)USART3_RX_BUF, "+ROLE:");
            if(p != NULL)
            {
                temp = *(p+6) - '0'; // ?????
                if(temp <= 1) break; // ???(0?1)
            }
        }
        delay_ms(100);
    }
    
    return (temp <= 1) ? temp : 0xFF;
}

// HC05????
// ??????OK???AT??
// atstr:AT???,?"AT+RESET","AT+UART=9600,0,0","AT+ROLE=0"?
// ???:0,????;??,????.
u8 HC05_Set_Cmd(u8* atstr)
{
    u8 retry = 5;
    u8 temp = 0xFF;
    u8 t;  // ???????????
    
    while(retry--)
    {
        HC05_KEY = 1; // ??AT??
        delay_ms(100);
        
        USART3_RX_STA = 0;
        u3_printf("%s\r\n", atstr); // ??AT??
        
        for(t=0; t<20; t++) // ????,??100ms
        {
            if(USART3_RX_STA & 0X8000) break;
            delay_ms(5);
        }
        
        HC05_KEY = 0; // ??AT??
        
        if(USART3_RX_STA & 0X8000) // ?????
        {
            temp = USART3_RX_STA & 0X7FFF;
            USART3_RX_BUF[temp] = 0; // ????????
            
            if(strstr((char*)USART3_RX_BUF, "OK") != NULL)
            {
                temp = 0; // ????
                break;
            }
        }
        delay_ms(100);
    }
    
    return temp;
}

// HC05????(?????)
// str:???(??????????)
void HC05_CFG_CMD(u8 *str)
{
    u8 t;  // ???????????
    
    HC05_KEY = 1; // ??AT??
    delay_ms(100);
    
    USART3_RX_STA = 0;
    u3_printf("%s\r\n", (char*)str); // ????
    
    for(t=0; t<50; t++) // ????,??500ms
    {
        if(USART3_RX_STA & 0X8000) break;
        delay_ms(10);
    }
    
    HC05_KEY = 0; // ??AT??
    
    if(USART3_RX_STA & 0X8000) // ?????
    {
        u16 temp = USART3_RX_STA & 0X7FFF; // ??????
        USART3_RX_BUF[temp] = 0; // ????
        printf("\r\n%s", USART3_RX_BUF); // ?????????1
    }
}
