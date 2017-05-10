/* library imports *****************************************************************************************************/ //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>// //<>//

import processing.serial.*;
import com.dhchoi.CountdownTimer;
import com.dhchoi.CountdownTimerService;

/* Device block definitions ********************************************************************************************/
Device            haply_2DoF;
byte              deviceID                   = 5;
Board             haply_board;
DeviceType        degreesOfFreedom;
boolean           rendering_force                 = false;


/* Simulation Speed Parameters ****************************************************************************************/
final long        SIMULATION_PERIOD          = 1; //ms
final long        HOUR_IN_MILLIS             = 36000000;
CountdownTimer    haptic_timer;
float             dt                        = SIMULATION_PERIOD/1000.0; 


/* generic data for a 2DOF device */
/* joint space */
PVector           angles                    = new PVector(0, 0);
PVector           torques                   = new PVector(0, 0);

/* task space */
PVector           pos_ee                    = new PVector(0, 0);
PVector           pos_ee_last               = new PVector(0, 0); 
PVector           f_ee                      = new PVector(0, 0); 

/* Graphic objects */
float pixelsPerCentimeter= 40.0; //this is the resolution of my screen divided by the number of centimeters  i.e. a 1600px x 800px display with a 40 cm screen -> 40 pixels/cm
FWorld world; 
FBox b; 
FPoly p; 
FCircle g; 
FCircle e; 
HVirtualCoupling s; 
PImage haply_avatar;  

float worldWidth = 16.0;  
float worldHeight = 10.0; 


float edgeTopLeftX = 0.0; 
float edgeTopLeftY = 0.0; 
float edgeBottomRightX = worldWidth; 
float edgeBottomRightY = worldHeight; 

void setup() {
  
  size(640, 400); // (worldWidth*pixelsPerCentimeter, worldHeight*pixelsPerCentimeter) must input as number

  /* BOARD */
  haply_board = new Board(this, "COM5", 0);

  /* DEVICE */
  haply_2DoF = new Device(degreesOfFreedom.HaplyTwoDOF, deviceID, haply_board);

  hAPI_Fisica.init(this); 
  hAPI_Fisica.setScale(pixelsPerCentimeter); 
  world = new FWorld();


  //Insert Box object
  b = new FBox(3.0, 3.0);  //<>// //<>//
  b.setPosition(edgeTopLeftX+worldWidth/3.0, edgeTopLeftY+worldHeight/2.0); 
  b.setDensity(5); 
  b.setFill(random(255),random(255),random(255));
  world.add(b);

  // Insert Different sized circle objects
  FCircle e1 = new FCircle(1.0);
  e1.setPosition(7, 7);
  e1.setFill(random(255),random(255),random(255));
  world.add(e1); 
  
  FCircle e2 = new FCircle(2.5);
  e2.setPosition(3, 3); 
  e2.setFill(random(255),random(255),random(255));
  world.add(e2); 
  
  
  //Insert T-shaped Polygon
  p= new FPoly(); 
  p.vertex(-1.5, -1.0);
  p.vertex( 1.5, -1.0);
  p.vertex( 3.0/2.0, 0);
  p.vertex( 1.0/2.0, 0);
  p.vertex( 1.0/2.0, 4.0/2.0);
  p.vertex(-1.0/2.0, 4.0/2.0);
  p.vertex(-1.0/2.0, 0);
  p.vertex(-3.0/2.0, 0);
  p.setPosition(edgeTopLeftX+10, edgeTopLeftY+5); 
  p.setDensity(5); 
  p.setFill(random(255),random(255),random(255));
  world.add(p);

 //Insert Hard Blob object
  FBlob bl = new FBlob();
  float sca = random(4, 5);
  sca= sca/2.0f; 
  bl.setAsCircle(9, 3, sca, 30);
  bl.setStroke(0);
  bl.setStrokeWeight(2);
  bl.setFill(255);
  bl.setFriction(0);
  bl.setDensity(.25); 
  bl.setFill(random(255),random(255),random(255)); 
  world.add(bl);
  
  //Insert soft Blob object
  FBlob b2 = new FBlob();
  float sca1 = random(2, 4);
  sca= sca1/2.0f; 
  b2.setAsCircle(13, 4, sca1, 100);
  b2.setStroke(0);
  b2.setStrokeWeight(2);
  b2.setFill(255);
  b2.setFriction(0);
  b2.setDensity(1.5); 
  b2.setFill(random(255),random(255),random(255)); 
  world.add(b2);

// Setup the Virtual Coupling Contact Rendering Technique

  s= new HVirtualCoupling((1)); 
  s.h_avatar.setDensity(2); 
  s.h_avatar.setFill(255,0,0); 
  s.init(world, edgeTopLeftX+worldWidth/2, edgeTopLeftY+2); 
  haply_avatar = loadImage("../img/Haply_avatar.png"); 
  haply_avatar.resize((int)(hAPI_Fisica.worldToScreen(1)), (int)(hAPI_Fisica.worldToScreen(1)));
  s.h_avatar.attachImage(haply_avatar); 


  world.setGravity((0.0), (0.0)); //1000 cm/(s^2)
  world.setEdges((edgeTopLeftX), (edgeTopLeftY), (edgeBottomRightX), (edgeBottomRightY)); 
  world.setEdgesRestitution(.4);
  world.setEdgesFriction(0.5);
  
  world.draw();
  
  haptic_timer = CountdownTimerService.getNewCountdownTimer(this).configure(SIMULATION_PERIOD, HOUR_IN_MILLIS).start();
  
  frameRate(60); 
}

void draw() {
  background(255); 
   
  if(!rendering_force){
    
    //s.drawContactVectors(this); 
    
   }
    world.draw();
    //world.drawDebug();  
}

/**********************************************************************************************************************
 * Haptics simulation event, engages state of physical mechanism, calculates and updates physics simulation conditions
 **********************************************************************************************************************/ 

void onTickEvent(CountdownTimer t, long timeLeftUntilFinish){
  
  rendering_force = true;
   
  if (haply_board.data_available()) {
    /* GET END-EFFECTOR STATE (TASK SPACE) */
        
    angles.set(haply_2DoF.get_device_angles()); 
    pos_ee.set( haply_2DoF.get_device_position(angles.array()));
    pos_ee.set(pos_ee.copy().mult(100)); 
    
  }

  s.setToolPosition(edgeTopLeftX+worldWidth/2-(pos_ee).x+1.0, edgeTopLeftY+(pos_ee).y); 
  s.updateCouplingForce();
 
  f_ee.set(-s.getVCforceX(), s.getVCforceY());
 
  f_ee.div(10000); //
  haply_2DoF.set_device_torques(f_ee.array());
  torques.set(haply_2DoF.mechanisms.get_torque());
  haply_2DoF.device_write_torques();
  
  
  world.step(1.0f/1000.0f);
  
  rendering_force = false;
}


/* Timer control event functions **************************************************************************************/

/**
 * haptic timer reset
 */
void onFinishEvent(CountdownTimer t){
  println("Resetting timer...");
  haptic_timer.reset();
  haptic_timer = CountdownTimerService.getNewCountdownTimer(this).configure(SIMULATION_PERIOD, HOUR_IN_MILLIS).start();
}