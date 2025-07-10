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
#include "usart3.h"			 	 
#include "ws2812.h"
#include "pwm.h"
extern const u8 g_rgb_num_buf[][5];

void RGB_ShowTemperature(u8 temp, u32 color, u8 color_n)
{
	
    u8 digits[3];
    int j;
    digits[0] = temp / 10;
    digits[1] = temp % 10;
    digits[2] = 16; // ����C���ϲ��ַ���g_rgb_num_buf������
    
        RGB_LED_Clear();
        RGB_ShowCharNum(digits[color_n], color);
}
// ���尴ť�ṹ��
typedef struct {
    u16 x_start;
    u16 y_start;
    u16 width;
    u16 height;
    u8 device_num;     // ���Ƶ��豸��ţ�1=LED1, 2=LED2, 3=BEEP
    u8 state;          // �豸��ǰ״̬��0=�أ�1=��
} Button;

Button buttons[3];  // �������ư�ť��LED1��LED2��BEEP

// �򻯿�����ʾ
void kai_display()  
{
    FRONT_COLOR = BLACK;
    LCD_ShowString(10, 10, tftlcd_data.width, tftlcd_data.height, 16, "Device Control");
}

// ��ʼ����ť����
void init_buttons()
{
    u16 btn_width = 80;
    u16 btn_height = 50;
    u16 spacing = 40;
    u16 start_x = (tftlcd_data.width - (3 * btn_width + 2 * spacing)) / 2;
    u16 start_y = 80;
    
    // LED1��ť
    buttons[0].x_start = start_x;
    buttons[0].y_start = start_y;
    buttons[0].width = btn_width;
    buttons[0].height = btn_height;
    buttons[0].device_num = 1;  // ����LED1
    buttons[0].state = 0;
    
    // LED2��ť
    buttons[1].x_start = start_x + btn_width + spacing;
    buttons[1].y_start = start_y;
    buttons[1].width = btn_width;
    buttons[1].height = btn_height;
    buttons[1].device_num = 2;  // ����LED2
    buttons[1].state = 0;
    
    // ��������ť - �����������
    buttons[2].x_start = start_x + 2 * (btn_width + spacing);
    buttons[2].y_start = start_y;  // ������ͬһ��
    buttons[2].width = btn_width;
    buttons[2].height = btn_height;
    buttons[2].device_num = 3;  // ���Ʒ�����
    buttons[2].state = 0;
}

// �������а�ť
void draw_buttons()
{
    u8 i;
    char text[10];
    
    for(i = 0; i < 3; i++) {
        // ���ư�ť���� - ʹ�ú���״ָ̬ʾ
        u16 bg_color = buttons[i].state ? GREEN : RED;
        LCD_Fill(buttons[i].x_start, 
                buttons[i].y_start,
                buttons[i].x_start + buttons[i].width,
                buttons[i].y_start + buttons[i].height,
                bg_color);
        
        // ���ư�ť�߿�
        LCD_DrawRectangle(buttons[i].x_start, 
                        buttons[i].y_start,
                        buttons[i].x_start + buttons[i].width,
                        buttons[i].y_start + buttons[i].height);
        
        // ���ư�ť����
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

// �����豸״̬���ػ水ť
void toggle_device(u8 btn_index)
{
    if(btn_index >= 3) return;
    
    // �л��豸״̬
    buttons[btn_index].state = !buttons[btn_index].state;
    
    // ����ʵ���豸
    if(buttons[btn_index].device_num == 1) {       // LED1
        LED1 = buttons[btn_index].state ? 0 : 1;   // ״̬Ϊ1ʱ����
    } 
    else if(buttons[btn_index].device_num == 2) {  // LED2
        LED2 = buttons[btn_index].state ? 0 : 1;   // ״̬Ϊ1ʱ����
    } 
    else if(buttons[btn_index].device_num == 3) {  // BEEP
        if(buttons[btn_index].state) {
            GPIO_SetBits(BEEP_PORT, BEEP_PIN);    // ����������
        } else {
            GPIO_ResetBits(BEEP_PORT, BEEP_PIN);  // �رշ�����
        }
    }
    
    // �ػ水ť
    draw_buttons();
}
//=========================================================================================
//=======================������������ʾ����=================================================
//=========================================================================================
// ����ȫ�ֱ����洢������ֵ
u8 g_temp, g_humi, g_lsens;

// �޸ĺ�����ݴ�����
void data_pros()    
{
		u8 temp_buf[3],humi_buf[3];
		u8 lsens_buf[3];
    DHT11_Read_Data(&g_temp, &g_humi);  // ���浽ȫ�ֱ���
    
    g_lsens = Lsens_Get_Val();  // ���浽ȫ�ֱ���
    
    // �¶���ʾ
    temp_buf[0]=g_temp/10+0x30;    
    temp_buf[1]=g_temp%10+0x30;
    temp_buf[2]='\0';
    LCD_ShowString(55,200,tftlcd_data.width,tftlcd_data.height,16,temp_buf);
    
    // ʪ����ʾ    
    humi_buf[0]=g_humi/10+0x30;    
    humi_buf[1]=g_humi%10+0x30;
    humi_buf[2]='\0';
    LCD_ShowString(55,230,tftlcd_data.width,tftlcd_data.height,16,humi_buf);
    
    // ������ʾ
    
    sprintf((char*)lsens_buf, "%d", g_lsens); 
    LCD_ShowString(55,260,tftlcd_data.width,tftlcd_data.height,16,lsens_buf);
    
    printf("�¶�=%d��C  ʪ��=%d%%RH  ����ǿ�ȣ�%d\r\n", g_temp, g_humi, g_lsens);
}

//=========================================================================================
//=======================��������������ʾ����===============================================
//=========================================================================================
//��ʾHC05ģ�������״̬
void HC05_Role_Show(void)
{
	if(HC05_Get_Role()==1)
	{
		LCD_ShowString(10,140,200,16,16,"ROLE:Master");	//����
	}
	else 
	{
		LCD_ShowString(10,140,200,16,16,"ROLE:Slave ");	//�ӻ�
	}
}

//��ʾHC05ģ�������״̬
void HC05_Sta_Show(void)
{												 
	if(HC05_LED)
	{
		LCD_ShowString(110,140,120,16,16,"STA:Connected ");	//���ӳɹ�
	}
	else 
	{
		LCD_ShowString(110,140,120,16,16,"STA:Disconnect");	 			//δ����
	}				 
}

//void data_pros()    //���ݴ�����
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
//    printf("�¶�=%d��C  ʪ��=%d%%RH\r\n",temp,humi);
//    
//    

//    lsens_value=Lsens_Get_Val();
//    sprintf((char*)lsens_buf, "%d", lsens_value); 
//    LCD_ShowString(55,260,tftlcd_data.width,tftlcd_data.height,16,lsens_buf);
//    printf("����ǿ�ȣ�%d\r\n",lsens_value);
//}

int main() {
	
	typedef enum {
        TEMP_DISPLAY_OFF,     // �ر�״̬
        TEMP_DISPLAY_TENS,    // ʮλ��
        TEMP_DISPLAY_UNITS,   // ��λ��
        TEMP_DISPLAY_SYMBOL   // �����
    } TempDisplayState;
    int pwm_value = 0;
    int new_pwm = 0;  // �洢�Զ�����ķ���PWMֵ
    u8 auto_fan_mode = 0; // 0=�ֶ�ģʽ, 1=�Զ�ģʽ
    u8 temp = 0, humi = 0;
    u8 color_index = 0;
    u8 color_n = 0;
    u32 color_list[] = {RGB_COLOR_RED, RGB_COLOR_GREEN, RGB_COLOR_BLUE, RGB_COLOR_WHITE, RGB_COLOR_YELLOW};
    u8 i = 0;
    u8 j;  // ����ѭ���ı���
    u8 key;
    u8 t = 0;
    u8 sendmask = 0;
    u8 sendcnt = 0;
    u8 sendbuf[20];	  
    u8 reclen = 0; 
    char fan_buf[10];


    
    TempDisplayState tempDisplayState = TEMP_DISPLAY_OFF;
    u8 tempToDisplay = 0;
    u32 tempDisplayColor = 0;
    u32 tempDisplayStartTime = 0;
		u32 tempUpdateTime = 0;  // �¶ȸ���ʱ���
    // ===== �������� =====
    
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
    // ���̿�����ʾʱ��
    kai_display();
    delay_ms(500);
    LCD_Clear(WHITE);
    RGB_LED_Init();  // ��ʼ������
    // ��ʼ����ť����
    init_buttons();
    draw_buttons();
    TIM3_CH2_PWM_Init(500,72-1); //Ƶ����2Kh
    TIM_SetCompare2(TIM3,400);    
    // ��RST��ʾ
    FRONT_COLOR = RED;
    LCD_ShowString(tftlcd_data.width - 30, 0, 30, 16, 16, "RST");
    
    // ��ʼ���豸״̬
		buttons[0].state == 1;
LED1 = buttons[0].state ? 0 : 1;  // ͬ����ʼ״̬
LED2 = buttons[1].state ? 0 : 1;
    GPIO_ResetBits(BEEP_PORT, BEEP_PIN); // ��ʼ�رշ�����
    
    // ������Ϣ����ʾ��Ļ�ߴ�
    printf("Screen Width: %d, Height: %d\n", tftlcd_data.width, tftlcd_data.height);
     // ��ӷ���PWM��ʾ��ǩ
    LCD_ShowString(10, 290, tftlcd_data.width, tftlcd_data.height, 16, "Fan:     ");
    
    // ��ʼ����ʾPWMֵ
    sprintf(fan_buf, "%d", pwm_value);
    LCD_ShowString(55, 290, tftlcd_data.width, tftlcd_data.height, 16, fan_buf);
    LCD_ShowString(10,200,tftlcd_data.width,tftlcd_data.height,16,"Temp:   C");
    LCD_ShowString(10,230,tftlcd_data.width,tftlcd_data.height,16,"Humi:   %RH");
    LCD_ShowString(10,260,tftlcd_data.width,tftlcd_data.height,16,"Lsen:     ");
    FRONT_COLOR=RED;
		
    while(DHT11_Init()) {   //���DS18B20�Ƿ���
        LCD_ShowString(130,50,tftlcd_data.width,tftlcd_data.height,16,"Error   ");
        printf("DHT11 Check Error!\r\n");
        delay_ms(500);        
    }
    LCD_ShowString(130,50,tftlcd_data.width,tftlcd_data.height,16,"Success");
    printf("DHT11 Check OK!\r\n");

    //=======================
    //��ʼ��HC05ģ��  
    while(HC05_Init()) {
        printf("HC05 Error!\r\n");
        LCD_ShowString(10,90,200,16,16,"HC05 Error!    "); 
        delay_ms(500);
        LCD_ShowString(10,90,200,16,16,"Please Check!!!"); 
        delay_ms(100);
    }
    printf("HC05 OK!\r\n");
    //=======================
    FRONT_COLOR=BLUE;
    HC05_Role_Show();
    delay_ms(100);
    USART3_RX_STA=0;
    
    while(1) {
        key = KEY_Scan(0);
        if(key == KEY_UP_PRESS) { 
            // �����Զ�ģʽ�������¶�ʵʱ��������
            auto_fan_mode = 1;
            LCD_ShowString(10, 320, tftlcd_data.width, tftlcd_data.height, 16, "Auto Mode: ON ");
        }
        else if(key == KEY1_PRESS) { 
            // �˳��Զ�ģʽ���رշ���
            auto_fan_mode = 0;
            pwm_value = 0;
            TIM_SetCompare2(TIM3, 500 - pwm_value);
            LCD_ShowString(10, 320, tftlcd_data.width, tftlcd_data.height, 16, "Auto Mode: OFF");
            
            // ���·���PWM��ʾ
            sprintf(fan_buf, "%d", pwm_value);
            LCD_ShowString(55, 290, tftlcd_data.width, tftlcd_data.height, 16, "000");
						
            // ԭ�еķ���/ֹͣ���͹���
            sendmask = !sendmask;
            if(sendmask == 0) LCD_Fill(10+40,160,240,160+16,WHITE); //�����ʾ
        }
        
        // ���Զ�ģʽ��ʵʱ���·����ٶ�
        if(auto_fan_mode) {
            pwm_value = new_pwm;
            TIM_SetCompare2(TIM3, 500 - pwm_value);
            sprintf(fan_buf, "%d", pwm_value);
            LCD_ShowString(55, 290, tftlcd_data.width, tftlcd_data.height, 16, fan_buf);
        }
        
        if(TP_Scan(0)) {
            // ��RST��ť���
            if (tp_dev.x[0] > (tftlcd_data.width - 30) && tp_dev.y[0] < 16) {
                LCD_Clear(WHITE);
                draw_buttons();
                LCD_ShowString(tftlcd_data.width - 30, 0, 30, 16, 16, "RST");
                // �����Զ�ģʽ��ʾ
                if(auto_fan_mode) {
                    LCD_ShowString(10, 320, tftlcd_data.width, tftlcd_data.height, 16, "Auto Mode: ON ");
                } else {
                    LCD_ShowString(10, 320, tftlcd_data.width, tftlcd_data.height, 16, "Auto Mode: OFF");
                }
            }
            else {
                // ����豸��ť���
                for(j = 0; j < 3; j++) {
                    // ������Ϣ����ʾ��������Ͱ�ť����
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
        
        if(t == 50) { // ��ʱ������������
            if(sendmask) { // ��ʱ����
                sprintf((char*)sendbuf,"PREHICN HC05 %d\r\n",sendcnt);
                printf("%s\r\n",sendbuf);
                u3_printf("PREHICN HC05 %d\r\n",sendcnt); //���͵�����ģ��
                sendcnt++;
                if(sendcnt > 99) sendcnt = 0;
            }
            HC05_Sta_Show();  	  
            t = 0;
        }
        
        if(USART3_RX_STA & 0X8000) { // ���յ�һ��������
            LCD_Fill(10,170,0,0,BLUE); // �����ʾ���򣨱��⸲����ʪ����Ϣ��
            reclen = USART3_RX_STA & 0X7FFF; // �õ����ݳ���
            USART3_RX_BUF[reclen] = '\0'; // ���������
            printf("reclen=%d\r\n",reclen);
            printf("USART3_RX_BUF=%s\r\n",USART3_RX_BUF);
            
            if(reclen == 10 || reclen == 11) { // ����D2���
                if(strcmp((const char*)USART3_RX_BUF,"+LED2 ON\r\n") == 0) LED2 = 0; // ��LED2
                if(strcmp((const char*)USART3_RX_BUF,"+LED2 OFF\r\n") == 0) LED2 = 1; // �ر�LED2
                if(strcmp((const char*)USART3_RX_BUF,"+LED1 ON\r\n") == 0) LED1 = 0; // ��LED1
                if(strcmp((const char*)USART3_RX_BUF,"+LED1 OFF\r\n") == 0) LED1 = 1; // �ر�LED1
                if(strcmp((const char*)USART3_RX_BUF,"+BEEP ON\r\n") == 0) BEEP = 1; // ��BEEP
                if(strcmp((const char*)USART3_RX_BUF,"+BEEP OFF\r\n") == 0) BEEP = 0; // �ر�beep
								if(strcmp((const char*)USART3_RX_BUF,"+FANF ON\r\n") == 0) 
								{
									if(pwm_value>=500)pwm_value=500;
									else pwm_value += 50;
									TIM_SetCompare2(TIM3, 500-pwm_value);}
									// ��BEEP
                if(strcmp((const char*)USART3_RX_BUF,"+FANF OFF\r\n") == 0)
								{
									if(pwm_value<=0)pwm_value=0;
									else pwm_value -= 50;
									TIM_SetCompare2(TIM3, 500-pwm_value);
								// �ر�beep
							}
            
            LCD_ShowString(10,350,209,50,16,USART3_RX_BUF); // ��ʾ���յ������ݣ�λ�õ�����
            USART3_RX_STA = 0;	 
        }}
        
        i++;
				
        if(i % 20 == 0) {
            data_pros(); // ��ȡһ��DHT11����
					if(DHT11_Read_Data(&temp, &humi) == 0 && i%100==0)
            {
                // ���� ��ʾ�¶����֣�ÿ���ַ�1�룩
                RGB_ShowTemperature(temp, color_list[color_index], color_n);
                color_n++;
                if(color_n>=3){
                    color_n=0;
                    color_index = (color_index + 1) % 5; // ѭ���л���ɫ
                    }
								
//										if(buttons[0].state == 0) LED1 = 0;
//										else LED1=1;
//										if(buttons[1].state == 0) LED2 = 0;
//										else LED2=1;
            }
					
            // ===== �Զ������߼� =====
            // ���տ���LED
//            if(g_lsens < 10) {
//                LED1 = 0;  // �͵�ƽ����
//                LED2 = 0;
//            } 
            if(g_lsens < 50) {
                //LED1 = 0;
                LED2 = 0;  // �ߵ�ƽϨ��
            } 
            else {
                // �ָ��ֶ�����״̬
                LED1 = buttons[0].state ? 0 : 1;
                LED2 = buttons[1].state ? 0 : 1;
            }
            
            // ʪ�ȿ��Ʒ�����
            if(g_humi > 70) {
                GPIO_SetBits(BEEP_PORT, BEEP_PIN);  // ����������
							delay_ms(1000);
            } 
            else {
                // �ָ��ֶ�����״̬
                if(buttons[2].state) {
                    GPIO_SetBits(BEEP_PORT, BEEP_PIN);
                } else {
                    GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
                }
            }
						
            // ===== �¶��Զ����Ʒ��� =====
            // �����¶ȼ������PWMֵ (23��C=100, 24��C=125, 25��C=150...)
            new_pwm = 100 + (g_temp - 23) * 25;
            
            // ����PWMֵ��0-500֮��
            if (new_pwm < 0) new_pwm = 0;
            if (new_pwm > 500) new_pwm = 500;
            
            // ���·���PWM��ʾ�������Զ�ģʽʱ����ʵ��PWMֵ��
            sprintf(fan_buf, "%d", pwm_value);
            LCD_ShowString(55, 290, tftlcd_data.width, tftlcd_data.height, 16, fan_buf);
            // ===== �������� =====
						
        }
  
        
        t++;
        delay_ms(10);
    }
}