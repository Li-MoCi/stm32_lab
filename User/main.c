#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "touch.h"
#include "beep.h"
#include "dht11.h"
#include "lsens.h"
#include "hc05.h"
// 定义按钮结构体
typedef struct {
    u16 x_start;
    u16 y_start;
    u16 width;
    u16 height;
    u8 device_num;     // 控制的设备编号：1=LED1, 2=LED2, 3=BEEP
    u8 state;          // 设备当前状态：0=关，1=开
} Button;

Button buttons[3];  // 三个控制按钮：LED1、LED2和BEEP

// 简化开机显示
void kai_display()  
{
    FRONT_COLOR = BLACK;
    LCD_ShowString(10, 10, tftlcd_data.width, tftlcd_data.height, 16, "Device Control");
}

// 初始化按钮布局
void init_buttons()
{
    u16 btn_width = 80;
    u16 btn_height = 50;
    u16 spacing = 40;
    u16 start_x = (tftlcd_data.width - (3 * btn_width + 2 * spacing)) / 2;
    u16 start_y = 80;
    
    // LED1按钮
    buttons[0].x_start = start_x;
    buttons[0].y_start = start_y;
    buttons[0].width = btn_width;
    buttons[0].height = btn_height;
    buttons[0].device_num = 1;  // 控制LED1
    buttons[0].state = 0;
    
    // LED2按钮
    buttons[1].x_start = start_x + btn_width + spacing;
    buttons[1].y_start = start_y;
    buttons[1].width = btn_width;
    buttons[1].height = btn_height;
    buttons[1].device_num = 2;  // 控制LED2
    buttons[1].state = 0;
    
    // 蜂鸣器按钮 - 修正坐标计算
    buttons[2].x_start = start_x + 2 * (btn_width + spacing);
    buttons[2].y_start = start_y;  // 保持在同一行
    buttons[2].width = btn_width;
    buttons[2].height = btn_height;
    buttons[2].device_num = 3;  // 控制蜂鸣器
    buttons[2].state = 0;
}

// 绘制所有按钮
void draw_buttons()
{
    u8 i;
    char text[10];
    
    for(i = 0; i < 3; i++) {
        // 绘制按钮背景 - 使用红绿状态指示
        u16 bg_color = buttons[i].state ? GREEN : RED;
        LCD_Fill(buttons[i].x_start, 
                buttons[i].y_start,
                buttons[i].x_start + buttons[i].width,
                buttons[i].y_start + buttons[i].height,
                bg_color);
        
        // 绘制按钮边框
        LCD_DrawRectangle(buttons[i].x_start, 
                        buttons[i].y_start,
                        buttons[i].x_start + buttons[i].width,
                        buttons[i].y_start + buttons[i].height);
        
        // 绘制按钮文字
        FRONT_COLOR = BLACK;
        if(buttons[i].device_num == 1) {
            sprintf(text, "LED1");
        } else if(buttons[i].device_num == 2) {
            sprintf(text, "LED2");
        } else if(buttons[i].device_num == 3) {
            sprintf(text, "BEEP");
        }
        LCD_ShowString(buttons[i].x_start + 20, 
                      buttons[i].y_start + 15,
                      buttons[i].width, 
                      buttons[i].height,
                      16, 
                      text);
    }
}

// 更新设备状态并重绘按钮
void toggle_device(u8 btn_index)
{
    if(btn_index >= 3) return;
    
    // 切换设备状态
    buttons[btn_index].state = !buttons[btn_index].state;
    
    // 控制实际设备
    if(buttons[btn_index].device_num == 1) {       // LED1
        LED1 = buttons[btn_index].state ? 0 : 1;   // 状态为1时亮灯
    } 
    else if(buttons[btn_index].device_num == 2) {  // LED2
        LED2 = buttons[btn_index].state ? 0 : 1;   // 状态为1时亮灯
    } 
    else if(buttons[btn_index].device_num == 3) {  // BEEP
        if(buttons[btn_index].state) {
            GPIO_SetBits(BEEP_PORT, BEEP_PIN);    // 开启蜂鸣器
        } else {
            GPIO_ResetBits(BEEP_PORT, BEEP_PIN);  // 关闭蜂鸣器
        }
    }
    
    // 重绘按钮
    draw_buttons();
}
//=========================================================================================
//=======================新增传感器显示部分=================================================
//=========================================================================================
// 定义全局变量存储传感器值
u8 g_temp, g_humi, g_lsens;

// 修改后的数据处理函数
void data_pros()    
{
		u8 temp_buf[3],humi_buf[3];
		u8 lsens_buf[3];
    DHT11_Read_Data(&g_temp, &g_humi);  // 保存到全局变量
    
    g_lsens = Lsens_Get_Val();  // 保存到全局变量
    
    // 温度显示
    temp_buf[0]=g_temp/10+0x30;    
    temp_buf[1]=g_temp%10+0x30;
    temp_buf[2]='\0';
    LCD_ShowString(55,200,tftlcd_data.width,tftlcd_data.height,16,temp_buf);
    
    // 湿度显示    
    humi_buf[0]=g_humi/10+0x30;    
    humi_buf[1]=g_humi%10+0x30;
    humi_buf[2]='\0';
    LCD_ShowString(55,230,tftlcd_data.width,tftlcd_data.height,16,humi_buf);
    
    // 光照显示
    
    sprintf((char*)lsens_buf, "%d", g_lsens); 
    LCD_ShowString(55,260,tftlcd_data.width,tftlcd_data.height,16,lsens_buf);
    
    printf("温度=%d°C  湿度=%d%%RH  光照强度：%d\r\n", g_temp, g_humi, g_lsens);
}

//=========================================================================================
//=======================新增蓝牙控制显示部分===============================================
//=========================================================================================
//显示HC05模块的主从状态
void HC05_Role_Show(void)
{
	if(HC05_Get_Role()==1)
	{
		LCD_ShowString(10,140,200,16,16,"ROLE:Master");	//主机
	}
	else 
	{
		LCD_ShowString(10,140,200,16,16,"ROLE:Slave ");	//从机
	}
}

//显示HC05模块的连接状态
void HC05_Sta_Show(void)
{												 
	if(HC05_LED)
	{
		LCD_ShowString(110,140,120,16,16,"STA:Connected ");	//连接成功
	}
	else 
	{
		LCD_ShowString(110,140,120,16,16,"STA:Disconnect");	 			//未连接
	}				 
}

//void data_pros()    //数据处理函数
//{
//    u8 temp;          
//    u8 humi;
//    u8 temp_buf[3],humi_buf[3];
//	  u8 lsens_value=0;
//    u8 lsens_buf[3];
//    DHT11_Read_Data(&temp,&humi);
//    temp_buf[0]=temp/10+0x30;    
//    temp_buf[1]=temp%10+0x30;
//    temp_buf[2]='\0';
//    LCD_ShowString(55,200,tftlcd_data.width,tftlcd_data.height,16,temp_buf);
//        
//    humi_buf[0]=humi/10+0x30;    
//    humi_buf[1]=humi%10+0x30;
//    humi_buf[2]='\0';
//    LCD_ShowString(55,230,tftlcd_data.width,tftlcd_data.height,16,humi_buf);
//    printf("温度=%d°C  湿度=%d%%RH\r\n",temp,humi);
//    
//    

//    lsens_value=Lsens_Get_Val();
//    sprintf((char*)lsens_buf, "%d", lsens_value); 
//    LCD_ShowString(55,260,tftlcd_data.width,tftlcd_data.height,16,lsens_buf);
//    printf("光照强度：%d\r\n",lsens_value);
//}


int main()
{
		u8 i=0;
    u8 j;  // 用于循环的变量
    u8 key;
		u8 t=0;
		u8 sendmask=0;
		u8 sendcnt=0;
		u8 sendbuf[20];	  
		u8 reclen=0; 
    
    SysTick_Init(72);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    LED_Init();
    USART1_Init(115200);
    TFTLCD_Init();
    KEY_Init();
    TP_Init();
    BEEP_Init();
    Lsens_Init();
    TFTLCD_Init();           
    // 缩短开机显示时间
    kai_display();
    delay_ms(500);
    LCD_Clear(WHITE);
    
    // 初始化按钮布局
    init_buttons();
    draw_buttons();
    

	
	
	
	
    // 简化RST显示
    FRONT_COLOR = RED;
    LCD_ShowString(tftlcd_data.width - 30, 0, 30, 16, 16, "RST");
    
    // 初始化设备状态
    LED1 = 1;
    LED2 = 1;
    GPIO_ResetBits(BEEP_PORT, BEEP_PIN); // 初始关闭蜂鸣器
    
    // 调试信息：显示屏幕尺寸
    printf("Screen Width: %d, Height: %d\n", tftlcd_data.width, tftlcd_data.height);
    
    LCD_ShowString(10,200,tftlcd_data.width,tftlcd_data.height,16,"Temp:   C");
    LCD_ShowString(10,230,tftlcd_data.width,tftlcd_data.height,16,"Humi:   %RH");
    LCD_ShowString(10,260,tftlcd_data.width,tftlcd_data.height,16,"Lsen:     ");
    FRONT_COLOR=RED;
		
		while(DHT11_Init())    //检测DS18B20是否纯在
    {
        LCD_ShowString(130,50,tftlcd_data.width,tftlcd_data.height,16,"Error   ");
        printf("DHT11 Check Error!\r\n");
        delay_ms(500);        
    }
    LCD_ShowString(130,50,tftlcd_data.width,tftlcd_data.height,16,"Success");
    printf("DHT11 Check OK!\r\n");

		
	//=======================
		//初始化HC05模块  
		while(HC05_Init()) 		
	{
		printf("HC05 Error!\r\n");
		LCD_ShowString(10,90,200,16,16,"HC05 Error!    "); 
		delay_ms(500);
		LCD_ShowString(10,90,200,16,16,"Please Check!!!"); 
		delay_ms(100);
	}
	printf("HC05 OK!\r\n");
	//=======================
	
    while(1)
    {
        key = KEY_Scan(0);
        if(key == KEY_UP_PRESS) {
            TP_Adjust();
            LCD_Clear(WHITE);
            draw_buttons();
            LCD_ShowString(tftlcd_data.width - 30, 0, 30, 16, 16, "RST");
        }
        
        if(TP_Scan(0)) {
            // 简化RST按钮检测
            if (tp_dev.x[0] > (tftlcd_data.width - 30) && tp_dev.y[0] < 16) {
                LCD_Clear(WHITE);
                draw_buttons();
                LCD_ShowString(tftlcd_data.width - 30, 0, 30, 16, 16, "RST");
            }
            else {
                // 检查设备按钮点击
                for(j = 0; j < 3; j++) {
                    // 调试信息：显示触摸坐标和按钮区域
                    printf("Touch: X=%d, Y=%d | Button %d: X1=%d, X2=%d, Y1=%d, Y2=%d\n", 
                           tp_dev.x[0], tp_dev.y[0],
                           j,
                           buttons[j].x_start, 
                           buttons[j].x_start + buttons[j].width,
                           buttons[j].y_start,
                           buttons[j].y_start + buttons[j].height);
                    
                    if(tp_dev.x[0] > buttons[j].x_start &&
                       tp_dev.x[0] < (buttons[j].x_start + buttons[j].width) &&
                       tp_dev.y[0] > buttons[j].y_start &&
                       tp_dev.y[0] < (buttons[j].y_start + buttons[j].height)) 
                    {
                        printf("Button %d pressed\n", j);
                        toggle_device(j);
                        delay_ms(200);
                        break;
                    }
                }
            }
        }
				i++;
        if(i%20==0)
        {
         
            data_pros();       //读取一次DHT11数据最少要大于100ms
						            // ===== 新增的自动控制逻辑 =====
            // 光照控制LED
            if(g_lsens < 10) {
                LED1 = 0;  // 低电平点亮
                LED2 = 0;
            } 
            else if(g_lsens < 50) {
                LED1 = 0;
                LED2 = 1;  // 高电平熄灭
            } 
            else {
                // 恢复手动控制状态
                LED1 = buttons[0].state ? 0 : 1;
                LED2 = buttons[1].state ? 0 : 1;
            }
            
            // 湿度控制蜂鸣器
            if(g_humi > 70) {
                GPIO_SetBits(BEEP_PORT, BEEP_PIN);  // 开启蜂鸣器
            } 
            else {
                // 恢复手动控制状态
                if(buttons[2].state) {
                    GPIO_SetBits(BEEP_PORT, BEEP_PIN);
                } else {
                    GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
                }
            }
        }
        
        delay_ms(10);
    }
}