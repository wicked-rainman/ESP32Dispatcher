# ESP32Dispatcher
## A simple cron-like "time slice" scheduler for ESPs and Arduinos.

I've had a stab at writing code using RTOS with some success, but every now and then bugs turn up in what I've written and I 
struggle to resolve where the issues are. I've read much of the RTOS documentation but there seems to be some magic involved in building
something that is reliable. I guess for me, much of the time RTOS is overkill for what I'm trying to achieve. On the most part, my 
objectives are to read sensors, update some sort of display and maybe read serial data Etc. The problem is, sometimes these tasks block or 
take an unpredictable length of time to complete. This induces delays with every other action I want to perform and for me, that's often unacceptable.

This simple task dispatcher (Yes, I know similar things have been written before) suits my requirements in a simplistic way. If a sub-task is slow, then it's slow. This won't help with that issue, but it does aid the "catchup" processes in that tardy tasks can be picked up and dealt with as soon as any blocking process relinquishes control. I'm using this in anger and it does seem to level out all the delays in high level function calls and seems to remove any of the reasons for ever calling the delay() function - which we all know, effectively blocks the ability to do anything else. 

### Method:

+ The dispatcher creates a link list of function pointers. Each entry in the list indicates how long a function should be delayed for prior to execution and how much time the function should be given to execute. These times are specified in milliseconds. Typically, the list creation takes place in startup code. For example:

```
#include <Dispatch.h>
Dispatch myjobs;                       //Create a dispatch instance

void setup() {

  myjobs.add(Function1, 2000,5);      //Add Function1 to the list. 
                                      //Run every 2 seconds, expire after 5Ms
  myjobs.add(Function2, 5000,1000);   //Add Function2 to the list. 
                                      //Run every 5 seconds, expire after 1 second
  myjobs.add(Function3, 120000,0);    //Add Function3 to the list. 
                                      //Run after 2 minutes, no expiry time. 
 }

```

+ The dispatcher recurse through this link list during it's ___run___ method to evaluate how long each function has been waiting. Any function that has an expired wait time will be executed. If more than one function is in this expired state, the one that has incurred the longest delay will be executed first. In an Arduino'esq environment, you would typically place this call inside the sketch main loop funciton so it get's it's cpu cycles along with whatever other calls are being made.

```
void loop() {
    myjobs.run();                     //Get the dispatcher to run through the link list
        ....                              //Do whatever other tasks....
}

```

+ Within each dispatched function, make calls at suitable points to the dispatch ___expire___ method (If you don't do this, then the dispatcher will let the function run to completion). If the lapse time is exceeded then the method returns a boolean which can be actioned on. Using the main loop as shown above, some simple examples are:

+ **Function1** - Runs every 2 seconds. The for loop will always execute for more than 5 milliseconds, so the call to ___myjobs.expire()___ will return true after 5 milliseconds. In this instance, Function1 will return to the dispatcher. Two seconds later, Function1 will be executed again.

```
            void Function1() {
                for(int k=0;k<5000000;k++) {
                    Serial.printf("%d ",k);
                    if(myjobs.expired()) {
                        Serial.printf("\n Function1d expired\n");
                        return;
                    }
                }
            }
 ```

+ **Function2**  - Runs every 5 seconds. It never expires because it runs too quickly (On an ESP32). When the function completes, the print statement includes an invocation of ___myjobs.delaytime()___ and ___myjobs.runtime()___. This shows how much time (milliseconds) the function used and how long any execution delay was. Knowing this information can help focus where code function optimisation may be required.

```
    void Function2() {
      for(int k=0;k<5000;k++) {
        if(myjobs.expired()) return;
        int x=k;
      }
        Serial.printf("In function 2. Ran after a %luMs delay. Took %luMs to complete\n", 
                                                myjobs.delaytime(), myjobs.runtime());
    }
```
+ **Funciton3** - Runs after 2 minutes. It includes a call to __myjobs.remove()___ which (obviously) removes itself from the task list. In effect, this makes Function3 a "run once" process. 

```
    void Function3() {
        int k;
        for(k=0;k<5;k++) {
            Serial.println("Hello");
        }
        if(k>2) {
            myjobs.remove(Function3);
            Serial.println("Function3 has run and will never be seen again");
        }
    }
```
