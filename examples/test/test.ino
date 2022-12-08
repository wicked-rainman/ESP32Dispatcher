//----------------------------------------------------------------------------------
// Demo of Dispatch class
//
// Two functions (Function1 and Function2) are added to the Dispatch task queue.
// Both are run every two seconds. Function1 expires after 10Ms, and Function2 
// after 2000Ms (2 Seconds), 
// When Function1 has run 4 times, it deletes itself rom the task queue and
// adds Function3.
// When Function3 has run 3 times, it deletes itself, which just leaves
// Function2 being run by itself.
//----------------------------------------------------------------------------------

#include <M5Core2.h>
#include <Dispatch.h>
Dispatch myjobs;  //Create a dispatch instance

void setup() {
  Serial.begin(115200);               //Set up the serial port Etc
  myjobs.add(Function1, 2000, 10);    //Add Function1 to the list. Run every 5 seconds, expire after 500 milliseconds
  myjobs.add(Function2, 2000, 2000);  //Add Function2 to the list. Run every 5 seconds, expire after 500 milliseconds

  delay(5000);                        //First execution of functions will be late
  Serial.println("----Starting-----");
}
void loop() {
  myjobs.run();
}
void Function1() {  
  static int x = 0;
  while (1) {
    delay(10);
    if (myjobs.expired()) {
      Serial.printf("Function 1 expired, Delayed by %luMs, ran for %luMs, x=%d\n", myjobs.delaytime(), myjobs.runtime(), x);
      x++;
      if (x == 4) {
        Serial.print("Function 1 replaced by function3\n");
        myjobs.remove(Function1);
        myjobs.add(Function3, 2000, 10);
      }
      return;
    }
  }
}

void Function2() {
  while (1) {
    delay(10);
    if (myjobs.expired()) {
      Serial.printf("Function 2 expired, Delayed by %luMs, ran for %luMs\n", myjobs.delaytime(), myjobs.runtime());
      return;
    }
  }
}

void Function3() {
  static int y=0;
  while (1) {
    delay(10);
    if (myjobs.expired()) {
      y++;
      Serial.printf("Function 3 expired, Delayed by %luMs, ran for %luMs, x=%d\n", myjobs.delaytime(), myjobs.runtime(), y);
      if(y==3) {
        Serial.print("Function 3 removed\n");
        myjobs.remove(Function3);
      }
      return;
    }
  }
}
