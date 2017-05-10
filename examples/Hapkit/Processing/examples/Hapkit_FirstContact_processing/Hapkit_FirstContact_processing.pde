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
Board         paddle_link;
Device        paddle;
DeviceType    degreesOfFreedom;

/* Animation Speed Parameters *****************************************************************************************/
int           baseFrameRate       = 50;
long          currentTime; 
long          oldTime; 

/* Communications parameters ******************************************************************************************/
byte          commType            = 0;
byte          device_function     = 2;
float[]       in_data;

float         freq                = 0;
float         amplitude           = 0;



/**********************************************************************************************************************
 * Main setup function, defines parameters for physics simulation and initialize hardware setup
 **********************************************************************************************************************/
void setup(){
  
  /* Setup for the graphic display window and drawing objects */
  size(200, 200);
  noStroke();
  frameRate(baseFrameRate);
  
  
  /* Initialization of the Board, Device, and Device Components */
  
  /* BOARD */
  paddle_link = new Board(this, Serial.list()[0], 57600);
  
  /* DEVICE */
  paddle = new Device(degreesOfFreedom.HapticPaddle, device_function, paddle_link);
}


/**********************************************************************************************************************
 * Main draw function, updates simulation animation at prescribed framerate 
 **********************************************************************************************************************/
void draw(){
  currentTime = millis(); 
  
  /* set occurence at 5 second intervals */
  if((currentTime-oldTime) > 5000){
    
    /* cycle through difference device functions */
    device_function = (byte)(device_function+1);  
    oldTime = currentTime; 
   
    if(device_function > 3) device_function = (byte) 0; 
   
  }
  
  
  /* check if new data is available from physical device */
  if(paddle_link.data_available()){
    
    /* update simulation states */
    paddle.receive_data();
    in_data = paddle.mechanisms.get_angle();
  }
  else{
    
    /* send command parameters to device */
    paddle.set_parameters(device_function, freq, amplitude);
    paddle.send_data();
  }
}