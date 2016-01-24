
extern "C"
{
#include "can.h"
}

#include <tm4c1294ncpdt.h>
#define PART_TM4C1294NCPDT
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/pwm.h>
#include <driverlib/fpu.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>

void SetSteerPWM(float duty_cycle)
{
  const uint32_t pwm_clk_period = 8000;
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, pwm_clk_period);
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, pwm_clk_period*(1-duty_cycle)); // Yellow wire (F2)
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm_clk_period*duty_cycle); // Green wire (F3)
}
void InitSteerPWM()
{
  PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
  SetSteerPWM(0.50f);
  PWMGenEnable(PWM0_BASE, PWM_GEN_1);
  PWMOutputState(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
  PWMOutputInvert(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
}

void SetSteerForce(float value)
{
  SetSteerPWM(value * 0.2f + 0.5f);
}

float target_steering_angle = 0.0f;

void SwitchHandler(void)
{
  GPIOIntClear(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  uint8_t data = GPIOPinRead(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  if((data & 0x2) == 0)
  {
    target_steering_angle = 0.5f;
  }
  else if((data & 0x1) == 0)
  {
    target_steering_angle = -0.5f;
  }
  else
  {
    target_steering_angle = 0.0f;
  }
}

static const float STEERING_SAMPLE_TIME = .005;

//                              ----------------     -----------------
// Target Steer Angle --> E --> |PID Controller| --> |Steering System| --> Actual Steer Angle
//                        ^     ----------------     -----------------  |
//                        |                                             |
//                        +---------------------------------------------+

float k_p = 0.2;
float k_i = 0.2;
float k_d = 0.2;
float Ts = STEERING_SAMPLE_TIME;

float a = k_p + k_i*Ts/2.0f + k_d/Ts;
float b = -k_p + k_i*Ts/2.0f - 2.0f*k_d/Ts;
float c = k_d/Ts;

float errors[3] = { 0.0f, 0.0f, 0.0f };
unsigned int error_idx = 0;
float forces[2] = { 0.0f, 0.0f };
unsigned int force_idx = 0;

#define IDX_MOD(arr, idx) ((arr)[((idx) + sizeof(arr)/sizeof(*(arr))) % sizeof(arr)/sizeof(*(arr))])

void UpdatePID()
{
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  
  // PID input
  error_idx ++; if(error_idx == 3) error_idx = 0;
  float * e_0 = &IDX_MOD(errors, error_idx - 0);
  float * e_1 = &IDX_MOD(errors, error_idx - 1);
  float * e_2 = &IDX_MOD(errors, error_idx - 2);
  
  *e_0 = target_steering_angle - can_steering_angle;
  
  // PID output
  force_idx ++; if(force_idx == 2) force_idx = 0;
  float * f_0 = &IDX_MOD(forces, force_idx - 0);
  float * f_1 = &IDX_MOD(forces, force_idx - 1);
  
  *f_0 = *f_1 + a*(*e_0) + b*(*e_1) + c*(*e_2);
  
  SetSteerForce(*f_0);
}

int main(void)
{
  FPUEnable();
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  
  GPIOPinTypePWM(GPIOF_AHB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinConfigure(GPIO_PF2_M0PWM2);
  GPIOPinConfigure(GPIO_PF3_M0PWM3);
  
  GPIOPinTypeGPIOInput(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPadConfigSet(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOIntRegister(GPIOJ_AHB_BASE, SwitchHandler);
  GPIOIntTypeSet(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1, GPIO_BOTH_EDGES);
  GPIOIntEnable(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  //GPIOPinTypeCAN(GPIOA_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  //GPIOPinConfigure(GPIO_PA0_CAN0RX);
  //GPIOPinConfigure(GPIO_PA1_CAN0TX);
  
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
  TimerIntRegister(TIMER0_BASE, TIMER_A, UpdatePID);
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  TimerLoadSet(TIMER0_BASE, TIMER_A, 16000000*STEERING_SAMPLE_TIME);
  TimerEnable(TIMER0_BASE, TIMER_A);
  
  InitSteerPWM();
  can_Init();
  
  IntMasterEnable();
  
  while(1) {}
}
