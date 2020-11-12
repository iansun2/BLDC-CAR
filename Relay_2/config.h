#include <Arduino_FreeRTOS.h>
#include <Servo.h>

#define xpwm_input_pin 2    //pwm input
#define ypwm_input_pin 3
#define wpcurrent_input_pin A0    //current input
#define lcurrent_input_pin A1
#define rcurrent_input_pin A2
#define wpshutdown_input_pin 4       //weapon shutdown input
#define wppwm_output_pin 8   //pwm output  
#define lpwm_output_pin 9
#define rpwm_output_pin 10
#define l_relay_pin 11
#define r_relay_pin 12

#define pulse_timeout 50000
#define wheel_current_max 160   //40A *4
#define weapon_current_max 280    //70A *4

#define wheel_stop_pwm 1000

TaskHandle_t pwm_input_handle;   //pwm input
TaskHandle_t wpshutdown_handle;   //weapon shutdown input & ctrl

TaskHandle_t lpwm_output_handle;     //lpwm output  
TaskHandle_t rpwm_output_handle;     //rpwm output  

void Task_pwm_input( void *pvParameters );   //pwm input
void Task_wpshutdown( void *pvParameters );   //weapon shutdown input & ctrl

void Task_lpwm_output( void *pvParameters );    //lpwm output   
void Task_rpwm_output( void *pvParameters );    //rpwm output   

Servo l_esc;
Servo r_esc;
Servo wp_esc;

int l_pwm = 1000,r_pwm = 1000,wp_pwm = 1000;
int l_pwm_max = 2000,r_pwm_max = 2000;
int wp_read = 1000;
