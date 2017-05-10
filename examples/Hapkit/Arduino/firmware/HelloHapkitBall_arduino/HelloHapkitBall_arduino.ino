/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This work was originally developed by Stanford Universty. The current version has been
 * modified by Steven Ding, Colin Gallacher, Oliver Schneider and Melisa Orta Martinez. 
 * The current work is intented to function with the hAPI developed for the World Haptics 
 * Conference Student Innovation Challenge.
 * 
 * The following code is subject to the 
 * 
 * GNU General Public License v3.0 
 * GNU GPLv3
 * Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.
 * 
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 * Change Log: 
 * 
 * 
 * 1) The file was partitioned for readability and usability
 * 2) A communication protocol was established to receive 10 bytes from the computer and transmit 9 bytes from the device while 
 * maintaining needed information throughput
 * 3) The Timer1 was repurposed to regularize the sampling time of the haptic simulation to 1kHz. 
 * 4) Added the sensor reading to the interrupt to make sure we never miss sensor counts.
 * 5) Changed default virtual environment to be No Force
 * 6) Added dynamic simulation
 * 
 *  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  
 * Instructions:
 * 
 * This code architecture consists of three files. The Hapkit_arduino.ino file handles the communication of data between an external 
 * computer handling the user interface. An API (hAPI) has been developed in Java to work in processing and handle communication between 
 * the Hapkit and the user interface. 
 * 
 * This example code is meant to show the user how to render a dynamic simulation using Hapkit. 
 * In this simulated environment there is a ball that collides with Hapkit. Upon collition the user will feel a force and the ball will
 * move in the other direction until gravity pulls it back towards Hapkit again. Please note that when using Hapkit the whole virtual
 * enviroment needs to be simulated in the Arduino code. 
 * 
 * 
 * The hAPI is the portal to the user interface on an external computer. It can handle the construction and respective communication   
 * of the data as well as setting up the respective paramters required to use a Haptic Paddle. 
 * 
 * A Processing example will be included so that users can see how the virtual environments can be programmed to interface with the Hapkit!
 * 
 * email crgallacher@gmail.com with any questions. 
 * 
 * Happy Coding :) 
 * 
 * 
 */

#include "Hapkit_arduino.h"
#include "Hapkit_Kinematics_Simulation.h"

byte      motors_active[4]; 
int       number_of_motors;  

actuator  Motor;


/* communication variables */
byte      cmd_code;
byte      device_function = 0;

float     outdata[2];
float     incoming_data[2];

bool      reply = false;

/* timing and debug variables */
int       timer1_flag = 0;
int       ledPin = 13;

/* Ball model */
typedef struct 
{
    float x;
    float mass;
    float velocity;
} Ball;
Ball ball;

/* constant velocity when the ball bounces so to not introduce too much energy into the system */
const float BALL_BOUNCE_VELOCITY = 0.02; //m/s
const float GRAVITY = 0.0098; //m/s^2

boolean startSim = false;
boolean applyForce = false;
int forceCounter = 0;


/*
 * Main setup function, defines parameters and hardware setup
 * 
 * You don't realy want to touch and of this code as it just handles the setup of the device.
 * The only  part users may want to explore is the passing of variables commented in the code below.
 */ 
void setup() {
  noInterrupts();

  ball.x = 0;
  ball.velocity = 0;
  ball.mass = 0.005; //kg

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  standard_hapkit_setup(&Motor);
  kinematics_simulation_setup();
  Serial.begin(57600);
  
  // don't run simulation unless processing is active. This is important for dynamic simulation
  // in which graphics guide action. Helps protect device. Can change for debugging.
  startSim = false;
  applyForce = false;
  forceCounter = 0;
  
  interrupts();
}

/*
 * Main loop function
 */
void loop() {
  
  if(timer1_flag){
    timer1_flag = 0;
    
    // even though for dynamic simulation we don't need to receive data from device, hAPI requires it 
    // in order to run and close the loop. So just receive. 
    if(Serial.available() > 0)
    {
      reply = true;
      startSim = true;
      
      cmd_code = command_instructions(Serial.read(), &number_of_motors, motors_active);

      device_function = receive_data(incoming_data);

      freq = incoming_data[0];
      amplitude = incoming_data[1];
    }
    else{

      // compute position
      computePosition();
      
      // Instead of the normal rendering algorithm, we will implement a dynamic 
      // simulation in the main loop. So comment out rendering Algorithm
      // renderingAlgorithm(device_function, &Motor);
      
      // We're using a custom rendering algorithm
      if (ball.x > xh) //the ball is in free space, will get pulled towards Hapkit with gravity
      {
        //update with gravity
        ball.velocity -= GRAVITY*0.001; 
        
        ball.x += ball.velocity*0.001;
         
      } else { // the ball has hit Hapkit. 
        applyForce = true;
        ball.velocity = BALL_BOUNCE_VELOCITY;
        ball.x = ball.x + ball.velocity*0.001;
        
      }
      if (applyForce && startSim) // if the ball has hit Hapkit and it is connected to processing, we will render a force
      {
        if (forceCounter < 3) // the force will be a step. 
        {
          force = -5;
          forceCounter = forceCounter + 1;
        }
        else
        {
          force = 0;
          applyForce = false;
          forceCounter = 0;
        }
      }
      else
      {
        force = 0; // there will be no force rendered when we are in free space
      }
      
      Tp = rp/rs * rh * force; // compute the required motor pulley torque (Tp) to generate that force
  
      forceOutput(&Motor, force, Tp);
      

      if(reply){
        reply = false;
        
        //Variables the adventurous user could play with. 
        outdata[0] = theta_s; //change to angle
        outdata[1] = ball.x;
        
        send_data(device_function, outdata);
      }
    }
  }
}

ISR(TIMER1_OVF_vect){
  TCNT1 = TIMER1_COUNTER;
  timer1_flag = 1;
  
  // update sensor position
  updateSensorPosition();
}
