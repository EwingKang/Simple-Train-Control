// These constants won't change. They're used to give names to the pins used:

// Input pins
const int PIN_throttle = A1;
const int PIN_key = 4;
const int PIN_dir_a = 5;
const int PIN_dir_b = 6;

// Output pins
const int PIN_motor1a = 10;
const int PIN_motor1b = 11;

// Timer
const float freq = 400;
const unsigned int perd_us = 1e6/freq;
const float cli_freq = 20;
const float spd_show_freq = 20;
const unsigned int cli_perd_us = 1e6/cli_freq;
const unsigned int spd_show_perd_us = 1e6/spd_show_freq;
unsigned long clk_us, last_us, last_cli_us, last_spd_show_us;

// LPF
const float lpf_freq = 0.25;

// Prototypes
void MotorControl(int pina, int pinb, float speed);

void setup() 
{
	Serial.begin(115200);
	pinMode(PIN_motor1a, OUTPUT);
	pinMode(PIN_motor1b, OUTPUT);
	digitalWrite(PIN_motor1b, LOW);
	digitalWrite(PIN_motor1a, LOW);
	
	pinMode(PIN_throttle, INPUT);
	pinMode(PIN_key, INPUT_PULLUP);
	pinMode(PIN_dir_a, INPUT_PULLUP);
	pinMode(PIN_dir_b, INPUT_PULLUP);
	Serial.println("Welcome to ArduModelTrain control!");
	Serial.println("Enter \"CMD\" for CLI control!");
	Serial.println("Under CLI, commands accept +-255 UART number input, use \"\\n as line break.");
	
	last_us = micros();
}

String cli_st = "";
String st_climd = "CMD\n";
String key_str[2] = {"On", "Off"};
String dir_str[3] = {"R", "N", "F"};
int throttle = 0;        // value read from the pot
float speed = 0;        // value output to the PWM (analog out)
float last_speed = 0;
float spd_cli = 0, spd_io = 0;
int key = HIGH;
int dir_a = false, dir_b = false;
int direction = 0, last_direction=0;		// -1,0,1
bool cli_mode = false;

void loop() 
{	
	unsigned long clk_us = micros();
	unsigned int dt_us = clk_us - last_us;
	if(dt_us >= perd_us)
	{
		//===== Uart CLI =====
		unsigned int dt_cli_us = clk_us -last_cli_us;
		if(dt_cli_us >= cli_perd_us)
		{
			char inChar=0;
			while (Serial.available() > 0) 
			{
				inChar = Serial.read();
				cli_st += (char)inChar;
				if(inChar == '\n') 
				{
					break;
				}
			}
			if(inChar == '\n') 
			{
				if(cli_mode)
				{
					spd_cli = constrain(cli_st.toFloat(), -255, 255);
				}
				if (cli_st == st_climd)
				{
					cli_mode = !cli_mode;
					if(cli_mode) {
						Serial.println("CLI mode!");
					}
					else {
						Serial.println("IO mode!");
					}
				}
				cli_st="";
			}
			last_cli_us +=dt_cli_us;
		}
		
		//===== Switches and Pins =====
		if(key != digitalRead(PIN_key))
		{
			key ^= HIGH;
			Serial.print("Key: ");
			Serial.println(key_str[key]);
		}
		if(key == LOW)
		{
			dir_a = digitalRead(PIN_dir_a);
			dir_b = digitalRead(PIN_dir_b);
			if(dir_a == dir_b)
			{
				direction = 0;	// both high or both low
			}
			else
			{
				direction = (dir_a>dir_b)? 1:-1;
				throttle = analogRead(PIN_throttle);
				spd_io = map(throttle, 0, 1023, 0, 255);
			}
			spd_io *= direction;
			if(direction != last_direction)
			{
				Serial.print("Dir: ");
				Serial.println(dir_str[direction+1]);
				last_direction = direction;
			}
		}
		else {
			spd_io = 0;
		}
		
		//===== Motor control =====
		float f = 2*M_PI*lpf_freq*dt_us*1e-6;
		float alpha = f/(f+1);
		if(cli_mode) {
			speed += (spd_cli - speed)*alpha;
		}
		else {
			speed += (spd_io - speed)*alpha;
		}
		
		// change the analog out value:
		MotorControl(PIN_motor1a, PIN_motor1b, speed);

		// print the results to the Serial Monitor:
		if( ((clk_us-last_spd_show_us) >= spd_show_perd_us) &&
			( abs(speed - last_speed) > 0.005 ) )
		{
			//Serial.print("sensor = ");
			//Serial.print(throttle);
			Serial.print("Spd = ");
			last_speed = (float)round(speed*100)/100.0;
			Serial.println(last_speed);
			last_spd_show_us = clk_us;
		}
		
		last_us += dt_us;
	}
	
}

// speed: +- 255
void MotorControl(int pina, int pinb, float speed)
{
	if(speed > 0)
	{
		analogWrite(pina, speed);
		analogWrite(pinb, 0);
	}
	else
	{
		analogWrite(pina, 0);
		analogWrite(pinb, abs(speed));
	}
}