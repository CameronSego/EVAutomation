
extern "C"
{
#include "can.h"
}

#define PART_TM4C1294NCPDT
#include <tm4c1294ncpdt.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/pwm.h>
#include <driverlib/fpu.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>
#include <driverlib/i2c.h>
#include <driverlib/watchdog.h>

namespace steering
{
  void SetPWM(float duty_cycle)
  {
    const uint32_t pwm_clk_period = 8000;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, pwm_clk_period);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, pwm_clk_period*(1-duty_cycle)); // Yellow wire (F2)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm_clk_period*duty_cycle); // Green wire (F3)
  }
  volatile float steer_value = 0.0f; // For debugging
  void SetNormalized(float val)
  {
    steer_value = val;
    SetPWM(val * 0.2f + 0.5f);
  }
  void Init()
  {
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    SetPWM(0.50f);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMOutputState(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
    PWMOutputInvert(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
  }
}

namespace accelerator
{
  void SetVoltage(float v)
  {
    uint16_t bitval = 0xFFFF * (v / 3.3f);
    I2CMasterSlaveAddrSet(I2C4_BASE, 0x62, false);
    I2CMasterDataPut(I2C4_BASE, 0x40);
    I2CMasterControl(I2C4_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C4_BASE)) {}
    I2CMasterDataPut(I2C4_BASE, (bitval >> 8));
    I2CMasterControl(I2C4_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(I2C4_BASE)) {}
    I2CMasterDataPut(I2C4_BASE, bitval & 0xFF);
    I2CMasterControl(I2C4_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    while(I2CMasterBusy(I2C4_BASE)) {}
  }
  volatile float accelerator_value = 0.0f; // For debugging
  void SetNormalized(float val)
  {
    accelerator_value = val;
    SetVoltage(0.39f + val * (0.6f - 0.39f));
  }
  void Init()
  {
    // I2C Stuff on K6(SCL) and K7(SDA)
    I2CMasterInitExpClk(I2C4_BASE, 16000000, true);
    GPIOPinTypeI2CSCL(GPIOK_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIOK_BASE, GPIO_PIN_7);
    GPIOPinConfigure(GPIO_PK6_I2C4SCL);
    GPIOPinConfigure(GPIO_PK7_I2C4SDA);
    
		//Set initial voltage
    SetVoltage(0.39f);
  }
}

namespace brakes
{
  void SetPWM(float duty_cycle1, float duty_cycle2)
  {
		const uint32_t pwm0_clk_period = 30200;
		const uint32_t pwm2_clk_period = 33506;
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, pwm0_clk_period);
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwm2_clk_period);
		if(duty_cycle1 >= 0.843f)
		{
			duty_cycle1 = 0.843f;
		}
		if(duty_cycle1 <= 0.535f)
		{
			duty_cycle1 = 0.535f;
		}
		if(duty_cycle2 >= 0.493f)
		{
			duty_cycle2 = 0.493f;
		}
		if(duty_cycle2 <= 0.155f)
		{
			duty_cycle2 = 0.155f;
		}
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, pwm0_clk_period*duty_cycle1);
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, pwm2_clk_period*duty_cycle2);	
  }
  volatile float brake_value1 = 0.0f; // For debugging
	volatile float brake_value2 = 0.0f;
	
  void SetNormalized(float val)
  {
    brake_value1 = (1.0f-val)*(.843f-.535f)+.535f;
		brake_value2 = (val)*(.493f-.155f)+.155f;
		SetPWM(brake_value1,brake_value2);
  }
  void Init()
  {
		// Brake pinout F1 and G1
		PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
		PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
		brakes::SetPWM(0.843f, 0.155f); // Resting duty % for brakes
		PWMGenEnable(PWM0_BASE, PWM_GEN_0);
		PWMGenEnable(PWM0_BASE, PWM_GEN_2);
		PWMOutputState(PWM0_BASE, (PWM_OUT_1_BIT | PWM_OUT_5_BIT), true);
  }
}

float accel_pulse_width = 0.0f;
float steer_pulse_width = 0.0f;

void UpdateAcceleration(float pulse_width)
{
  if(pulse_width > 0.0008f && pulse_width < 0.0022f)
  {
    if(pulse_width < 0.00145f)
    {
      accelerator::SetNormalized((float) (0.00145f - pulse_width)/(0.00145f - 0.001f));
      brakes::SetNormalized(0.0f);
    }
    else if(pulse_width > 0.00155f)
    {
      accelerator::SetNormalized(0.0f);
      brakes::SetNormalized((float) (pulse_width - 0.00155f)/(0.002f - 0.00155f));
    }
    else
    {
      accelerator::SetNormalized(0.0f);
      brakes::SetNormalized(0.0f);
    }
  }
  else
  {
    accelerator::SetNormalized(0.0f);
    brakes::SetNormalized(0.0f);
  }
}
void UpdateSteering(float pulse_width)
{
  if(pulse_width > 0.0008f && pulse_width < 0.0022f)
  {
    if(pulse_width < 0.00145f || pulse_width > 0.00155f)
    {
      steering::SetNormalized((pulse_width - 0.0015f)/(0.002f - 0.0015f));
    }
    else
    {
      steering::SetNormalized(0.0f);
    }
  }
  else
  {
    steering::SetNormalized(0.0f);
  }
}

void PulseHandler(void)
{
  if(GPIOIntStatus(GPIOM_BASE, true) & GPIO_INT_PIN_6)
  {
    GPIOIntClear(GPIOM_BASE, GPIO_INT_PIN_6);
    static uint32_t lastval = TIMER5->TAV;
    
    if(GPIOPinRead(GPIOM_BASE, GPIO_INT_PIN_6))
    {
      lastval = TIMER5->TAV;
    }
    else
    {
      accel_pulse_width = (float) (lastval - TIMER5->TAV) / 16000000;
    }
  }
  else if(GPIOIntStatus(GPIOM_BASE, true) & GPIO_INT_PIN_7)
  {
    GPIOIntClear(GPIOM_BASE, GPIO_INT_PIN_7);
    static uint32_t lastval = TIMER5->TAV;
    
    if(GPIOPinRead(GPIOM_BASE, GPIO_INT_PIN_7))
    {
      lastval = TIMER5->TAV;
    }
    else
    {
      steer_pulse_width = (float) (lastval - TIMER5->TAV) / 16000000;
    }
  }
}
void SteerPulseHandler(void)
{
  GPIOIntClear(GPIOM_BASE, GPIO_INT_PIN_7);
}

void modeSelectHandler(void)
{
	
	GPIOIntClear(GPIOC_AHB_BASE, GPIO_INT_PIN_4);
	
	if(GPIOPinRead(GPIOC_AHB_BASE, GPIO_INT_PIN_4))
	{
		GPIOPinWrite(GPIOC_AHB_BASE,GPIO_INT_PIN_5,0xFF);
	}
	else
	{
		GPIOPinWrite(GPIOC_AHB_BASE,GPIO_INT_PIN_5,0x00);
	}
	
	
}

void modeSelectSetup(void)
{
	//Mode select interrupt/switch
	GPIOPinTypeGPIOInput(GPIOC_AHB_BASE, GPIO_PIN_4);
	GPIOPinTypeGPIOOutput(GPIOC_AHB_BASE, GPIO_PIN_5);
	GPIOPadConfigSet(GPIOC_AHB_BASE,GPIO_PIN_4,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
	GPIOPadConfigSet(GPIOC_AHB_BASE,GPIO_PIN_5,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD);
  GPIOIntTypeSet(GPIOC_AHB_BASE, GPIO_PIN_4, GPIO_BOTH_EDGES);
  GPIOIntRegister(GPIOC_AHB_BASE, modeSelectHandler);
	
	if(GPIOPinRead(GPIOC_AHB_BASE, GPIO_INT_PIN_4))
	{
		GPIOPinWrite(GPIOC_AHB_BASE,GPIO_INT_PIN_5,0xFF);
	}
	else
	{
		GPIOPinWrite(GPIOC_AHB_BASE,GPIO_INT_PIN_5,0x00);
	}
	
  GPIOIntEnable(GPIOC_AHB_BASE, GPIO_PIN_4);
}

void watchdogHandler(void)
{
	GPIOPinWrite(GPIOC_AHB_BASE,GPIO_INT_PIN_5,0x00);
}

void watchdogSetup(void)
{
	
	WatchdogStallEnable(WATCHDOG0_BASE); //for debugging only
	WatchdogReloadSet(WATCHDOG0_BASE,0xFFFFFF);
	WatchdogResetDisable(WATCHDOG0_BASE);
	WatchdogIntTypeSet(WATCHDOG0_BASE,WATCHDOG_INT_TYPE_INT);
	WatchdogIntRegister(WATCHDOG0_BASE,watchdogHandler);
	WatchdogIntEnable(WATCHDOG0_BASE);
	WatchdogEnable(WATCHDOG0_BASE);
}

int main(void)
{
  FPUEnable();
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C4);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
  
  // Steering PWM 
  GPIOPinTypePWM(GPIOF_AHB_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinConfigure(GPIO_PF2_M0PWM2);
  GPIOPinConfigure(GPIO_PF3_M0PWM3);
	
	// Braking PWM
	GPIOPinTypePWM(GPIOG_AHB_BASE, GPIO_PIN_1);
	GPIOPinConfigure(GPIO_PF1_M0PWM1);
	GPIOPinConfigure(GPIO_PG1_M0PWM5);
  
  // Buttons
  /*
  GPIOPinTypeGPIOInput(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPadConfigSet(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOIntRegister(GPIOJ_AHB_BASE, SwitchHandler);
  GPIOIntTypeSet(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1, GPIO_BOTH_EDGES);
  GPIOIntEnable(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  */
  
  // CAN0 on GPIOA
  GPIOPinTypeCAN(GPIOA_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinConfigure(GPIO_PA0_CAN0RX);
  GPIOPinConfigure(GPIO_PA1_CAN0TX);
  // CAN1 on GPIOB
  GPIOPinTypeCAN(GPIOB_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinConfigure(GPIO_PB0_CAN1RX);
  GPIOPinConfigure(GPIO_PB1_CAN1TX);
  
  /*
  // PID timer
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
  TimerIntRegister(TIMER0_BASE, TIMER_A, UpdatePID);
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  TimerLoadSet(TIMER0_BASE, TIMER_A, 16000000*STEERING_SAMPLE_TIME);
  TimerEnable(TIMER0_BASE, TIMER_A);
  */
  
  // Pulse Timer
  TimerConfigure(TIMER5_BASE, TIMER_CFG_PERIODIC);
  TimerLoadSet(TIMER5_BASE, TIMER_A, 0xFFFFFF);
  TimerControlStall(TIMER5_BASE, TIMER_A, true);
  TimerEnable(TIMER5_BASE, TIMER_A);
  
  // GPIOM6 is accel pulse, GPIOM7 is steer pulse
  GPIOPinTypeGPIOInput(GPIOM_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOIntTypeSet(GPIOM_BASE, GPIO_PIN_6 | GPIO_PIN_7, GPIO_BOTH_EDGES);
  GPIOIntRegister(GPIOM_BASE, PulseHandler);
  GPIOIntEnable(GPIOM_BASE, GPIO_INT_PIN_6 | GPIO_INT_PIN_7);
	
	modeSelectSetup();
	
	//watchdogSetup();
   
  steering::Init();
  accelerator::Init();
	brakes::Init();
  
  //can_Init();
  
  IntMasterEnable();
  
  //uint16_t count = 0; 
  while(true)
  {
    for(volatile int i = 0 ; i < 1000 ; i ++);
    
    UpdateAcceleration(accel_pulse_width);
    UpdateSteering(steer_pulse_width);
  }
}
