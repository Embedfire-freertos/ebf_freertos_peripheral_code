/**
  *********************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   FreeRTOS V9.0.0  + STM32 固件库例程
  *********************************************************************
  * @attention
  *
  * 实验平台:野火 STM32 全系列开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  **********************************************************************
  */ 
 
/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/ 
/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
/* 开发板硬件bsp头文件 */
#include "bsp_led.h"
#include "bsp_debug_usart.h"
#include "bsp_key.h"
#include "./i2c/i2c.h"
#include "./mpu6050/mpu6050.h"
#include "./delay/core_delay.h"  
//设置是否使用LCD进行显示，不需要的话把这个宏注释掉即可
#define USE_LCD_DISPLAY

#ifdef USE_LCD_DISPLAY
 #include "./lcd/bsp_ili9806g_lcd.h"
#endif

/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
static TaskHandle_t AppTaskCreate_Handle = NULL;/* 创建任务句柄 */
static TaskHandle_t Test_Task_Handle = NULL;/* LED任务句柄 */
static TaskHandle_t KEY_Task_Handle = NULL;/* KEY任务句柄 */

/********************************** 内核对象句柄 *********************************/
/*
 * 信号量，消息队列，事件标志组，软件定时器这些都属于内核的对象，要想使用这些内核
 * 对象，必须先创建，创建成功之后会返回一个相应的句柄。实际上就是一个指针，后续我
 * 们就可以通过这个句柄操作这些内核对象。
 *
 * 内核对象说白了就是一种全局的数据结构，通过这些数据结构我们可以实现任务间的通信，
 * 任务间的事件同步等各种功能。至于这些功能的实现我们是通过调用这些内核对象的函数
 * 来完成的
 * 
 */


/******************************* 全局变量声明 ************************************/
/*
 * 当我们在写应用程序的时候，可能需要用到一些全局变量。
 */


/*
*************************************************************************
*                             函数声明
*************************************************************************
*/
static void AppTaskCreate(void);/* 用于创建任务 */

static void Test_Task(void* pvParameters);/* Test_Task任务实现 */
static void KEY_Task(void* pvParameters);/* KEY_Task任务实现 */

static void BSP_Init(void);/* 用于初始化板载相关资源 */

/*****************************************************************
  * @brief  主函数
  * @param  无
  * @retval 无
  * @note   第一步：开发板硬件初始化 
            第二步：创建APP应用任务
            第三步：启动FreeRTOS，开始多任务调度
  ****************************************************************/
int main(void)
{	
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  /* 开发板硬件初始化 */
  BSP_Init();
  
  printf("这是一个[野火]-STM32全系列开发板-FreeRTOS固件库例程！\n\n");
  printf("按下KEY1挂起任务，按下KEY2恢复任务\n");
  
   /* 创建AppTaskCreate任务 */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* 任务入口函数 */
                        (const char*    )"AppTaskCreate",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )1, /* 任务的优先级 */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* 任务控制块指针 */ 
  /* 启动任务调度 */           
  if(pdPASS == xReturn)
    vTaskStartScheduler();   /* 启动任务，开启调度 */
  else
    return -1;  
  
  while(1);   /* 正常不会执行到这里 */    
}


/***********************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void AppTaskCreate(void)
{
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  taskENTER_CRITICAL();           //进入临界区
  
  /* 创建Test_Task任务 */
  xReturn = xTaskCreate((TaskFunction_t )Test_Task, /* 任务入口函数 */
                        (const char*    )"Test_Task",/* 任务名字 */
                        (uint16_t       )512,   /* 任务栈大小 */
                        (void*          )NULL,	/* 任务入口函数参数 */
                        (UBaseType_t    )2,	    /* 任务的优先级 */
                        (TaskHandle_t*  )&Test_Task_Handle);/* 任务控制块指针 */
  if(pdPASS == xReturn)
    printf("创建Test_Task任务成功!\r\n");
  /* 创建KEY_Task任务 */
  xReturn = xTaskCreate((TaskFunction_t )KEY_Task,  /* 任务入口函数 */
                        (const char*    )"KEY_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )3, /* 任务的优先级 */
                        (TaskHandle_t*  )&KEY_Task_Handle);/* 任务控制块指针 */ 
  if(pdPASS == xReturn)
    printf("创建KEY_Task任务成功!\r\n");
  
  vTaskDelete(AppTaskCreate_Handle); //删除AppTaskCreate任务
  
  taskEXIT_CRITICAL();            //退出临界区
}



/**********************************************************************
  * @ 函数名  ： Test_Task
  * @ 功能说明： Test_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void Test_Task(void* parameter)
{	
	short Acel[3];
	short Gyro[3];
	float Temp;
	
  while (1)
  {
		MPU6050ReadAcc(Acel);
		printf("加速度：%8d%8d%8d",Acel[0],Acel[1],Acel[2]);
		MPU6050ReadGyro(Gyro);
		printf("    陀螺仪%8d%8d%8d",Gyro[0],Gyro[1],Gyro[2]);
		MPU6050_ReturnTemp(&Temp);
		printf("    温度%8.2f\r\n",Temp);				
				
#ifdef USE_LCD_DISPLAY	
			{
				char cStr [ 70 ];
				sprintf ( cStr, "Acc  :%6d%6d%6d",Acel[0],Acel[1],Acel[2] );	//加速度原始数据


				ILI9806G_DispStringLine_EN(LINE(7),cStr);			

				sprintf ( cStr, "Gyro :%6d%6d%6d",Gyro[0],Gyro[1],Gyro[2] );	//角原始数据

				ILI9806G_DispStringLine_EN(LINE(8),cStr);			

				sprintf ( cStr, "Tem  :%6.2f",Temp );	//温度值
				ILI9806G_DispStringLine_EN(LINE(9),cStr);			

			}
#endif
    vTaskDelay(500);   /* 延时500个tick */
  }
}

/**********************************************************************
  * @ 函数名  ： Test_Task
  * @ 功能说明： Test_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void KEY_Task(void* parameter)
{	
  while (1)
  {
    if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON )
    {/* K1 被按下 */
      printf("挂起Test任务！\n");
      vTaskSuspend(Test_Task_Handle);/* 挂起LED任务 */
      printf("挂起Test任务成功！\n");
    } 
    if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON )
    {/* K2 被按下 */
      printf("恢复Test任务！\n");
      vTaskResume(Test_Task_Handle);/* 恢复LED任务！ */
      printf("恢复Test任务成功！\n");
    }
    vTaskDelay(20);/* 延时20个tick */
  }
}

/***********************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  *********************************************************************/
static void BSP_Init(void)
{
	/*
	 * STM32中断优先级分组为4，即4bit都用来表示抢占优先级，范围为：0~15
	 * 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断，
	 * 都统一用这个优先级分组，千万不要再分组，切忌。
	 */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	/* LED 初始化 */
	LED_GPIO_Config();

	/* 串口初始化	*/
	Debug_USART_Config();
  
  /* 按键初始化	*/
  Key_GPIO_Config();
	
	CPU_TS_TmrInit();
	
#ifdef USE_LCD_DISPLAY	
		 /*初始化液晶屏*/
	ILI9806G_Init ();         //LCD 初始化
	
	//其中0、3、5、6 模式适合从左至右显示文字，
	//不推荐使用其它模式显示文字	其它模式显示文字会有镜像效果			
	//其中 6 模式为大部分液晶例程的默认显示方向  
  ILI9806G_GramScan ( 6 );

#endif
	//初始化 I2C
	I2cMaster_Init(); 

	printf("\r\n 欢迎使用野火  STM32 F407 开发板。\r\n");		 

	printf("\r\n 这是一个I2C外设(AT24C02)读写测试例程 \r\n");

 	 	//MPU6050初始化
	MPU6050_Init();
	
	//检测MPU6050
	if (MPU6050ReadID() == 1)
	{	
		LED2_ON;
	}
	else
	{
			printf("\r\n没有检测到MPU6050传感器！\r\n");
			LED_RED; 
			#ifdef USE_LCD_DISPLAY			
				/*设置字体颜色及字体的背景颜色*/
				LCD_SetColors(BLUE,BLACK);	

				ILI9806G_DispStringLine_EN(LINE(4),"No MPU6050 detected! ");			//野火自带的16*24显示
				ILI9806G_DispStringLine_EN(LINE(5),"Please check the hardware connection! ");			//野火自带的16*24显示

			#endif
		while(1);	
	}
}

/********************************END OF FILE****************************/
