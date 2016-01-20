
#include <tm4c1294ncpdt.h>
#define PART_TM4C1294NCPDT
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/pwm.h>
#include <driverlib/fpu.h>

void set_pwm(float duty_cycle)
{
  const uint32_t pwm_clk_period = 8000;
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, pwm_clk_period);
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, pwm_clk_period*duty_cycle);
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, pwm_clk_period*(1-duty_cycle));
}
void init_pwm()
{
  PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
  set_pwm(0.50f);
  PWMGenEnable(PWM0_BASE, PWM_GEN_1);
  PWMOutputState(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
  PWMOutputInvert(PWM0_BASE, (PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);
}

float target_duty_cycle = 0.50f;
float duty_cycle = 0.50f;

void switch_handler(void)
{
  GPIOIntClear(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  uint8_t data = GPIOPinRead(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  if((data & 0x2) == 0)
  {
    target_duty_cycle = 0.4f;
  }
  else if((data & 0x1) == 0)
  {
    target_duty_cycle = 0.6f;
  }
  else
  {
    target_duty_cycle = 0.5f;
  }
}

int main(void)
{
  FPUEnable();
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  
  GPIOPinTypePWM(GPIOF_AHB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinConfigure(GPIO_PF2_M0PWM2);
  GPIOPinConfigure(GPIO_PF3_M0PWM3);
  
  GPIOPinTypeGPIOInput(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPadConfigSet(GPIOJ_AHB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOIntRegister(GPIOJ_AHB_BASE, switch_handler);
  GPIOIntTypeSet(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1, GPIO_BOTH_EDGES);
  GPIOIntEnable(GPIOJ_AHB_BASE, GPIO_INT_PIN_0 | GPIO_INT_PIN_1);
  
  init_pwm();
  
  while(1)
  {
    for(uint32_t i = 0 ; i < 1000 ; i ++) {}
    
    duty_cycle += (target_duty_cycle - duty_cycle) * 0.002f;
    set_pwm(duty_cycle);
  }
}
