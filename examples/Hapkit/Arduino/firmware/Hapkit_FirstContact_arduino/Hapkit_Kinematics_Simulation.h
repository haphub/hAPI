/* Includes *****************************************************************************************************/
#include <math.h>

/* Threshold and offset Constants *******************************************************************************/
#define FLIPTHRESH      100
#define OFFSET          972-50
#define OFFSET_NEG      50

/* Position tracking variables **********************************************************************************/
int updatedPos = 0;     // keeps track of the latest updated value of the MR sensor reading
int rawPos = 0;         // current raw reading from MR sensor
int lastRawPos = 0;     // last raw reading from MR sensor
int lastLastRawPos = 0; // last last raw reading from MR sensor
int flipNumber = 0;     // keeps track of the number of flips over the 180deg mark
int tempOffset = 0;
int rawDiff = 0;
int lastRawDiff = 0;
int rawOffset = 0;
int lastRawOffset = 0;
boolean maybeFlipped = false;

/* Calibration parameters ***************************************************************************************/
float m = 0.0002292;
float b = 0;

/* Force output variables ***************************************************************************************/
float force = 0;  
float Tp = 0;

/* Kinematics variables *****************************************************************************************/
float xh = 0;           // Position of the handle [m]
float xh_prev;          // Distance of the handle at previous time step
float dxh;              // Velocity of the handle
float dxh_prev;
float dxh_filt;         // Filtered velocity of the handle

/* Kinematic parameters *****************************************************************************************/
float rs = 0.073152;   //[m] length of Sector in m
float rp = 0.004191;   //[m] radius of motor drive in m
float rh = 0.075;      //[m]  length of handle in m

/* Adjustable variables *****************************************************************************************/
float freq = 0.0;
float amplitude = 0.0;

/* Haptic rendering variables***********************************************************************************/
// Parameter for virtual wall
float x_wall = 0.005;                   // Position of the virtual wall
float k_wall = 700;                   // Maximum stiffness of the virtual wall

// Parameter for linear damping
float b_linear = 10;                   // Linear damping in N/m

// Parameter for spring:
float k_spring = 100;


/* Setup function declarations **********************************************************************************/
void kinematics_simulation_setup(void);

/* Kinematics and Simulation function declarations **************************************************************/
void updateSensorPosition(void);
void computePosition(void);
void renderingAlgorithm(int functionNumber, actuator *mtr);
void forceOutput(actuator *mtr, float force, float Tp);


/* Setup declarations *******************************************************************************************/
/**
 * kinematics_simulation_setup, initializes MR  sensor to a starting position
 * 
 * @return   none
 */
void kinematics_simulation_setup(void){
  lastLastRawPos = analogRead(SENSORPOSPIN);
  lastRawPos = analogRead(SENSORPOSPIN);
  flipNumber = 0;
  b = -(lastRawPos-OFFSET_NEG) * m;
}

/* Kinematics and Simulation function ***************************************************************************/
/*
 * updates MR sensor position
 * 
 * @return   none
 */
 
void updateSensorPosition(void){
  
  rawPos = analogRead(SENSORPOSPIN);  //current raw positoin from MR sensor 1
  
  // Calculate differences between subsequent MR sensor readings
  rawDiff = rawPos - lastRawPos;          //difference btwn current raw position and last raw position
  rawOffset = abs(rawDiff);
  
  // we don't update the position if we are in the middle of debouncing
  if (( rawOffset > FLIPTHRESH ) && (maybeFlipped == 0)){
    maybeFlipped = 1; 
  }
  else if (maybeFlipped == 1){
    maybeFlipped = 0;
    lastRawDiff = rawPos - lastLastRawPos; //difference btwn current raw position and last last raw position
    lastRawOffset = abs(lastRawDiff);
    
    // so here the debouncing is that if the difference is consistently above the threshold, we add "Offset"  
    if (lastRawOffset > FLIPTHRESH){
    
      // check to see which direction the drive wheel was turning
      if(lastRawDiff > 0){        
        flipNumber--;              // cw rotation 
      } 
      else {                     // if(rawDiff < 0)
        flipNumber++;              // ccw rotation
      }
    } 
    updatedPos = (rawPos-OFFSET_NEG) + flipNumber*OFFSET; // need to update pos based on what most recent offset is   
  }
  else {
    updatedPos = (rawPos-OFFSET_NEG) + flipNumber*OFFSET; // need to update pos based on what most recent offset is 
  }
  
  // Update position record-keeping vairables
  lastLastRawPos = lastRawPos;
  lastRawPos = rawPos;
}

/**
 * Compute position of system in meters
 * 
 * @note     Section 1
 * @return   none
 */
void computePosition(void){
  
  float theta_s; // Angle of the sector pulley in deg
  
  // Compute the angle of the sector pulley (ts) in degrees based on updatedPos
  theta_s = m * updatedPos + b; // m = 0.0131 [deg/pos], b = -8.504 [deg]  
  
  // Compute the position of the handle in m
  xh = rh*theta_s;
  
  // Calculate the velocity of the handle
  dxh = (xh - xh_prev) / 0.001;
  
  // Calculate the filtered velocity of the handle using an infinite impulse response filter
  dxh_filt = 0.9*dxh + 0.1*dxh_prev; 
    
  // Record the position and velocity
  xh_prev = xh;
  dxh_prev = dxh;
}


/**
 * Rendering algorithm, determine force values that are needed and generates said forces
 * 
 * @note     Section 2: include your own rendering algorithm in this section
 * @param    functionNumber: the type of function to be carried out
 * @param    mtr: the actuator to generate the force on
 * @return   none
 */
void renderingAlgorithm(int functionNumber, actuator *mtr){
force =0; 
  
  switch(functionNumber){
    // no force
    case 0:
      force = 0;
      break;
    
    // virtual wall
    case 1:
      if(xh > x_wall){
        force = -k_wall * (xh - x_wall);
      } else {
        force = 0;
      }
      break;
    
    // linear damping
    case 2:
      force = -b_linear * dxh_filt;
      break;
    
    // spring
    case 3:
      force = -k_spring*xh;
      break;
    
    default:
      force = 0;
      break;
  }

  // Section 3: transforms force to torque at motor
  Tp = rp/rs * rh * force; // compute the required motor pulley torque (Tp) to generate that force
  
  forceOutput(mtr, force, Tp);
}


/**
 * Force output to be generated
 * 
 * @note     Section 4
 * @param    mtr: actuator to generate the specified force
 * @param    force: force to be generated
 * @param    Tp: torque to be generated at the motor pulley
 * @return   none
 */
void forceOutput(actuator *mtr, float force, float Tp){
  
  float duty;
  
  // Determine correct direction for motor torque
  if(force > 0) { 
    digitalWrite(mtr->dirPin, HIGH);
  } else {
    digitalWrite(mtr->dirPin, LOW);
  }

  // Compute the duty cycle required to generate Tp (torque at the motor pulley)
  duty = sqrt(abs(Tp)/0.0183);

  // Make sure the duty cycle is between 0 and 100%
  if (duty > 1) {            
    duty = 1;
  } else if (duty < 0) { 
    duty = 0;
  }
  
  int output = (int)(duty * 255);   // convert duty cycle to output signal
  analogWrite(mtr->pwmPin, output);  // output the signal  
}







