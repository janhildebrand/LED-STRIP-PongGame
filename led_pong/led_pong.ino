#include "FastLED.h"

FASTLED_USING_NAMESPACE

//////////////////////////////////////
//
//
//      1D PONG - for LED Strips
//          created by
//       Jan P. Hildebrand
//
//
///////////////////////////////////////

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

//Pins for the two buttons
#define BTN_P1_PIN  3
#define BTN_P2_PIN  2

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    117     //Lodgia = 116 LEDS
CRGB leds[NUM_LEDS];

//////////////////////////////////////
//
//           Settings
//
////////////////////////////////////// 

  //GAME SETTINGS
#define LIFES 4
#define START_leds_per_player 5   // Number of the LEDs on that the player has to press his button
#define START_BALL_SPEED    10    
#define KILL_FLASHES       2

  //LED SETTINGS
#define FRAMES_PER_SECOND   100    // Framerate on most parts
#define START_SEQ_SPEED     30    // Start sequence speed in LEDs/second
#define BRIGHTNESS          96   // LED Brightness
#define FADE_SETTING        10



//////////////////////////////////////
//
// Initialization of game parameters
//
//////////////////////////////////////

int FADE_TIME = FADE_SETTING*5;
const int NUM_GAME_LEDS = NUM_LEDS-(10*LIFES);

bool gamemode_active = false;
bool game_started = false;
bool btnP1 = false;
bool btnP2 = false;
int entprellZeit=40;
volatile unsigned long btn1_alteZeit=0;
volatile unsigned long btn2_alteZeit=0;

int lifes_p1 = LIFES;
int lifes_p2 = LIFES;
const int ball_pos_center = NUM_GAME_LEDS/2;     // center ball position on the Game leds
int ball_pos = ball_pos_center;                  // current ball position
int ball_pos_rev = NUM_GAME_LEDS-ball_pos-1;
int ball_dir = 1;                   // Ball direction: -1=left  1=right
int ball_speed = START_BALL_SPEED;  // LEDs/second

int leds_per_player = START_leds_per_player;

//////////////////////////////////////
//
// Initial. of LED-lightshow parameters
//
//////////////////////////////////////
int gHue = 0;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti/*, sinelon*/, juggle/*, bpm*/ };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current


void setup() {
  delay(1000); // 1 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  //pinModes for Button pins
  pinMode(BTN_P1_PIN, INPUT);
  pinMode(BTN_P2_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(BTN_P1_PIN), ponepressed, RISING);
  delay(20);
  attachInterrupt(digitalPinToInterrupt(BTN_P2_PIN), ptwopressed, RISING);

  randomSeed(analogRead(0));

  Serial.begin(9600);
}


void loop()
{
  ball_pos_rev = NUM_GAME_LEDS-ball_pos-1;
  
  // if a button has been pressed switch to gaming mode
  if (btnP1 || btnP2)
  {
	btnP1 = false;
	btnP2 = false;
	gamemode_active = true;
  }
  
  if (gamemode_active)
  {
	  oneDPong();
  }
  else
  {
	  // play lightshow
  }
  
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

/*
  Serial.print("Button1: ");
  Serial.print(btnP1);
  Serial.print(" -- Button2: ");
  Serial.print(btnP2);
*/
/*
  Serial.print(" -- ball_pos: ");
  Serial.print(ball_pos);
  Serial.print(" -- ball_pos_rev: ");
  Serial.print(ball_pos_rev);
  Serial.print(" -- ball_dir: ");
  Serial.print(ball_dir);

  Serial.print(" -- lifes_p1: ");
  Serial.print(lifes_p1);
  Serial.print(" -- lifes_p2: ");
  Serial.print(lifes_p2);

  Serial.println();
  */
}


void oneDPong()
{

  // If the game hasn't been started yet
  if (!game_started)
  {
    lifes_p1 = LIFES;
    lifes_p2 = LIFES;
    ball_speed = START_BALL_SPEED;
    
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    for (int i=0; i<(NUM_LEDS/4); i++)
    {
      leds[i] = leds[NUM_LEDS-1-i] = CRGB::Orange;
    }
    //fill_rainbow( &( leds[leds_per_player+10] ), ( NUM_LEDS - 2*leds_per_player - 20 ), gHue, 7);


    if (btnP1 || btnP2)
    {
      btnP1 = false;
      btnP2 = false;
      game_started = true;
      startSequence();
      refresh_oneDPong();
    }
  }

  // If the game is in progress
  else
  {
    if (btnP1)
    {
      btnP1 = false;
      if (ball_pos <= leds_per_player)
      {
        ball_dir = ball_dir*(-1);
        ball_speed += 3;
      }
      else
      {
        playerOneKilled();
      }
    }
    if (btnP2)
    {
      btnP2 = false;
      if ((NUM_GAME_LEDS-ball_pos-1) <= leds_per_player)
      {
        ball_dir = ball_dir*(-1);
        ball_speed += 3;
      }
      else
      {
      playerTwoKilled();
      }
    }
   
    refresh_oneDPong();
    ball_onePong();
  }
  
}


void ball_onePong()
{
  ball_pos += ball_dir;
  
  
  if (ball_pos < -1)
  {
    playerOneKilled();
  }
  if (ball_pos > (NUM_GAME_LEDS))
  {
    playerTwoKilled();
  }

  if (ball_pos >= 0 && ball_pos <= NUM_GAME_LEDS-1)
  {
    leds[(LIFES*5)+ball_pos] = CRGB::Blue;
  }
  delay(1000/ball_speed);
}


void pOneLost()
{
  for(int i=0; i<10;i++)
  {
    fill_solid(leds,NUM_LEDS/2, CRGB::Red);
    fill_solid(&(leds[NUM_LEDS/2]), NUM_LEDS/2 , CRGB::Green);
    FastLED.show();
    FastLED.delay(200);
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(300);
  }
  ball_pos = ball_pos_center;
  game_started=false;
  gamemode_active = false;
}


void pTwoLost()
{
  for(int i=0; i<10;i++)
  {
    fill_solid(leds,NUM_LEDS/2, CRGB::Green);
    fill_solid(&(leds[NUM_LEDS/2]), NUM_LEDS/2 , CRGB::Red);
    FastLED.show();
    FastLED.delay(200);
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(300);
  }
  ball_pos = ball_pos_center;
  game_started=false;
  gamemode_active = false;
}


void playerOneKilled()
{
  lifes_p1--;
  ball_pos = ball_pos_center;
  
  if (lifes_p1 <= 0)
    {
      pOneLost();
    }
  else
  {
    for(int i=0; i<KILL_FLASHES;i++)
    {
      fill_solid(leds,NUM_LEDS/2, CRGB::Red);
      fill_solid(&(leds[NUM_LEDS/2]), NUM_LEDS/2 , CRGB::Green);
      FastLED.show();
      FastLED.delay(500);
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      FastLED.delay(500);
    }
    startSequence();
  }
}


void playerTwoKilled()
{
  lifes_p2--;
  ball_pos = ball_pos_center;
  
  if (lifes_p2 <= 0)
    {
      pTwoLost();
    }
  else
  {
    for(int i=0; i<KILL_FLASHES;i++)
    {
      fill_solid(leds,NUM_LEDS/2, CRGB::Green);
      fill_solid(&(leds[NUM_LEDS/2]), NUM_LEDS/2 , CRGB::Red);
      FastLED.show();
      FastLED.delay(500);
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      FastLED.delay(500);
    }
    startSequence();
  }
}


void startSequence()
{
  fill_solid( leds, NUM_LEDS, CRGB::Black);
  for (int i=0; i<=(NUM_LEDS/2); i++)
  {
    leds[(NUM_LEDS/2)+i] = leds[(NUM_LEDS/2)-i] = CHSV( gHue, 255, 192);;
    FastLED.show();
    FastLED.delay(1000/START_SEQ_SPEED);
  }
  fill_solid( leds, NUM_LEDS, CRGB::Black);

  ball_dir = random(0 ,2);
  if(ball_dir == 0)
  {
    ball_dir = -1;
  }
}


void refresh_oneDPong()
{
  for (int i=0; i<(lifes_p1*5); i++)
  {
    leds[i] = leds[i+1] = CRGB::Purple;
    leds[i+2] = leds[i+3] = leds[i+4] = CRGB::Black;
    i+=4;
  }
 
  for (int i=0; i<(lifes_p2*5); i++)
  {
    leds[NUM_LEDS-1-i] = leds[NUM_LEDS-1-(i+1)] = CRGB::Purple;
    leds[NUM_LEDS-1-(i+2)] = leds[NUM_LEDS-1-(i+3)] = leds[NUM_LEDS-1-(i+4)] = CRGB::Black;
    i+=4;
  }
  
  for (int i=LIFES*5; i< (LIFES*5 + leds_per_player); i++)
  {
      leds[i] = leds[NUM_LEDS-1-i] = CRGB::Green;
  }

  for (int i=(LIFES*5 + leds_per_player); i< (LIFES*5 + 2*leds_per_player); i++)
  {
      leds[i] = leds[NUM_LEDS-1-i] = CRGB::Orange;
  }

  for (int i=(LIFES*5 + leds_per_player); i< (LIFES + 2*leds_per_player); i++)
  {
      leds[i] = leds[NUM_LEDS-1-i] = CRGB::Red;
  }
  
  //fadeToBlackBy( &(leds[LIFES*5+2*leds_per_player]) , (NUM_LEDS - 2*(LIFES*5+2*leds_per_player)), 10);
  fadeToBlackBy( leds, NUM_LEDS, FADE_TIME);
}


void ponepressed()
{
  if((millis() - btn1_alteZeit) > entprellZeit)
  {
    btnP1 = true;
    btn1_alteZeit = millis();
  }    
}


void ptwopressed()
{
  if((millis() - btn1_alteZeit) > entprellZeit)
  {
    btnP2 = true;
    btn1_alteZeit = millis();
  }   
}




















