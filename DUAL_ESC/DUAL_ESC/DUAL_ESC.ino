#include "config.h"

//setup//=========================================================================================

void setup() {
  Serial.begin(115200);    //common init
  DDRB = B11111000;
  pinMode(lpwm_input_pin,INPUT);
  pinMode(rpwm_input_pin,INPUT);
  pinMode(wpcurrent_input_pin,INPUT);
  pinMode(lcurrent_input_pin,INPUT);
  pinMode(rcurrent_input_pin,INPUT);
  pinMode(wpshutdown_input_pin,INPUT);
  
  wp_esc.attach(wppwm_output_pin);   //esc init
  l_esc.attach(lpwm_output_pin);
  r_esc.attach(rpwm_output_pin);
  
  wp_esc.writeMicroseconds(1000);
  l_esc.writeMicroseconds(1460);
  r_esc.writeMicroseconds(1460);
  delay(5000);

  xTaskCreate(Task_lpwm_input, "lpwm_input", 128 ,NULL, 2, &lpwm_input_handle);   //task create
  xTaskCreate(Task_rpwm_input, "rpwm_input", 128 ,NULL, 2, &rpwm_input_handle);
  xTaskCreate(Task_current_input, "current_input", 128 ,NULL, 1, &current_input_handle);
  xTaskCreate(Task_wpshutdown_input, "current_input", 128 ,NULL, 1, &wpshutdown_input_handle);
  
  xTaskCreate(Task_wppwm_output, "wppwm_output", 128 ,NULL, 1, &wppwm_output_handle);
  xTaskCreate(Task_lpwm_output, "lpwm_output", 128 ,NULL, 1, &lpwm_output_handle);
  xTaskCreate(Task_rpwm_output, "rpwm_output", 128 ,NULL, 1, &rpwm_output_handle);
}

//pwm_input//=========================================================================================

void Task_lpwm_input(void *pvParameters){
  (void) pvParameters;
  unsigned long rd_high;
  unsigned long last_average = 1460;
  while(1){ 
    rd_high = pulseIn(lpwm_input_pin,1,pulse_timeout);    //read left pwm input 
    last_average *= 5;        //noise filter
    last_average += rd_high;
    last_average /= 6;
    rd_high = last_average;
    if(rd_high >= 1700){                    //if left positive
      l_pwm = map(rd_high,1700,1900,1600,2000);
    }else if(rd_high <= 1300){              //if left negative
      l_pwm = map(rd_high,1050,1300,1000,1400);
    }else{
      l_pwm = 1460;
    }  
    //Serial.print("l_rd_high:");
    //Serial.println(rd_high); 
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}
//-------------------------------------------------------------------
void Task_rpwm_input(void *pvParameters){
  (void) pvParameters;
  unsigned long rd_high;
  unsigned long last_average = 1460;
  while(1){
    rd_high = pulseIn(rpwm_input_pin,1,pulse_timeout);    //read right pwm input 
    last_average *= 5;        //noise filter
    last_average += rd_high;
    last_average /= 6;
    rd_high = last_average;
    if(rd_high){
      if(rd_high <= 1300){              //if right positive
        r_pwm = map(rd_high,1050,1300,2000,1600);
      }else if(rd_high >= 1700){                    //if right negative
        r_pwm = map(rd_high,1700,1900,1400,1000);
      }else{
        r_pwm = 1460;
      }
    }
    Serial.print("r_rd_high:");
    Serial.println(rd_high);
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}
//current sensor input//========================================================================

void Task_current_input(void *pvParameters){
    (void) pvParameters;
    int current_read;
    
    while(1){
      current_read = analogRead(lcurrent_input_pin);
      if(current_read >= wheel_current_max){
        l_pwm_max = (l_pwm_max - 1000) / 2 + 1000;
      }else if(l_pwm_max <= 1980){
        l_pwm_max += 20;
      }
      //..............................................................
      current_read = analogRead(rcurrent_input_pin);
      if(current_read >= wheel_current_max){
        r_pwm_max = (r_pwm_max - 1000) / 2 + 1000;
      }else if(r_pwm_max <= 1980){
        r_pwm_max += 20;
      }
      //..............................................................
      current_read = analogRead(wpcurrent_input_pin);
      if(current_read >= weapon_current_max){
        wp_pwm = (wp_pwm - 1000) / 2 + 1000 ;
      }else if(wp_pwm <= 1990){
        wp_pwm += 10;
      }
      vTaskDelay( 50 / portTICK_PERIOD_MS );
    }
}
//wpshutdown//=====================================================================================

void Task_wpshutdown_input(void *pvParameters){
    (void) pvParameters;
    bool last_status = 0;
    unsigned long last_average = 1000;
    while(1){
      last_average = (pulseIn(wpshutdown_input_pin,1,pulse_timeout) + (last_average * 5)) / 6;
      //Serial.println(last_average);
      if(last_average < 775){     //if disconnect
         vTaskSuspend(wppwm_output_handle);
         vTaskSuspend(lpwm_output_handle);
         vTaskSuspend(rpwm_output_handle);
        while(last_average < 775){
          last_average = (pulseIn(wpshutdown_input_pin,1,pulse_timeout) + (last_average * 5)) / 6;
          wp_esc.writeMicroseconds(1000);
          l_esc.writeMicroseconds(1460);
          r_esc.writeMicroseconds(1460);
          last_status = 1;
          //Serial.println(last_average);
          //Serial.println("wp_no_signal_shutdown");
          vTaskDelay( 100 / portTICK_PERIOD_MS );
        }
      }
      if(last_average > 1250){      //if shutdown
        vTaskSuspend(wppwm_output_handle);
        while(last_average > 1250){
          last_average = (pulseIn(wpshutdown_input_pin,1,pulse_timeout) + (last_average * 5)) / 6;
          wp_esc.writeMicroseconds(1000);
          last_status = 1;
          //Serial.println(last_average);
          //Serial.println("wp_shutdown");
          vTaskDelay( 100 / portTICK_PERIOD_MS );
        }
      }
      if(last_status){    //if status ok
        //Serial.println("wp_start");
        last_status = 0;
        wp_pwm = 1000;
        vTaskResume(wppwm_output_handle);
        vTaskResume(lpwm_output_handle);
        vTaskResume(rpwm_output_handle);
      }
      vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}

//pwm_output//=========================================================================================

void Task_lpwm_output(void *pvParameters){
  (void) pvParameters;
  while(1){
    if(l_pwm > l_pwm_max){
      l_pwm = l_pwm_max;
    }
    //Serial.print("l_pwm_max: ");
    //Serial.println(l_pwm_max);
    //Serial.print("l_pwm: ");
    //Serial.println(l_pwm);
    l_esc.writeMicroseconds(l_pwm);
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}
//----------------------------------------------------------------------------------
void Task_rpwm_output(void *pvParameters){
  (void) pvParameters;
  while(1){
    if(r_pwm > r_pwm_max){
      r_pwm = r_pwm_max;
    }
    //Serial.print("r_pwm_max: ");
    //Serial.println(r_pwm_max);
    Serial.print("r_pwm: ");
    Serial.println(r_pwm);
    r_esc.writeMicroseconds(r_pwm);
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}

//----------------------------------------------------------------------------------------------
void Task_wppwm_output(void *pvParameters){
  (void) pvParameters;
  vTaskDelay( 500 / portTICK_PERIOD_MS );
  while(1){
    //Serial.print("wp_pwm: ");
    //Serial.println(wp_pwm);
    wp_esc.writeMicroseconds(abs(wp_pwm));
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}


void loop() {}
