# ESP32Dispatcher
I've had a stab at writing code using RTOS with some success, but every now and then bugs turn up in what I've written and I often
struggle to resolve where the issues are. I've read much of the documentation but there still seems to be some black art involved in building
something reliable. I guesa much of the time RTOS is overkill for what I'm actually trying to achieve. On the most part, my 
objectives are to read sensors, update some sort of display and maybe read serial data from somewhere. The problem with this is, sometimes (Like 
reading the serial stream from a GPS), some of these tasks are blocking and that induces a delay with every other action I want to perform.

This simple task dispatcher (Yes, I know similar things have been written before) suits my requirements in a simplistic way. 

This is a work in progress
