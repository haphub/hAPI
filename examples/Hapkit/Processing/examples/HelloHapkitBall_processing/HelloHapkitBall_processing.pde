/*
code by Colin Gallacher, Steven Ding, Oliver Schneider and Melisa Orta Martinez
 The following code is subject to the 
 * 
 * GNU General Public License v3.0 
 * GNU GPLv3
 * Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.
 To come: 
 This code is used to generate a Dynamic Simulation using Hapkit. In order for it to be functional
 Hapkit needs to be running the code HellowHapkitBallArduino.ino
 This processing code will create a window in wich a virtual paddle and ball will be rendered. 
 The ball will fall towards the paddle and bounce off it when it hits. 
 The processing code is in charge of just rendering the graphics. The whole simulation happens in 
 Arduino. Arduino sends the position of the ball and paddle to the processing code which "draws" 
 it on the screen. 
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
byte            device_function     = 2;
float[]         in_data;

/* Graphics parameters ************************************************************************************************/
int             graphics_y          = 100;
int             paddle_width        = 20;
int             paddle_height       = 50;
int             ball_radius         = 20;
int             paddle_position     = 100;
int             ball_position       = 200;
int             WindowHeight        = 500;
int             WindowWidth         = 900;


/**********************************************************************************************************************
 * Main setup function, defines parameters for physics simulation and initialize hardware setup
 **********************************************************************************************************************/
void setup() {
  
  /* Setup for the graphic display window and drawing objects */
  size(900,500);
  noStroke();
  frameRate(baseFrameRate);
  
  /* Determining and printing the string name of the serial USB ports available */
  String serial_port = "";
  for (int i = 0; i < Serial.list().length; i++){
    print(Serial.list()[i]);
  }
  
  
  /* Initialization of the Board, Device, and Device Components */
  
  /* BOARD */
  paddle_link = new Board(this, Serial.list()[0], 57600);
  
  /* DEVICE */
  paddle = new Device(DeviceType.HapticPaddle, device_function, paddle_link);
}


/**********************************************************************************************************************
 * Main draw function, updates simulation animation at prescribed framerate 
 **********************************************************************************************************************/
void draw() {
  
  
  float temp_paddle_position;
  float temp_ball_position;
  
  /* check if new data is available from physical device */
  if (paddle_link.data_available()) {
    
    /* receive data from Arduino */
    paddle.receive_data();
    
    temp_paddle_position = paddle.mechanisms.get_coordinate()[0]; //get device position here forward kin
    
    /* scale position of paddle to window parameters */
    paddle_position = (int)(map(temp_paddle_position, -0.1,0.1,0,WindowWidth)); 
    temp_ball_position = paddle.mechanisms.get_torque()[0];
    
    /* scale position of ball to window parameters */
    ball_position = (int)(map(temp_ball_position, -0.1,0.1,0,WindowWidth));
  }
  else{
    /* send data request. For this The arduino does not need any parameters from processing. */ 
    /* but it does need to be sent something to run the simulation. We continue sending the */
    /* same parameters */
    paddle.send_data();
  }
  
  /* draw ball and paddle */
  background(255);
  fill(255,0,0);
  rect(paddle_position-paddle_width/2, graphics_y-paddle_height/2, paddle_width, paddle_height);
  fill(0,0,255);
  ellipse(ball_position, graphics_y, ball_radius*2, ball_radius*2);
}