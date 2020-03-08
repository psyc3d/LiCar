  /*Interrupts from timer 1 are used to schedule and deliver the sensor
trigger pulse. The same interrupt is used to control the flash rate
of the onboard LED indicating distance.*/

#include <TimerOne.h>                                 // Header file for TimerOne library

#define trigPin 12                                    // Pin 12 trigger output
#define echoPin 2                                     // Pin 2 Echo input
#define onBoardLED 13                                 // Pin 13 onboard LED
#define echo_int 0                                    // Interrupt id for echo pulse
#define mot_input1 7                                  //mot input 1 (for both the motors)
#define mot_input2 8                                  //mot input 2(for both the motors)
#define enable 9                                      //enable pwm for both the motors

#define TIMER_US 50                                   // 50 uS timer duration 
#define TICK_COUNTS 4000                              // 200 mS worth of timer ticks


float rel_vel[20];
float rel_dist[21];
int spd = 100;
int count;
int constant_one;
int speed_control; 
int mean;
volatile long echo_start = 0;                         // Records start of echo pulse 
volatile long echo_end = 0;                           // Records end of echo pulse
volatile long echo_duration = 0;                      // Duration - difference between end and start
volatile int trigger_time_count = 0;                  // Count down counter to trigger pulse time
volatile long range_flasher_counter = 0;              // Count down counter for flashing distance LED

void setup() 
{
  Serial.begin(9600); 
  pinMode(trigPin, OUTPUT);                           // Trigger pin set to output
  pinMode(echoPin, INPUT);                            // Echo pin set to input
  pinMode(onBoardLED, OUTPUT);                        // Onboard LED pin set to output
  pinMode(mot_input1,OUTPUT);
  pinMode(mot_input2,OUTPUT);
  pinMode(enable,OUTPUT);
  Timer1.initialize(TIMER_US);                        // Initialise timer 1
  Timer1.attachInterrupt( timerIsr );                 // Attach interrupt to the timer service routine 
  attachInterrupt(echo_int, echo_interrupt, CHANGE);  // Attach interrupt to the sensor echo input
}

void loop(){
  
  
int i;  //value for looping



float dis = echo_duration*0.033/2;      //dis is the relative distance by multiplying echo duration * 0.033/2 cm/us 
analogWrite(enable,dis*10);     //writing the pwm pin to the speed obtained from the slope
    Serial.println(dis*10);      
             
    if (count < 11 ){
      rel_dist[count] = dis;                //assigning valsuse to the relative distance arra
      if (dis<5){                           //caputure condition for sudden brakes
        immediate_brake();
        }
        else{
          start_motor();
          count++;
          }
    }                                                        
    else {
      count = 0;
      for(i = 0;i<10;i++){
        rel_vel[i] = rel_dist[i+1]-rel_dist[i];       //Converting relative distance into rel velocity
//        Serial.println(i);
//        Serial.println(rel_dist[i+1]);
//        Serial.println(rel_dist[i]);
//        Serial.println(rel_vel[i]);
        }
      //best fit line
      int x[10];
      int k = 0;
      int j = 0;
      for(i=0;i<10;i++){            //creating the x coordinate
        x[i] = i;
        k = k+i;
        }
      k = k/10;                     //k is the mean of the x coordinate 
      for(i=0;i<10;i++){
        j = j + rel_vel[i];
      }
      j = j/10;                     //j is the mean of the y coordinate
      
      for(i=0;i<10;i++){            //Looping to get the optimum slope 
        mean = (x[i]-k)*(rel_vel[i]-j);
//        Serial.println(mean);
        mean = mean/(pow(x[i]-k,2));
//        Serial.println(mean);
      }      
//      Serial.println("Mean is ");
//      Serial.println(mean);
      if (mean<2){                   //if mean is less than 0 we dont need to change the speed
        spd = spd;
      }
      else{ 
      //converting slope to pwm output
      constant_one = 20;                //assumed constant for speed output
      speed_control = mean*constant_one;
      spd = speed_control;
//      Serial.println("Speed is ");
//      Serial.println(spd);
      }  
    }                                                // calculates double distance 
    delay(10);                                       // every 10 mS
}

// timerIsr() 50uS second interrupt ISR()
// Called every time the hardware timer 1 times out.
void timerIsr()
{
    trigger_pulse();                                 // Schedule the trigger pulses
    distance_flasher();                              // Flash the onboard LED distance indicator
}

// trigger_pulse() called every 50 uS to schedule trigger pulses.
// Generates a pulse one timer tick long.
// Minimum trigger pulse width for the HC-SR04 is 10 us. This system delivers a 50 uS pulse.
void trigger_pulse()
{
      static volatile int state = 0;                 // State machine variable

      if (!(--trigger_time_count))                   // Count to 200mS
      {                                              // Time out - Initiate trigger pulse
         trigger_time_count = TICK_COUNTS;           // Reload
         state = 1;                                  // Changing to state 1 initiates a pulse
      }
      
    
      switch(state)                                  // State machine handles delivery of trigger pulse
      {
        case 0:                                      // Normal state does nothing
            break;
        
        case 1:                                      // Initiate pulse
           digitalWrite(trigPin, HIGH);              // Set the trigger output high
           state = 2;                                // and set state to 2
           break;
        
        case 2:                                      // Complete the pulse
        default:      
           digitalWrite(trigPin, LOW);               // Set the trigger output low
           state = 0;                                // and return state to normal 0
           break;
     }
}

// echo_interrupt() External interrupt from HC-SR04 echo signal. 
// Called every time the echo signal changes state.
void echo_interrupt()
{
  switch (digitalRead(echoPin))                     // Test to see if the signal is high or low
  {
    case HIGH:                                      // High so must be the start of the echo pulse
      echo_end = 0;                                 // Clear the end time
      echo_start = micros();                        // Save the start time
      break;
      
    case LOW:                                       // Low so must be the end of the echo pulse
      echo_end = micros();                          // Save the end time
      echo_duration = echo_end - echo_start;        // Calculate the pulse duration
      break;
  }
}

// distance_flasher() Called from the timer 1 timerIsr service routine.
// Flashes the onboard LED at a rate inversely proportional to distance. The closer it gets the higher the frequency.
void distance_flasher()
{
      if (--range_flasher_counter <= 0)                // Decrement and test the flash timer
      {                                                // Flash timer time out
         if (echo_duration < 25000)                    // If the echo duration is within limits
         {
           range_flasher_counter = echo_duration * 2;  // Reload the timer with the current echo duration
         }
         else
         {
           range_flasher_counter = 25000;              // If out of range use a default
         }
         
         digitalWrite( onBoardLED, digitalRead( onBoardLED ) ^ 1 );   // Toggle the onboard LED
      }
}

void immediate_brake()
{
  digitalWrite(mot_input1,HIGH);
  digitalWrite(mot_input2,HIGH);
  Serial.println("The car has stopped");
  }
void start_motor()
{
  digitalWrite(mot_input2,HIGH);
  digitalWrite(mot_input1,LOW);
  }
