#include <Dispatch.h>
#include <M5Station.h>
Dispatch jobs;
void setup() {
  //Enable and start up the serial port
  //Add two simple jobs to the dispatcher link list
  M5.begin(true,true,true);
  Serial.begin(115200);
  jobs.add(Counter,15000,20); //Run "Counter" function every 15 seconds. Expire in 20 milliseconds.
  jobs.add(DitchMe,30000,100);//Run "Ditchme" function after 30 seconds. Expire in 200 milliseconds.
  jobs.add(Overtime,5000,0);  //Run "Overtime" function every 5 seconds. No expire time.
}

void loop() {
  //Run the jobs in the dispatcher queue. 
  jobs.run();
}
//---------------------------------------------------------
// Function Counter
// Loop from 1 to 10000000, displaying the count.
// The loop is never allowed to complete because it runs out of time.
// Function will be re-launched every 15 seconds (or there abouts)
//----------------------------------------------------------
void Counter() {
  int k;
  Serial.println("Counter: Will I ever get to 100000000 ?");
  for(k=0;k<100000000;k++) {
    Serial.printf(" %d",k);
    if(jobs.expired()) {
      Serial.printf("\nCounter out of time after %lu milliseconds\n",jobs.runtime());
      return;
    }
  }
}
//---------------------------------------------------------
// Function DitchMe
// Runs after a 30 second delay.
// Shows how long the function was delayed by, writes a message
// and then gets removed from the dispatcher queue. Function
// will therefore only run once.
//-------------------------------------------------------
void DitchMe() {
  Serial.printf("Ditchme was delayed by %luMs\n",jobs.delaytime());
  Serial.println("Hello. After waiting 30 seconds, this is the last you'll see of me as I'll be deleted.");
  jobs.remove(DitchMe);
}
//-------------------------------------------------------
// Function Overtime
// Runs once every 5 seonds. Displays a message
//--------------------------------------------------------
void Overtime() {
    Serial.printf("I'll run once every 5 seconds or there abouts, (delayed by %luMs)\n", jobs.delaytime());
}
