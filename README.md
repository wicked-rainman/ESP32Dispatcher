# ESP32 Dispatcher
## A simple time slice scheduler for ESPs, Arduinos and the like.

I've had a stab at writing code using RTOS with some success, but every now and then bugs turn up in what I've written and I 
struggle to resolve where the issues are. I've read much of the RTOS documentation but there seems to be some magic involved in building
something that is robust. I guess for me, much of the time RTOS is overkill for what I'm trying to achieve. On the most part, my 
objectives are to read sensors, update some sort of display and maybe read serial data Etc. The problem is, sometimes these tasks block or 
take an unpredictable length of time to complete. This induces delays with every other action I want to perform and for me, that's often unacceptable.

This task dispatcher satisfies these objectives in a simplistic way. You define when you want a process to run and (optionally) end. If any one task is slow and non re-entrant, then nothing will really help. What this dispatcher will do is aid with any catching up process. Tardy tasks can be picked up and dealt with as soon as any blocking process relinquishes control. It effectively flattens the delays on calling other functions and helps provide a more smooth process flow. Taking this approach removes many of the reasons for calling delay() - which we all know, effectively blocks out the processor's ability to do anything else. 

There are pleanty of other "dispatcher" offerings around, but they all seem fairly complicated to use. Although this isn't as feature rich as some, it's simplistic approach may be more suitable in a domestic development scenario.

### Method:

+ The dispatcher creates a link list of function pointers. Each entry in the list indicates how long a function should be delayed before execution and how much time the function should be given to execute. These times are specified in milliseconds. Typically, the list creation takes place in startup code using the ___add___ funciton. For example:

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

+ The dispatcher recurses through this link list when you invoke its ___run___ method, evaluating how long each function has been waiting. Any function that has an expired wait time will be executed. If more than one function is in this state, the one thats incurred the longest delay will be executed first (This approach provides the delay leveling up). In an Arduino'esq environment, you would typically place this call inside the sketch main loop so it get's it's cpu cycles along with whatever other calls are being made.

```
void loop() {
    myjobs.run();                     //Get the dispatcher to run through the link list
        ....                              //Do whatever other tasks....
}

```

+ Within each dispatched function, calls can be made at suitable points to the dispatch ___expire___ method (If you don't do this, then the dispatcher will let the function run to completion). If the specified lapse time has been exceeded then this method returns a boolean which can be tested and actioned on. Using the main loop{} and setup{} as shown above, some simple examples using these methods are:

+ **Function1** - Runs every 2 seconds. The for{} loop will always execute for more than 5 milliseconds, so the call to ___expire()___ will return true after 5 milliseconds. In this instance, Function1 will return to the dispatcher. Two seconds later, Function1 will be executed again.

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

+ **Function2**  - Runs every 5 seconds. It never expires because it runs too quickly (On an ESP32). When the function's for{} loop completes, the print statement that follows includes an invocation of ___delaytime()___ and ___runtime()___. This shows how much time (in milliseconds) the function used and how long any execution delay was. Knowing this information can help focus where function code optimisation may be required.

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
+ **Funciton3** - Runs after 2 minutes. It includes a call to ___remove()___ which (obviously) removes itself from the dispatcher link list. In effect, this makes Function3 a "run once" process. 

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
### Notes

+ This library is intended to provide simplistic __run()__, __add()__, __remove()__, __runtime()__ and __delaytime()__ methods that help simplify complex sketch logic flows. It does not provide any inter-process communications, or support the use of multi-core processing. 

* Process timings are based on calls to millis(). A toggle to call micros() could be added, but as it stands the resolution seems good enough.  

+ All this code is written in *C* with a *C++* styled "wrapper" round it. This seems to be the only way to include a library within an Arduino IDE environment. There were indications that this could be achieved using the compiler extern directive, but I couldn't get past failures in the link stage. If anyone can show me how to do this, then please drop me a line!  
