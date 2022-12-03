# ESP32Dispatcher
## A simple cron-like "time slice" scheduler for ESPs and Arduinos.

I've had a stab at writing code using RTOS with some success, but every now and then bugs turn up in what I've written and I 
struggle to resolve where the issues are. I've read much of the RTOS documentation but there seems to be some magic involved in building
something that is reliable. I guess for me, much of the time RTOS is overkill for what I'm trying to achieve. On the most part, my 
objectives are to read sensors, update some sort of display and maybe read serial data Etc. The problem is, sometimes these tasks block or 
take an unpredictable length of time to complete. This induces delays with every other action I want to perform and for me, that's often unacceptable.

This simple task dispatcher (Yes, I know similar things have been written before) suits my requirements in a simplistic way. 
### Method:

+ Create a link list of function pointers. Each entry in the list indicates how long the function should be delayed for prior to execution and
how much time the function should be given to execute. These times are specified in milliseconds. Typically, this creation takes place in
startup code. For example:

```Dispatch myjobs;                       //Create a dispatch instance
    void setup() {
      Serial.begin(115200)                //Set up the serial port
      myjobs.add(Function1, 2000,100);    //Add Function1 to the list. Run every 2 seconds, expire after 100Ms
      myjobs.add(Function2, 5000,1000);   //Add Function2 to the list. Run every 5 seconds, expire after 1 second
      myjobs.add(Function3, 120000,0);    //Add Function3 to the list. Run after 2 minutes, no expiry time. 
    }

```

+ Recurse through this link list to evaluate how long each function has been waiting and invoke any function that has expired it's wait time.
If more than one function has expired, the function that has incurred the longest delay will be executed first. In an Arduino'esq environment, this takes place in the main loop funciton. 

```
    void loop() {
        myjobs.run();                     //Get the dispatcher to run through the link list
        ....                              //Do other tasks....
    }
```

+ Within each dispatched function, make calls at suitable points to test expiry time. If the time has been exceeded then return. In the main loop above, examples are:

**Function1** - Runs every 2 seconds. Always executes for too long, so returns after 100 milliseconds.

        ```
            void Function1() {
                for(int k=0;k<5000000;k++) {
                    Serial.printf("The value for K is %d\n",k);
                    if(myjobs.expire()) return;
                }
            }
        ```

**Function2**  - Runs every 5 seconds. Never expires because it runs too quickly. Displays how much time it's used and any execution delay

```
    void Function2() {
      for(int k=0;k<500;k++) {
        if(myjobs.expire()) return;
        int x=k;
      }
        Serial.printf("In function 2. Ran after a %lu delay. Took %lu to complete\n", 
                                                myjobs.delaytime(), myjobs.runtime());
    }
```
**Funciton3** - Runs after 2 minutes then removes itself from the task list

```
    void Function3() {
        for(int k=0;k<5;k++) {
            Serial.println("Hello");
        }
        if(k>2) myjobs.remove(Function3);
    }
```
