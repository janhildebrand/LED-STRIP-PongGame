# 1D LED Pong
 This is a one-dimensional pong game for LED Strips.<br>
 If no game is played the LED Strip displays a lightshow
 - The game is designed for two Players.
 - The ball has to be hit by the player with a button press at the correct time (when the ball is in the green area of that player).
 - If a player presses the button too early or too late he loses a life.
 - The ball speed increases with every hit of the ball which makes the game more difficult over time.
 
# Required Hardware:
- Arduino Nano (or similar/better arduino (has to support two interrupt pins))
- LED Strip + Power supply (WS2812B or any other LED-Strip that has individual lightable LEDs and is supported by the FastLED library)
- two buttons with added pull-down resistors, connected to interrupt pins on the arduino

# Required Software:
- FastLED library downloadable directly in the Arduino IDE library manager or at https://github.com/FastLED/FastLED
