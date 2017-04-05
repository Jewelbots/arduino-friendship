/*
 * Copyright (c) 2017 Jewelbots
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * Friend Notification Example
 *
 * In this example, the Jewelbot is programmed to respond when a friend in the green color group is recognized.
 * The Jewelbot will only respond the first time it sees a friend in the green color group using a boolean.
 * In this case, the Jewelbot will buzz, show the rainbow animation, and buzz again when it first sees another
 * Jewelbot in your green friend group.
 *
 * You can utilize a set of functions that return a boolean (true or false) that tell you if any friends from that
 * friend group are around.
 * see_red_friends()
 * see_green_friends()
 * see_blue_friends()
 * see_cyan_friends()
 *
 * Please note that this is an early version of this functionality.  Due to the nature of bluetooth, the Jewelbot might respond "as the first time"
 * to another Jewelbot that has been nearby for a while.  Similarly, the Jewelbot will respond "as the first time" when returning to showing friends
 * after sending or receiving a message.
 */
void setup() {
  // Optional -  tell Arduino if you are using the button
  // If using the button, uncomment the next line:
  //set_arduino_button();

  // put your setup code here, to run once:


} // setup

// Declare Jewelbots hardware components to use
Buzzer buzz;
Timer timer;
Animation animation;
// Declare booleans which control when you first see a friend of a particular color
bool first_time_red = true;
bool first_time_green = true;
bool first_time_blue = true;
bool first_time_cyan = true;

void loop() {
  // put your main code here, to run repeatedly:

  // Red friend group:
  if ((see_red_friends()) && (first_time_red)) {
    // Put commands here when a red friend is found
    first_time_red = false;
  } else if ((!see_red_friends()) && (!first_time_red)) {
    first_time_red = true;
  }

  // Green friend group:
  // Example code added here
  if ((see_green_friends()) && (first_time_green)) {
    buzz.short_buzz();
    animation.rainbows();
    buzz.short_buzz();
    timer.pause(500);
    first_time_green = false;
  } else if ((!see_green_friends()) && (!first_time_green)) {
    first_time_green = true;
  }

  // Blue friend group:
  if ((see_blue_friends()) && (first_time_blue)) {
    // Put commands here when a blue friend is found
    first_time_blue = false;
  } else if ((!see_blue_friends()) && (!first_time_blue)) {
    first_time_blue = true;
  }

  // Cyan friend group:
  if ((see_cyan_friends()) && (first_time_cyan)) {
    // Put commands here when a blue friend is found
    first_time_cyan = false;
  } else if ((!see_cyan_friends()) && (!first_time_cyan)) {
    first_time_cyan = true;
  }

} // loop
