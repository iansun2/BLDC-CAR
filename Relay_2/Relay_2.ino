#include "config.h"

//setup//=========================================================================================

void setup() {
  Serial.begin(115200);    //common init
  Serial.println("start init");
  DDRB = B11111000;
  pinMode(ypwm_input_pin,INPUT);
  pinMode(xpwm_input_pin,INPUT);
  pinMode(wpcurrent_input_pin,INPUT);
  pinMode(lcurrent_input_pin,INPUT);
  pinMode(rcurrent_input_pin,INPUT);
  pinMode(wpshutdown_input_pin,INPUT);
  
  wp_esc.attach(wppwm_output_pin);   //esc init
  l_esc.attach(lpwm_output_pin);
  r_esc.attach(rpwm_output_pin);
  
  wp_esc.writeMicroseconds(1000);
  l_esc.writeMicroseconds(wheel_stop_pwm);
  r_esc.writeMicroseconds(wheel_stop_pwm);
  delay(3000);
  while(pulseIn(wpshutdown_input_pin,1,pulse_timeout) < 1850 || pulseIn(wpshutdown_input_pin,1,pulse_timeout) > 2150){
    Serial.println("Controller no connect");
  }
  Serial.println("task starts");
  xTaskCreate(Task_pwm_input, "pwm_input", 128 ,NULL, 2, &pwm_input_handle);   //task create
  xTaskCreate(Task_wpshutdown_input, "wpshutdown_input", 128 ,NULL, 0, &wpshutdown_input_handle);

  xTaskCreate(Task_switch, "switch", 128 ,NULL, 1, &switch_handle);
  xTaskCreate(Task_pwm_output, "pwm_output", 128 ,NULL, 0, &pwm_output_handle);
}

//pwm_input//=========================================================================================================================

void Task_pwm_input(void *pvParameters){
  (void) pvParameters;
  unsigned long xrd_high,yrd_high;
  unsigned long last_average = wheel_stop_pwm;
  int current_read;
  while(1){ 
    //pwm_in//------------------------------------------------------------
    
    //wheel_pwm//
    yrd_high = pulseIn(ypwm_input_pin,1,pulse_timeout);    //read wheel pwm input 
    xrd_high = pulseIn(xpwm_input_pin,1,pulse_timeout);
    
    if(yrd_high <= 1300 && xrd_high <= 1700 && xrd_high >= 1300){                 //front only
      l_pwm = map(yrd_high,1300,1050,1600,2000);
      r_pwm = map(yrd_high,1300,1050,1600,2000);
    }else if(xrd_high >= 1700 && yrd_high <= 1700 && yrd_high >= 1300){           //right only
      l_pwm = map(xrd_high,1700,1900,1600,2000);
      r_pwm = map(xrd_high,1700,1900,1400,1000);
    }else if(xrd_high <= 1300 && yrd_high <= 1700 && yrd_high >= 1300){           //left only
      l_pwm = map(xrd_high,1300,1050,1400,1000);
      r_pwm = map(xrd_high,1300,1050,1600,2000);
    }else if(yrd_high >= 1700 && xrd_high <= 1700 && xrd_high >= 1300){           //back only
      l_pwm = map(yrd_high,1700,1900,1400,1000);
      r_pwm = map(yrd_high,1700,1900,1400,1000);
    }else{
      if(yrd_high <= 1300 && xrd_high >= 1700){                                   //front right
        l_pwm = 2000;
        r_pwm = map(xrd_high,1700,1900,2000,1600);
      }else if(yrd_high <= 1300 && xrd_high <= 1300){                             //front left
        l_pwm = map(xrd_high,1300,1050,2000,1600);
        r_pwm = 2000;
      }else if(yrd_high >= 1700 && xrd_high >= 1700){                             //back right
        l_pwm = 1000;
        r_pwm = map(xrd_high,1700,1900,1000,1300);
      }else if(yrd_high >= 1700 && xrd_high <= 1300){                             //back left
        l_pwm = map(xrd_high,1300,1050,1000,1300);
        r_pwm = 1000;
      }else{
        r_pwm = wheel_stop_pwm;
        l_pwm = wheel_stop_pwm;
      }
    }

    //wppwm_in//
    wp_read = pulseIn(wpshutdown_input_pin,1,pulse_timeout);   //read weapon pwm input 

    //current sensor input//---------------------------------------------------
    
    //lcurrent in//
      current_read = abs(analogRead(lcurrent_input_pin) - 512);
      if(current_read >= wheel_current_max){
        l_pwm_max = (l_pwm_max - wheel_stop_pwm) / 2 + wheel_stop_pwm;
      }else if(l_pwm_max <= 1980){
        l_pwm_max += 20;
      }
      
      //rcurrent in//
      current_read = abs(analogRead(rcurrent_input_pin) - 512);
      if(current_read >= wheel_current_max){
        r_pwm_max = (r_pwm_max - wheel_stop_pwm) / 2 + wheel_stop_pwm;
      }else if(r_pwm_max <= 1980){
        r_pwm_max += 20;
      }
      
      //wpcurrent in//
      current_read = abs(analogRead(wpcurrent_input_pin) - 512);
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
         wp_shutdown = 1;
         vTaskSuspend(pwm_output_handle);
        while(last_average < 700){
          last_average = wp_read;
          wp_esc.writeMicroseconds(1000);
          l_esc.writeMicroseconds(wheel_stop_pwm);
          r_esc.writeMicroseconds(wheel_stop_pwm);
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

//switch//=========================================================================================

void Task_switch(void *pvParameters){
  (void) pvParameters;
  bool l_pwm_last = 0;    //0:positive 1:negative
  bool l_pwm_last = 0;    //0:positive 1:negative
  
  while(1){
    if(l_pwm < 0 && !l_pwm_last){   //left positive to negative
        vTaskSuspend(lpwm_output_handle);
        l_esc.writeMicroseconds(1000);
        digitalWrite(lrelay_pin,1);
        l_pwm_last = 1;
        Serial.println("left positive to negative");
        vTaskDelay( 50 / portTICK_PERIOD_MS );
        vTaskResume(lpwm_output_handle);
        
    }else if(l_pwm > 0 && l_pwm_last){   //left negative to positive
        vTaskSuspend(lpwm_output_handle);
        l_esc.writeMicroseconds(1000);
        digitalWrite(lrelay_pin,0);
        l_pwm_last = 0;
        Serial.println("left negative to positive");
        vTaskDelay( 50 / portTICK_PERIOD_MS );
        vTaskResume(lpwm_output_handle);
    }
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}

//pwm_output//==================================================================================================================================================

void Task_pwm_output(void *pvParameters){
  (void) pvParameters;
  int lpwm_last = wheel_stop_pwm, rpwm_last = wheel_stop_pwm, lpwm_now = wheel_stop_pwm, rpwm_now = wheel_stop_pwm;
  while(1){
    
    //lpwm//----------------------------------------------------------
    lpwm_now = l_pwm;
    
    if(abs(lpwm_now-wheel_stop_pwm) > l_pwm_max-wheel_stop_pwm){
      if((lpwm_now-wheel_stop_pwm) > 0){
        lpwm_now = l_pwm_max;
      }else if((lpwm_now-wheel_stop_pwm) < 0){
        lpwm_now = wheel_stop_pwm - (l_pwm_max - wheel_stop_pwm);
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
    Serial.print("l_pwm_now: ");
    Serial.println(lpwm_now);
    
    l_esc.writeMicroseconds(lpwm_now);
    lpwm_last = lpwm_now;

    //rpwm//----------------------------------------------
    rpwm_now = r_pwm;
    
    if(abs(rpwm_now-wheel_stop_pwm) > r_pwm_max-wheel_stop_pwm){
      if((rpwm_now-wheel_stop_pwm) > 0){
        rpwm_now = r_pwm_max;
      }else if((rpwm_now-wheel_stop_pwm) < 0){
        rpwm_now = wheel_stop_pwm - (r_pwm_max - wheel_stop_pwm);
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
    Serial.print("r_pwm_now: ");
    Serial.println(rpwm_now);
    
    r_esc.writeMicroseconds(rpwm_now);
    rpwm_last = rpwm_now;

    //rpwm//------------------------------------------
    if(!wp_shutdown){
      wp_esc.writeMicroseconds(abs(wp_pwm));
    }
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}



void loop() {}
