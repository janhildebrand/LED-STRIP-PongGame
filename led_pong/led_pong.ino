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


//////////////////////////////////////
//
// Adjust according to your hardware:
//
//////////////////////////////////////

//Pins of the two buttons (add Pull-down resistor to ground)
#define BTN_P1_PIN  3         // Button Player 1 + Start game
#define BTN_P2_PIN  2         // Button Player 2 + Scene switch

#define NUM_LEDS    117     	// Number of LEDs your strip contains has to be unequal 				// Lodgia = 116 LEDS
#define DATA_PIN    5			    // DATA_PIN of the LED Strip
#define LED_TYPE    WS2812B		// LED Strip type check FastLED library for support
#define COLOR_ORDER GRB			  // color order of the LED Strip




//////////////////////////////////////
//
//           Settings
//
////////////////////////////////////// 

  //GAME SETTINGS
#define LIFES 4					  // Number of Lifes a player has
#define START_leds_per_player 5   // Number of the LEDs on that the player has to press his button
#define START_BALL_SPEED    30    // initial ball speed (speed increases during gameplay)
#define SPEED_INCREASE		2	  // increase of speed at every paddle hit
#define KILL_FLASHES 		1


  //LED SETTINGS
#define FRAMES_PER_SECOND  100   // Framerate on most parts
#define START_SEQ_SPEED     30    // Start sequence speed in LEDs/second
#define BRIGHTNESS          70    // LED Brightness  //96
#define FADE_SETTING        10



///////////////////////////////////////////
//
// Initialization of game + LED parameters
//
///////////////////////////////////////////

int FADE_TIME = FADE_SETTING*5;
const int NUM_GAME_LEDS = NUM_LEDS-(10*LIFES);

int lightScene = 0;               // variable to switch between the lighting scenes
bool game_active = false;         // variable to switch between lighting and the game
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

int frames_per_second=50;
CRGB leds[NUM_LEDS];


void setup() {
  delay(300); // 300 millisecond delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  //pinModes for Button pins
  pinMode(BTN_P1_PIN, INPUT);
  pinMode(BTN_P2_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(BTN_P1_PIN), gameStartPressed, RISING);
  delay(20);
  attachInterrupt(digitalPinToInterrupt(BTN_P2_PIN), sceneSwitchPressed, RISING);
  //detachInterrupt(digitalPinToInterrupt(pin))

  randomSeed(analogRead(0));

  Serial.begin(9600);
  frames_per_second = FRAMES_PER_SECOND;
}


// List of patterns to cycle through during lightshow each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { confetti, juggle_blue, /*sinelon,*/ confetti, juggle_red/*, bpm*/ };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is currently active
int gHue = 0;


void loop()
{
  ball_pos_rev = NUM_GAME_LEDS-ball_pos-1;

  if (!game_active) {
    switch (lightScene) {
      case 0:
        fill_solid( leds, NUM_LEDS, CRGB::Black);
        for (int i=0; i<(NUM_LEDS/4); i++)
        {
          leds[i] = leds[NUM_LEDS-1-i] = CRGB::Orange;
        }
        break;
      
      case 1:
        fill_solid( leds, NUM_LEDS, CRGB::Black);
        for (int i=0; i<(NUM_LEDS/8); i++)
        {
          leds[i] = leds[NUM_LEDS-1-i] = CRGB::Orange;
        }
        break;
        
      case 2:
        fill_solid( leds, NUM_LEDS, CRGB::Black);
        for (int i=0; i<(NUM_LEDS-2); i=i+3)
        {
          leds[i] = CRGB::Orange;
        }
        break;

      case 3:
        fill_solid( leds, NUM_LEDS, CRGB::Black);
        for (int i=0; i<(NUM_LEDS/8); i++)
        {
          leds[i] = leds[NUM_LEDS-1-i] = CRGB::Red;
        }
        break;
        
      case 4:
        fill_solid( leds, NUM_LEDS, CRGB::Black);
        for (int i=0; i<(NUM_LEDS-2); i=i+3)
        {
          leds[i] = CRGB::Red;
        }
        break;
        
      case 5:
        // execute lightshow
        // Call the current pattern function once, updating the 'leds' array
        gPatterns[gCurrentPatternNumber]();
        break;
          
      case 6:
        juggle_red_slow();
        break;

      case 7:
        confetti();
        break;

      case 8:
        rainbow();
        break;
  
      case 9:
        rainbowWithGlitter();
        break;

      case 10:
        sinelon();
        break;
  
      case 11:
        bpm();
        break;

      default:
        lightScene = 1;
        break;
    }
  }
  else {
    // Display Game
    oneDPong();
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/frames_per_second);

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  EVERY_N_SECONDS( 15 ) { nextPattern(); } // change patterns periodically

}

///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//
//
//        Lightshow functions
//
//
///////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////// 

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 1);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(32), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 15);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue/2, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle_blue() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 18);
  byte dothue = 160;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 6;
  }
}

void juggle_red() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 18);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 4;
  }
}

void juggle_red_slow() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 2);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+2, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 2;
  }
}


void sceneSwitchPressed()
{
  if((millis() - btn1_alteZeit) > entprellZeit)
  {
    lightScene++;
    btn1_alteZeit = millis();
  }    
}

void gameStartPressed()
{
  game_active = true;

  // Detach scene switch/game start button interrupts
  detachInterrupt(digitalPinToInterrupt(BTN_P1_PIN));
  delay(20);
  detachInterrupt(digitalPinToInterrupt(BTN_P2_PIN));
  delay(50);

  // Attach Game Button interrupts
  attachInterrupt(digitalPinToInterrupt(BTN_P1_PIN), ponepressed, RISING);
  delay(20);
  attachInterrupt(digitalPinToInterrupt(BTN_P2_PIN), ptwopressed, RISING);
}


//////////////////////////////////////////////////////////////////////////////////

//    Game functions

//////////////////////////////////////////////////////////////////////////////////

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
      leds[i] = leds[NUM_LEDS-1-i] = CRGB::Purple;
    }
    //fill_rainbow( &( leds[leds_per_player+10] ), ( NUM_LEDS - 2*leds_per_player - 20 ), gHue, 7);


    delay(200);

    // XOR check of the buttons so game only gets started when only one button is pressed
    if (!btnP1 != !btnP2)
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
	  // if player 1 pressed in the correct time
      if (ball_pos <= leds_per_player)
      {
        ball_dir = ball_dir*(-1);
        ball_speed += SPEED_INCREASE;
      }
	  // if player 1 pressed in the wrong time
      else
      {
        playerOneKilled();
      }
    }
    if (btnP2)
    {
      btnP2 = false;
	  // if player 2 pressed in the correct time
      if ((NUM_GAME_LEDS-ball_pos-1) <= leds_per_player)
      {
        ball_dir = ball_dir*(-1);
        ball_speed += SPEED_INCREASE;
      }
	  // if player 2 pressed in the wrong time
      else
      {
      playerTwoKilled();
      }
    }
   
    refresh_oneDPong();
    update_ball();
  }
  
}


void update_ball()
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
  game_active=false;
  enableSceneSwitchInterrupts();
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
  game_active=false;
  enableSceneSwitchInterrupts();
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


void enableSceneSwitchInterrupts()
{
  // Detach game button interrupts
  detachInterrupt(digitalPinToInterrupt(BTN_P1_PIN));
  delay(20);
  detachInterrupt(digitalPinToInterrupt(BTN_P2_PIN));
  delay(50);

  // Attach scene switch/game start interrupts
  attachInterrupt(digitalPinToInterrupt(BTN_P1_PIN), gameStartPressed, RISING);
  delay(20);
  attachInterrupt(digitalPinToInterrupt(BTN_P2_PIN), sceneSwitchPressed, RISING);
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
