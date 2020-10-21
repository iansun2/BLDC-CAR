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
  l_esc.writeMicroseconds(1000);
  r_esc.writeMicroseconds(1000);
  delay(5000);

  xTaskCreate(Task_lpwm_input, "lpwm_input", 128 ,NULL, 2, &lpwm_input_handle);   //task create
  xTaskCreate(Task_rpwm_input, "rpwm_input", 128 ,NULL, 2, &rpwm_input_handle);
  xTaskCreate(Task_current_input, "current_input", 128 ,NULL, 1, &current_input_handle);
  xTaskCreate(Task_wpshutdown_input, "current_input", 128 ,NULL, 1, &wpshutdown_input_handle);
  
  xTaskCreate(Task_lswitch, "lswitch", 128 ,NULL, 1, &lswitch_handle);
  xTaskCreate(Task_rswitch, "rswitch", 128 ,NULL, 1, &rswitch_handle);
  
  xTaskCreate(Task_wppwm_output, "wppwm_output", 128 ,NULL, 1, &wppwm_output_handle);
  xTaskCreate(Task_lpwm_output, "lpwm_output", 128 ,NULL, 1, &lpwm_output_handle);
  xTaskCreate(Task_rpwm_output, "rpwm_output", 128 ,NULL, 1, &rpwm_output_handle);
}

//pwm_input//=========================================================================================

void Task_lpwm_input(void *pvParameters){
  (void) pvParameters;
  unsigned long rd_high;
  unsigned long last_average = 1500;
  while(1){ 
    rd_high = pulseIn(lpwm_input_pin,1,pulse_timeout);    //read left pwm input 
    last_average *= 5;        //noise filter
    last_average += rd_high;
    last_average /= 6;
    rd_high = last_average;
    if(rd_high >= 1700){                    //if left positive
      l_pwm = map(rd_high,1700,1900,1000,2000);
    }else if(rd_high <= 1300){              //if left negative
      l_pwm = map(rd_high,1050,1300,-2000,-1000);
    }else{
      l_pwm = 1000;
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
  unsigned long last_average = 1500;
  while(1){
    rd_high = pulseIn(rpwm_input_pin,1,pulse_timeout);    //read right pwm input 
    last_average *= 5;        //noise filter
    last_average += rd_high;
    last_average /= 6;
    rd_high = last_average;
    if(rd_high){
      if(rd_high <= 1300){              //if right positive
        r_pwm = map(rd_high,1050,1300,2000,1000);
      }else if(rd_high >= 1700){                    //if right negative
        r_pwm = map(rd_high,1700,1900,-1000,-2000);
      }else{
        r_pwm = 1000;
      }
    }
    //Serial.print("r_rd_high:");
    //Serial.println(rd_high);
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
        wp_pwm = (wp_pwm - 900) / 2 + 900 ;
      }else if(wp_pwm <= 2180){
        wp_pwm += 20;
      }
      vTaskDelay( 50 / portTICK_PERIOD_MS );
    }
}
//wpshutdown//=====================================================================================

void Task_wpshutdown_input(void *pvParameters){
    (void) pvParameters;
    bool last_status = 0;
    while(1){
      while(pulseIn(wpshutdown_input_pin,1,pulse_timeout) < 1500){
        vTaskSuspend(wppwm_output_handle);
        wp_esc.writeMicroseconds(1000);
        last_status = 1;
        vTaskDelay( 50 / portTICK_PERIOD_MS );
      }
      if(last_status){
        last_status = 0;
        wp_pwm = 900;
        vTaskResume(wppwm_output_handle);
      }
      vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}
//switch//=========================================================================================

void Task_lswitch(void *pvParameters){
  (void) pvParameters;
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
//----------------------------------------------------------------------------------
void Task_rswitch(void *pvParameters){
  (void) pvParameters;
  bool r_pwm_last = 0;    //0:positive 1:negative
  
  while(1){
    if(r_pwm < 0 && !r_pwm_last){   //right positive to negative
        vTaskSuspend(rpwm_output_handle);
        r_esc.writeMicroseconds(1000);
        digitalWrite(rrelay_pin,1);
        r_pwm_last = 1;
        Serial.println("right positive to negative");
        vTaskDelay( 50 / portTICK_PERIOD_MS );
        vTaskResume(rpwm_output_handle);
        
    }else if(r_pwm > 0 && r_pwm_last){   //right negative to positive
        vTaskSuspend(rpwm_output_handle);
        r_esc.writeMicroseconds(1000);
        digitalWrite(rrelay_pin,0);
        r_pwm_last = 0;
        Serial.println("right negative to positive");
        vTaskDelay( 50 / portTICK_PERIOD_MS );
        vTaskResume(rpwm_output_handle);
    }
    vTaskDelay( 50 / portTICK_PERIOD_MS );
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
    Serial.print("l_pwm: ");
    Serial.println(l_pwm);
    l_esc.writeMicroseconds(abs(l_pwm));
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
    r_esc.writeMicroseconds(abs(r_pwm));
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}

//----------------------------------------------------------------------------------------------
void Task_wppwm_output(void *pvParameters){
  (void) pvParameters;
  while(1){
    //Serial.print("wp_pwm: ");
    //Serial.println(wp_pwm);
    wp_esc.writeMicroseconds(abs(wp_pwm));
    vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}


void loop() {}
