/* Timer 1 Counter constant *************************************************************************************/
#define TIMER1_COUNTER    65474

/* Motor and sensor pin Constants *******************************************************************************/
#define PWMPINA           5
#define DIRPINA           8

#define PWMPINB           6 
#define DIRPINB           7

#define SENSORPOSPIN      A2 

/* Communication size constants *********************************************************************************/
#define IN_PARAMETERS     2
#define OUT_DATA          2

/* actuator struct **********************************************************************************************/
typedef struct motor{
  int pwmPin;
  int dirPin;
}actuator;


/* Setup function declarations **********************************************************************************/
void standard_hapkit_setup(actuator *Motor);
void setTimer1Interrupt(void);
void setPwmFrequency(int pin, int divisor);
void initialize_actuator(actuator *m, int pwm, int dir);

/* Communication function declarations **************************************************************************/
byte command_instructions(byte control, int *number, byte active[]);
void send_data(byte device, float data[]);
byte receive_data(float parameters[]);

/* Helper function declarations *********************************************************************************/
void FloatToBytes(float val, byte segments[]);
float BytesToFloat(byte segments[]);
void ArrayCopy(byte src[], int src_index, byte dest[], int dest_index, int len );


/* Setup function ***********************************************************************************************/
/**
 * standard hapkit hardware setup parameters
 * 
 * @param    Motor: actuator parameter struct for actuator that is to be setup
 * @return   none
 */
void standard_hapkit_setup(actuator *Motor){
  setTimer1Interrupt();
  initialize_actuator(Motor, PWMPINA, DIRPINA);
  pinMode(SENSORPOSPIN, INPUT);
}


/**
 * setups Timer1 to run as an overflow interrupt at a rate of 1.008kHz 
 * 
 * @note     - timer1 is a 16 bit timer giving it 65536 total counts before overflow
 *           - at a prescaler value of 256, timer1 has a base frequency of 62.5kHz (16MHz/256) 
 *           - to count 1 second, count needs to be initialized at 3036 (to count 62500 times before overflow)
 *           - count initialized at 65474 will result in: (65536-65474)/62500 = 0.0992ms between interrupt
 *           - 0.992ms period equals 1.008kHz
 */
void setTimer1Interrupt(void){
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = TIMER1_COUNTER;
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << TOIE1);
}

/**
 * sets up pwm frequency for given pin
 * 
 * @param    pin: pin to be setup
 * @param    divisor: timer clock divisor
 * @return   none
 */
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6) {
    
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }

    TCCR0B = TCCR0B & 0b11111000 | mode;
    
  } 
  else if(pin == 3 || pin == 11) {
    
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

/**
 * initialize actuator for use with given pins
 * 
 * @param    m: actuator
 * @param    pwm: pwm pin to be used
 * @param    dir: direction pin to be used
 * @return   none
 */
void initialize_actuator(actuator *m, int pwm, int dir){
  m->pwmPin = pwm;
  m->dirPin = dir;
  
  pinMode(pwm, OUTPUT);
  pinMode(dir, OUTPUT);
  
  setPwmFrequency(pwm, 1);
}


/* Communication function ***************************************************************************************/
/**
 * Decypher command instructions
 * 
 * @note     updates, motors_active, number_of_motors, and cmd_code
 * @param    control: input header byte to be parsed
 * @param    number: number of motors value to be manipulated
 * @param    motors_active: active motors array indicator
 * @return   command code indicating communication type 
 */
byte command_instructions(byte control, int *number, byte active[]){
  
  int j = 0;
  for(int i = 0; i < 4; i++){
    
    active[i] = control &0x01;
    control = control >> 1;

    if(active[i] > 0){
      j++;
    }
  }

  *number = j;
  return control;
}


/**
 * Send data to computer
 * 
 * @param    function: the current function mode the device is in 
 * @param    data: data to be sent
 * @return   none
 */
void send_data(byte function, float data[]){

  int data_length = 4 * OUT_DATA + 1;

  byte outgoing_data[data_length];
  byte segments[4];

  outgoing_data[0] = function;

  int j = 1;
  for(int i = 0; i < OUT_DATA; i++){
    FloatToBytes(data[i], segments);
    ArrayCopy(segments, 0, outgoing_data, j, 4);
    j = j + 4;
  }

  Serial.write(outgoing_data, data_length);
}

/**
 * receive data from computer
 * 
 * @param    parameters: array of floating point parameter value to be changed
 * @return   function, the function mode the device needs to be in 
 */
byte receive_data(float parameters[]){
  
  int data_length = 4 * IN_PARAMETERS + 1;

  byte incoming_parameters[data_length];
  byte segments[4];
  
  Serial.readBytes(incoming_parameters, data_length);

  int j = 1;
  for(int i = 0; i < IN_PARAMETERS; i++){
    ArrayCopy(incoming_parameters, j, segments, 0, 4);
    parameters[i] = BytesToFloat(segments);
    j = j + 4;
  }
  
  return incoming_parameters[0];

}



/* Helper functions *********************************************************************************************/

/**
 * Union definition for floating point and integer representation conversion
 */
typedef union{
  long val_l;
  float val_f;
} ufloat;

/**
 * Translates a 32-bit floating point into an array of four bytes
 * 
 * @note     None
 * @param    val: 32-bit floating point
 * @param    segments: array of four bytes
 * @return   None 
 */
void FloatToBytes(float val, byte segments[]){
  ufloat temp;

  temp.val_f = val;

  segments[3] = (byte)((temp.val_l >> 24) & 0xff);
  segments[2] = (byte)((temp.val_l >> 16) & 0xff);
  segments[1] = (byte)((temp.val_l >> 8) & 0xff);
  segments[0] = (byte)((temp.val_l) & 0xff);
}


/**
 * Translates an array of four bytes into a floating point
 * 
 * @note     None
 * @param    segment: the input array of four bytes
 * @return   Translated 32-bit floating point 
 */
float BytesToFloat(byte segments[]){
  ufloat temp;

  temp.val_l = (temp.val_l | (segments[3] & 0xff)) << 8;
  temp.val_l = (temp.val_l | (segments[2] & 0xff)) << 8;
  temp.val_l = (temp.val_l | (segments[1] & 0xff)) << 8;
  temp.val_l = (temp.val_l | (segments[0] & 0xff)); 

  return temp.val_f;
}


/**
 * Copies elements from one array to another
 * 
 * @note     None
 * @param    src: The source array to be copied from
 * @param    src_index: The starting index of the source array
 * @param    dest: The destination array to be copied to
 * @param    dest_index: The starting index of the destination array
 * @param    len: Number of elements to be copied
 * @return   None 
 */
void ArrayCopy(byte src[], int src_index, byte dest[], int dest_index, int len ){
  for(int i = 0; i < len; i++){
    dest[dest_index + i] = src[src_index + i];
  }
}

