/*
code by Colin Gallacher and Steven Ding

The following code is subject to the 
 * 
 * GNU General Public License v3.0 
 * GNU GPLv3
 * Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.


To come: 
Instructions

L
*/


/* library imports *****************************************************************************************************/
import processing.serial.*;

/* Device block definitions ********************************************************************************************/
Board           paddle_link;
Device          paddle;
DeviceType      degreesOfFreedom;

/* Animation Speed Parameters *****************************************************************************************/
int             baseFrameRate       = 40;

/* Communications parameters ******************************************************************************************/
byte            commType            = 0;
byte            device_function     = 1;

float           freq                = 0;
float           amplitude           = 0;

int             counter             = 0; 

float[]         angle ;
float[]         torque;

/* Graphics parameters ************************************************************************************************/
PShape          HapticPaddle, Base, Force, Logo; 



/**********************************************************************************************************************
 * Main setup function, defines parameters for physics simulation and initialize hardware setup
 **********************************************************************************************************************/
void setup(){
  
  /* Setup for the graphic display window and drawing objects */
  size(1000 , 1000, P2D);
  noStroke();
  frameRate(baseFrameRate);
  
  
  /* Initialization of the Board, Device, and Device Components */
  
  /* BOARD */
  paddle_link = new Board(this, Serial.list()[0], 57600);
  
  /* DEVICE */
  paddle = new Device(degreesOfFreedom.HapticPaddle, device_function, paddle_link);
  
  HapticPaddle = loadShape("hapticPaddle.svg");
  Base = loadShape("hapticPaddleBase.svg"); 
  Force = loadShape("Force.svg");
  Logo = loadShape("StanfordHapkit.svg"); 
}


/**********************************************************************************************************************
 * Main draw function, updates simulation animation at prescribed framerate 
 **********************************************************************************************************************/
void draw(){

  /* check if new data is available from physical device */
  if(paddle_link.data_available()){
    
    /* update simulation states */
    paddle.receive_data();
    angle = paddle.mechanisms.get_angle();
    torque = paddle.mechanisms.get_torque(); 

    /* update animation */
    draw_static(); 
    update_handle(angle[0]*10); 
    update_force(torque[0], angle[0]*10); 
  
  }
  else{
    
    /* send command parameters to device */
    paddle.set_parameters(device_function, freq, amplitude);
    paddle.send_data();
  }
}

/* Graphical and physics functions ************************************************************************************/

/**
 * Updates static elements in the animation
 */
void draw_static(){
  
  background(255); 
  shape(Logo,0,0);
  pushMatrix(); 
  translate(width/2, height/2); 
  shape(Base, -Base.width/2, 0); 
  popMatrix();  
}


/**
 * Updates dynamic elements in the animation
 */
void update_handle(float angle) {
  
  pushMatrix(); 
  translate(width/2, height/2); 
  rotate(angle); 
  translate(-HapticPaddle.width/2, -HapticPaddle.height/2);
  shape(HapticPaddle,0,0);
  popMatrix(); 
}


/**
 * update kinematic forces needed for animation
 */
void update_force(float force, float angle) {
  
  pushMatrix(); 
  translate(width/2, height/2); 
  rotate(angle);    
  translate(0, -Force.height/2-HapticPaddle.height/4);
  force = -force; 
  float scaleFactor = 3.0f; 
  scale(force/scaleFactor);
  translate(0, -Force.height/2*force/(2*scaleFactor)); 
  shape(Force,0,0); 
  popMatrix(); 
}