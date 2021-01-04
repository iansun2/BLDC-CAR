#include "config.h"

//setup//=========================================================================================

void setup() {
  Serial.begin(115200);    //common init
  Serial.println("start init");
  DDRB = B11111000;
  pinMode(ypwm_input_pin,INPUT);
  pinMode(xpwm_input_pin,INPUT);
  pinMode(lcurrent_input_pin,INPUT);
  pinMode(rcurrent_input_pin,INPUT);
  pinMode(shutdown_input_pin,INPUT);
  
  l_esc.attach(lpwm_output_pin);    //esc init
  r_esc.attach(rpwm_output_pin);
  
  l_esc.writeMicroseconds(wheel_stop_pwm);
  r_esc.writeMicroseconds(wheel_stop_pwm);
  delay(3000);
  while(pulseIn(shutdown_input_pin,1,pulse_timeout) < 1850 || pulseIn(shutdown_input_pin,1,pulse_timeout) > 2150){
    Serial.println("Controller no connect");
  }
  Serial.println("task starts");
  xTaskCreate(Task_pwm_input, "pwm_input", 128 ,NULL, 2, &pwm_input_handle);   //task create
  xTaskCreate(Task_shutdown, "shutdown", 128 ,NULL, 0, &shutdown_handle);

  xTaskCreate(Task_lpwm_output, "lpwm_output", 128 ,NULL, 1, &lpwm_output_handle);
  xTaskCreate(Task_rpwm_output, "rpwm_output", 128 ,NULL, 0, &rpwm_output_handle);
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
      l_pwm = map(yrd_high,1300,1050,1000,2000);
      r_pwm = map(yrd_high,1300,1050,1000,2000);
    }else if(xrd_high >= 1700 && yrd_high <= 1700 && yrd_high >= 1300){           //right only
      l_pwm = map(xrd_high,1700,1900,1000,2000);
      //r_pwm = map(xrd_high,1700,1900,-1000,-2000);
      r_pwm = -1300;
    }else if(xrd_high <= 1300 && yrd_high <= 1700 && yrd_high >= 1300){           //left only
      //l_pwm = map(xrd_high,1300,1050,-1000,-2000);
      l_pwm = -1300;
      r_pwm = map(xrd_high,1300,1050,1000,2000);
    }else if(yrd_high >= 1700 && xrd_high <= 1700 && xrd_high >= 1300){           //back only
      l_pwm = map(yrd_high,1700,1900,-1000,-2000);
      r_pwm = map(yrd_high,1700,1900,-1000,-2000);
    }else{
      if(yrd_high <= 1300 && xrd_high >= 1700){                                   //front right
        l_pwm = 2000;
        r_pwm = map(xrd_high,1700,1900,2000,1000);
      }else if(yrd_high <= 1300 && xrd_high <= 1300){                             //front left
        l_pwm = map(xrd_high,1300,1050,2000,1000);
        r_pwm = 2000;
      }else if(yrd_high >= 1700 && xrd_high >= 1700){                             //back right
        l_pwm = -2000;
        r_pwm = map(xrd_high,1700,1900,-2000,-1000);
      }else if(yrd_high >= 1700 && xrd_high <= 1300){                             //back left
        l_pwm = map(xrd_high,1300,1050,-2000,-1000);
        r_pwm = -2000;
      }else{
        r_pwm = wheel_stop_pwm;
        l_pwm = wheel_stop_pwm;
      }
    }

    //wppwm_in//
    wp_read = pulseIn(shutdown_input_pin,1,pulse_timeout);   //read weapon pwm input 

    //current sensor input//---------------------------------------------------
    /*
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
     */
      vTaskDelay( 50 / portTICK_PERIOD_MS );
  }
}

//wpshutdown//===========================================================================================================================================

void Task_shutdown(void *pvParameters){
    (void) pvParameters;
    bool shutdown_stats = 0;   //0:ok 1:error
    while(1){
      //Serial.println(wp_read);
      while(wp_read < 700){   //if disconnect
         shutdown_stats = 1;
         vTaskSuspend(lpwm_output_handle);
         vTaskSuspend(rpwm_output_handle);
         l_esc.writeMicroseconds(wheel_stop_pwm);
         r_esc.writeMicroseconds(wheel_stop_pwm);
         Serial.print("wp_no_signal_shutdown: ");
         Serial.println(wp_read);
         vTaskDelay( 200 / portTICK_PERIOD_MS );
      }
      while(wp_read > 1250){     //if wp shutdown
         Serial.print("wp_shutdown: ");
         Serial.println(wp_read);
         vTaskDelay( 200 / portTICK_PERIOD_MS );
      }
      if(shutdown_stats){    //if status ok
        Serial.println("wp_start");
        shutdown_stats = 0;
        vTaskResume(lpwm_output_handle);
        vTaskResume(rpwm_output_handle);
      }
      vTaskDelay( 200 / portTICK_PERIOD_MS );
    }
}


//pwm_output//==================================================================================================================================================

//lpwm//-----------------------------------------------------------------------
void Task_lpwm_output(void *pvParameters){
  (void) pvParameters;
  
  bool l_reverse = 0;    //0:positive 1:negative
  int lpwm_last = wheel_stop_pwm;
  int lpwm_fix = wheel_stop_pwm; 
  int d_speed = 100;
  
  while(1){
    lpwm_fix = l_pwm;
    /*
    if( abs(abs(lpwm_fix) - abs(lpwm_last)) > d_speed){
      if(lpwm_fix >= 0){
        if(lpwm_fix > lpwm_last){
          lpwm_fix = lpwm_last + d_speed;
        }else if(lpwm_fix < lpwm_last){
          lpwm_fix = lpwm_last - d_speed;
        }
      }else if(lpwm_fix <= 0){
        if(lpwm_fix < lpwm_last){
          lpwm_fix = lpwm_last - d_speed;
        }else if(lpwm_fix > lpwm_last){
          lpwm_fix = lpwm_last + d_speed;
        }
      }
    }

    if(lpwm_fix >= 0 && lpwm_fix < 1000){
      lpwm_fix = -1000;
      lpwm_last = -1000;
    }else if(lpwm_fix < 0 && lpwm_fix > -1000){
      lpwm_fix = 1000;
      lpwm_last = 1000;
    }
    
    if( abs(lpwm_fix) > l_pwm_max){
      if(lpwm_fix > 0){
        lpwm_fix = l_pwm_max;
      }else if(lpwm_fix < 0){
        lpwm_fix = -l_pwm_max;
      }
    }
    */
    if(lpwm_fix < 0 && !l_reverse){   //left positive to negative
      l_esc.writeMicroseconds(1000);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      digitalWrite(l_relay_pin,1);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      l_reverse = 1;
      Serial.println("left positive to negative");
    }else if(lpwm_fix > 0 && l_reverse){     //left negative to positive
      l_esc.writeMicroseconds(1000);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      digitalWrite(l_relay_pin,0);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      l_reverse = 0;
      Serial.println("left negative to positive");
    }
    
      //Serial.print("l_pwm_max: ");
      //Serial.println(l_pwm_max);
      //Serial.print("l_pwm: ");
      //Serial.println(l_pwm);
      Serial.print("l_pwm_fix: ");
      Serial.println(lpwm_fix);
      
      l_esc.writeMicroseconds(abs(lpwm_fix));
      lpwm_last = lpwm_fix;

    vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}

//rpwm//-----------------------------------------------------------------------
void Task_rpwm_output(void *pvParameters){
  (void) pvParameters;
  
  bool r_reverse = 0;    //0:positive 1:negative
  int rpwm_last = wheel_stop_pwm;
  int rpwm_fix = wheel_stop_pwm; 
  int d_speed = 100;
  
  while(1){
    rpwm_fix = r_pwm;
    /*
    if( abs(abs(rpwm_fix) - abs(rpwm_last)) > d_speed){
      if(rpwm_fix >= 0){
        if(rpwm_fix > rpwm_last){
          rpwm_fix = rpwm_last + d_speed;
        }else if(rpwm_fix < rpwm_last){
          rpwm_fix = rpwm_last - d_speed;
        }
      }else if(rpwm_fix <= 0){
        if(rpwm_fix < rpwm_last){
          rpwm_fix = rpwm_last - d_speed;
        }else if(rpwm_fix > rpwm_last){
          rpwm_fix = rpwm_last + d_speed;
        }
      }
    }
    
    if(rpwm_fix >= 0 && rpwm_fix < 1000){
      rpwm_fix = -1000;
      rpwm_last = -1000;
    }else if(rpwm_fix < 0 && rpwm_fix > -1000){
      rpwm_fix = 1000;
      rpwm_last = 1000;
    }
    
    
    if( abs(rpwm_fix) > r_pwm_max){
      if(rpwm_fix > 0){
        rpwm_fix = r_pwm_max;
      }else if(rpwm_fix < 0){
        rpwm_fix = -r_pwm_max;
      }
    }
*/
    if(rpwm_fix < 0 && !r_reverse){   //left positive to negative
      r_esc.writeMicroseconds(1000);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      digitalWrite(r_relay_pin,1);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      r_reverse = 1;
      Serial.println("right positive to negative");
    }else if(rpwm_fix > 0 && r_reverse){     //left negative to positive
      r_esc.writeMicroseconds(1000);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      digitalWrite(r_relay_pin,0);
      vTaskDelay( 100 / portTICK_PERIOD_MS );
      r_reverse = 0;
      Serial.println("right negative to positive");
    }
    
      //Serial.print("r_pwm_max: ");
      //Serial.println(r_pwm_max);
      //Serial.print("r_pwm: ");
      //Serial.println(r_pwm);
      Serial.print("r_pwm_fix: ");
      Serial.println(rpwm_fix);
      
      r_esc.writeMicroseconds(abs(rpwm_fix));
      rpwm_last = rpwm_fix;

    vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}



void loop() {}
