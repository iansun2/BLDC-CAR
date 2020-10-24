#include <Arduino_FreeRTOS.h>
#include <Servo.h>

#define lpwm_input_pin 2    //pwm input
#define rpwm_input_pin 3
#define wpcurrent_input_pin A0    //current input
#define lcurrent_input_pin A1
#define rcurrent_input_pin A2
#define wpshutdown_input_pin 4       //weapon shutdown input
#define wppwm_output_pin 8   //pwm output  
#define lpwm_output_pin 9
#define rpwm_output_pin 10

#define pulse_timeout 100000
#define wheel_current_max 300
#define weapon_current_max 500

TaskHandle_t lpwm_input_handle;   //pwm input
TaskHandle_t rpwm_input_handle;
TaskHandle_t current_input_handle;   //current sensor input
TaskHandle_t wpshutdown_input_handle;   //weapon shutdown input & ctrl

TaskHandle_t lpwm_output_handle;     //pwm output  
TaskHandle_t rpwm_output_handle;
TaskHandle_t wppwm_output_handle;    


void Task_lpwm_input( void *pvParameters );   //pwm input
void Task_rpwm_input( void *pvParameters );
void Task_current_input( void *pvParameters );     //current sensor input
void Task_wpshutdown_input( void *pvParameters );   //weapon shutdown input & ctrl

void Task_lpwm_output( void *pvParameters );    //pwm output   
void Task_rpwm_output( void *pvParameters );
void Task_wppwm_output( void *pvParameters );   


Servo l_esc;
Servo r_esc;
Servo wp_esc;

int l_pwm = 1000,r_pwm = 1000,wp_pwm = 1000;
int l_pwm_max = 2000,r_pwm_max = 2000;
