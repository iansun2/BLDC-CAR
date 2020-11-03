#include "config.h"

//setup//=========================================================================================

void setup() {
  Serial.begin(115200);    //common init
  Serial.println("start init");
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
  l_esc.writeMicroseconds(1472);
  r_esc.writeMicroseconds(1472);
  delay(3000);
  Serial.println("task starts");
  xTaskCreate(Task_pwm_input, "pwm_input", 128 ,NULL, 2, &pwm_input_handle);   //task create
  xTaskCreate(Task_wpshutdown_input, "wpshutdown_input", 128 ,NULL, 1, &wpshutdown_input_handle);
  
  xTaskCreate(Task_wppwm_output, "wppwm_output", 128 ,NULL, 1, &wppwm_output_handle);
  xTaskCreate(Task_pwm_output, "pwm_output", 128 ,NULL, 1, &pwm_output_handle);
}

//pwm_input//=========================================================================================================================

void Task_pwm_input(void *pvParameters){
  (void) pvParameters;
  unsigned long rd_high;
  unsigned long last_average = 1472;
  int current_read;
  while(1){ 
    //pwm_in//------------------------------------------------------------
    
    //lpwm//
    //rd_high = pulseIn(lpwm_input_pin,1,pulse_timeout);    //read left pwm input 
    rd_high = pulseIn(lpwm_input_pin,1,pulse_timeout);
    
    if(rd_high >= 1700){                    //if left positive
      l_pwm = map(rd_high,1700,1900,1600,2000);
    }else if(rd_high <= 1300){              //if left negative
      l_pwm = map(rd_high,1050,1300,1000,1400);
    }else{
      l_pwm = 1472;
    }  
    //Serial.print("l_rd_high:");
    //Serial.println(rd_high);
     
    //rpwm_in//
    //rd_high = pulseIn(rpwm_input_pin,1,pulse_timeout);    //read right pwm input 
    rd_high = pulseIn(rpwm_input_pin,1,pulse_timeout);
    
    if(rd_high){
      if(rd_high <= 1300){              //if right positive
        r_pwm = map(rd_high,1050,1300,2000,1600);
      }else if(rd_high >= 1700){                    //if right negative
        r_pwm = map(rd_high,1700,1900,1400,1000);
      }else{
        r_pwm = 1472;
      }
    }
    //Serial.print("r_rd_high:");
    //Serial.println(rd_high);

    //wppwm_in//
    wp_read = pulseIn(wpshutdown_input_pin,1,pulse_timeout);   //read weapon pwm input 

    //current sensor input//---------------------------------------------------
    
    //lcurrent in//
      current_read = analogRead(lcurrent_input_pin);
      if(current_read >= wheel_current_max){
        l_pwm_max = (l_pwm_max - 1472) / 2 + 1472;
      }else if(l_pwm_max <= 1980){
        l_pwm_max += 20;
      }
      
      //rcurrent in//
      current_read = analogRead(rcurrent_input_pin);
      if(current_read >= wheel_current_max){
        r_pwm_max = (r_pwm_max - 1472) / 2 + 1472;
      }else if(r_pwm_max <= 1980){
        r_pwm_max += 20;
      }
      
      //wpcurrent in//
      current_read = analogRead(wpcurrent_input_pin);
      if(current_read >= weapon_current_max){
        wp_pwm = (wp_pwm - 1000) / 2 + 1000 ;
      }else if(wp_pwm <= 1990){
        wp_pwm += 10;
      }
      vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}

//wpshutdown//===========================================================================================================================================

void Task_wpshutdown_input(void *pvParameters){
    (void) pvParameters;
    bool last_status = 0;   //0:ok 1:error
    unsigned long last_average = 1000;
    while(1){
      last_average = wp_read;
      //Serial.println(last_average);
      if(last_average < 700){     //if disconnect
         vTaskSuspend(wppwm_output_handle);
         vTaskSuspend(pwm_output_handle);
        while(last_average < 700){
          last_average = wp_read;
          wp_esc.writeMicroseconds(1000);
          l_esc.writeMicroseconds(1472);
          r_esc.writeMicroseconds(1472);
          last_status = 1;
          Serial.println(last_average);
          Serial.println("wp_no_signal_shutdown");
          vTaskDelay( 150 / portTICK_PERIOD_MS );
        }
      }
      if(last_average > 1250){      //if shutdown
        vTaskSuspend(wppwm_output_handle);
        while(last_average > 1250){
          last_average = wp_read;
          wp_esc.writeMicroseconds(1000);
          last_status = 1;
          Serial.println(last_average);
          Serial.println("wp_shutdown");
          vTaskDelay( 150 / portTICK_PERIOD_MS );
        }
      }
      if(last_status){    //if status ok
        Serial.println("wp_start");
        last_status = 0;
        wp_pwm = 1000;
        vTaskResume(wppwm_output_handle);
        vTaskResume(pwm_output_handle);
      }
      vTaskDelay( 150 / portTICK_PERIOD_MS );
    }
}

//pwm_output//==================================================================================================================================================

void Task_pwm_output(void *pvParameters){
  (void) pvParameters;
  int lpwm_last = 1472, rpwm_last = 1472, lpwm_now = 1472, rpwm_now = 1472;
  while(1){
    
    //lpwm//---------------------------------
    lpwm_now = l_pwm;
    
    if(abs(lpwm_now-1472) > l_pwm_max-1472){
      if((lpwm_now-1472) > 0){
        lpwm_now = l_pwm_max;
      }else if((lpwm_now-1472) < 0){
        lpwm_now = 1472 - (l_pwm_max - 1472);
      }
    }
    
    if(lpwm_now - lpwm_last > 20){
      if(lpwm_last < 1750 && lpwm_last > 1250){
        lpwm_now = lpwm_last+20;
      }
    }else if(lpwm_now - lpwm_last < -20){
      if(lpwm_last < 1750 && lpwm_last > 1250){
        lpwm_now = lpwm_last-20;
      }
    }
    //Serial.print("l_pwm_max: ");
    //Serial.println(l_pwm_max);
    //Serial.print("l_pwm: ");
    //Serial.println(l_pwm);
    //Serial.print("l_pwm_now: ");
    //Serial.println(lpwm_now);
    
    l_esc.writeMicroseconds(lpwm_now);
    lpwm_last = lpwm_now;

    //rpwm//---------------------------------
    rpwm_now = r_pwm;
    
    if(abs(rpwm_now-1472) > r_pwm_max-1472){
      if((rpwm_now-1472) > 0){
        rpwm_now = r_pwm_max;
      }else if((rpwm_now-1472) < 0){
        rpwm_now = 1472 - (r_pwm_max - 1472);
      }
    }
    
    if(rpwm_now - rpwm_last > 20){
      if(rpwm_last < 1750 && rpwm_last > 1250){
        rpwm_now = rpwm_last+20;
      }
    }else if(rpwm_now - rpwm_last < -20){
      if(rpwm_last < 1750 && rpwm_last > 1250){
        rpwm_now = rpwm_last-20;
      }
    }
    //Serial.print("r_pwm_max: ");
    //Serial.println(r_pwm_max);
    //Serial.print("r_pwm: ");
    //Serial.println(r_pwm);
    //Serial.print("r_pwm_now: ");
    //Serial.println(rpwm_now);
    
    r_esc.writeMicroseconds(rpwm_now);
    rpwm_last = rpwm_now;
    
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}
//wppwm_output//============================================================================================================================================
void Task_wppwm_output(void *pvParameters){
  (void) pvParameters;
  vTaskDelay( 500 / portTICK_PERIOD_MS );
  while(1){
    Serial.print("wp_pwm: ");
    Serial.println(wp_pwm);
    wp_esc.writeMicroseconds(abs(wp_pwm));
    vTaskDelay( 100 / portTICK_PERIOD_MS );
  }
}


void loop() {}
