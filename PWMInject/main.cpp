
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
    //SetVoltage(0.39f + val * (0.6f - 0.39f));
  }
  void Init()
  {
    // I2C Stuff on K6(SCL) and K7(SDA)
    I2CMasterInitExpClk(I2C4_BASE, 16000000, true);
    GPIOPinTypeI2CSCL(GPIOK_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIOK_BASE, GPIO_PIN_7);
    GPIOPinConfigure(GPIO_PK6_I2C4SCL);
    GPIOPinConfigure(GPIO_PK7_I2C4SDA);
    
    //SetADCVoltage(0.0f);
  }
}

namespace brakes
{
  void SetPWM(float duty_cycle)
  {
  }
  volatile float brake_value = 0.0f; // For debugging
  void SetNormalized(float val)
  {
    brake_value = val;
  }
  void Init()
  {
  }
}

/*
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
*/

/*
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
*/

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

int main(void)
{
  FPUEnable();
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C4);
  
  // Steering PWM
  GPIOPinTypePWM(GPIOF_AHB_BASE, GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinConfigure(GPIO_PF2_M0PWM2);
  GPIOPinConfigure(GPIO_PF3_M0PWM3);
  
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
  
  steering::Init();
  accelerator::Init();
  brakes::Init();
  can_Init();
  
  IntMasterEnable();
  
  //uint16_t count = 0;
  while(true)
  {
    for(volatile int i = 0 ; i < 1000 ; i ++);
    
    UpdateAcceleration(accel_pulse_width);
    UpdateSteering(steer_pulse_width);
  }
}
