//-----------------------------------------------------------
// Simple Arduino sketch to show how the dispatcher library
// can be used.
// Author: WR
//==========================================================

#include <Dispatch.h>
Dispatch myjobs;  //Create a dispatch instance

void setup() {
  Serial.begin(115200);               //Set up the serial port Etc
  myjobs.add(Function1, 2000, 5);   //Add Function1 to the list. Run every 2 seconds, expire after 5Ms
  myjobs.add(Function2, 5000, 1000);  //Add Function2 to the list. Run every 5 seconds, expire after 1 second
  myjobs.add(Function3, 120000, 0);   //Add Function3 to the list. Run after 2 minutes, no expiry time.
}
void loop() {
  myjobs.run();
}
void Function1() {
  for (int k = 0; k < 5000000; k++) {
    Serial.printf("%d ", k);
    if (myjobs.expired()) {
      Serial.printf("\nFunction1 expired\n");
      return;
      }
  }
}
void Function2() {
  for (int k = 0; k < 5000; k++) {
    if (myjobs.expired()) return;
    int x = k;
  }
  Serial.printf("In function 2. Ran after a %luMs delay. Took %luMs to complete\n",
                myjobs.delaytime(), myjobs.runtime());
}

void Function3() {
  int k;
  for (k = 0; k < 5; k++) {
    Serial.println("Hello");
  }
  if (k > 2) {
    myjobs.remove(Function3);
    Serial.println("Function3 had run and will never be seen again");
  }
}
