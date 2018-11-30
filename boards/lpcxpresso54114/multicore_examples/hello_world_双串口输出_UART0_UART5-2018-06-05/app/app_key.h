#ifndef _APP_KEY_H
#define _APP_KEY_H
#include <stdio.h>
#include <stdint.h>

#define KEY_NUM 4

struct KEY_VALUE
{
	uint8_t sda;
	uint8_t scl;
	uint8_t irq;
};
struct CAP_KEY
{
	uint8_t key_chn;
	uint8_t response_chn;
};
enum 
{
    SPEED_KEY=0X01,
    ADD_KEY,
    DEC_KEY,
    MODE_KEY,
    SWITCH_KEY,
};
#define BL_on_time 600;//��λ���� 
#define CTRL_DELAY_TIME 5;//�ϵ���ʱһ��ʱ���ٶ����̵��� ��λ����
//�����������Ŷ���
// PB4  CAP_IEQ
// PA11 SCL3
// PA12 SDA3
//#define IRQ_PORT GPIOB
//#define IRQ_PIN GPIO_Pin_4
//#define IRQ_RCC RCC_AHBPeriph_GPIOB

//#define IRQ_EXTI_PortSource EXTI_PortSourceGPIOB
//#define IRQ_EXTI_PinSource EXTI_PinSource4
//#define IRQ_EXTI_Line			EXTI_Line4

//#define SCL3_PORT GPIOA
//#define SCL3_PIN GPIO_Pin_11
//#define SCL3_RCC RCC_AHBPeriph_GPIOA
//#define SCL3_EXTI_PortSource EXTI_PortSourceGPIOA
//#define SCL3_EXTI_PinSource EXTI_PinSource11
//#define SCL3_EXTI_Line			EXTI_Line11

//#define SDA3_PORT GPIOA
//#define SDA3_PIN GPIO_Pin_12
//#define SDA3_RCC RCC_AHBPeriph_GPIOA
//#define SDA3_EXTI_PortSource EXTI_PortSourceGPIOA
//#define SDA3_EXTI_PinSource EXTI_PinSource12
//#define SDA3_EXTI_Line			EXTI_Line12
//void NVIC_Configuration(void)
//{

// //   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�

//}

//void cap_key_init(void);//�����������ų�ʼ��
void key_value(void);
uint8_t key_init(void);
uint8_t  key_scan(void);
uint8_t  key_scan(void);
#endif
