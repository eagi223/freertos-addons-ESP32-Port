# FreeRTOS C++ for ESP-IDF
Original project by michaelbecker is here: https://github.com/michaelbecker/freertos-addons

### Documentation
The original project documentation is here: https://michaelbecker.github.io/freertos-addons/cppdocs/html/index.html
and is almost completely still accurate. Differences are noted below

### Why a seperate repo?
I created this seperate repo because the original was targeted at the Linux FreeRTOS port and I didn't want to try to interfere with that by trying a PR with my changes. This makes it simpler to maintain and fix ESP-IDF only bugs. I'll try to keep up with new features from the original if possible and fix any bugs that are brought to my attention.

### What is different in this vs the original repo
This repo should be directly capable of interfacing with an existing ESP-IDF project. I've included the component.mk and the (untested) CMakeLists.txt file that should take care of adding all of the FreeRTOS-AddoOn source and includes into your project. I've also added an esp-idf-cfg.h file which checks the local project's sdkconfig to determine whether to set assertion or exceptions for errors. The call to start the scheduler was removed from the thread class as mentioned here: https://github.com/michaelbecker/freertos-addons/issues/23 by f00bard because the IDF calls this automatically. Finally, I changed the thread(task) and workqueue constructors to use the IDF xTaskCreatePinnedToCore macro instead of the xTaskCreate and added a coreID parameter to all relevant constructors. I've tested both thread and workqueue examples from the original project and they work fine (with some minor tweaks which I will get to below).

### Differences in class constructors
(see the original documentation for the differences among the different constructors for each class):
Thread:
Thread constructors now have a coreID parameter and are defined as: 
```C
  Thread( const std::string Name,
          uint16_t StackDepth,
          UBaseType_t Priority,
          core_id_t coreID);
          
  Thread( const char *Name,
          uint16_t StackDepth,
          UBaseType_t Priority,
          core_id_t coreID);
          
  Thread( uint16_t StackDepth,
        UBaseType_t Priority,
        core_id_t coreID);
```

Work Queue:
Work Queue constructors now have a coreID parameter which defaults to 1 if not set and are defined as:
```C
  WorkQueue(  const char * const Name,
              uint16_t StackDepth = DEFAULT_WORK_QUEUE_STACK_SIZE,
              UBaseType_t Priority = DEFAULT_WORK_QUEUE_PRIORITY,
              UBaseType_t MaxWorkItems = DEFAULT_MAX_WORK_ITEMS,
              core_id_t coreID = APP_CPU);
              
 WorkQueue(  uint16_t StackDepth = DEFAULT_WORK_QUEUE_STACK_SIZE,
            UBaseType_t Priority = DEFAULT_WORK_QUEUE_PRIORITY,
            UBaseType_t MaxWorkItems = DEFAULT_MAX_WORK_ITEMS,
            core_id_t coreID = APP_CPU);
```

Remember that you must implement a class that inherits from Thread that implements the Run() method as shown in the example below. 

### How to use:
Your project structure should follow the basic layout of the ESP-IDF template project here: https://github.com/espressif/esp-idf-template. In the project directory (not main, the one enclosing that) add a components directory. You can paste, git clone, etc. this project into the components directory so it looks similar to this:
```
|-components/
|   |-freertos-addons-esp32-port/
|       |-component.mk
|       |-CMakeLists.txt
|       |-freertos-addons/
|
|-main/
|   |-main.cpp //this can be a cpp file if you extern "C" app_main()
|   |-...
|
|-CMakeLists.txt
|-Makefile
|-...
```

in your project files, directly include the freertos-addons files like:
```C++
#include "thread.hpp"
```

A main.cpp example using a workqueue would look like:
```C++
#include <stdio.h>
#include <iostream>

#include "freertos/FreeRTOs.h"
#include "freertos/task.h"
#include "thread.hpp"
#include "ticks.hpp"
#include "workqueue.hpp"


using namespace cpp_freertos;
using namespace std;


class MyWorkItem : public WorkItem {

    public:
        MyWorkItem(int id, bool freeAfterComplete = false)
            : WorkItem(freeAfterComplete), Id(id)
        {
            WorkItemCount++;
        }

        ~MyWorkItem() 
        {
            WorkItemCount--;
        }

        void Run() 
        {
            cout << "[w:" << Id << "] " << WorkItemCount << " active." << endl;
        }

    private:
        int Id;
        static volatile int WorkItemCount;
};


volatile int MyWorkItem::WorkItemCount = 0;


class TestThread : public Thread {

    public:

        // The original example uses a thread with a stack of 100
        // That causes crashes for me, so I bumped it up to 1000

        // Notice we've added the parameter APP_CPU which is the 
        // coreID to the original example
        TestThread(int i, int delayInSeconds)
           : Thread("TestThread", 1000, 3, APP_CPU),
             id (i), 
             DelayInSeconds(delayInSeconds)
        {
            Start();
        };

    protected:

        virtual void Run() {

            cout << "Starting thread " << id << endl;

            //
            //  Low priority work queue - lower than this thread
            //

            // Notice we've added the parameter APP_CPU which is the 
            // coreID to the original example

            WorkQueue wq_low("wq_low", DEFAULT_WORK_QUEUE_STACK_SIZE, APP_CPU);

            //
            //  High priority work queue - higher than this thread
            //
            WorkQueue wq_high("wq_high", DEFAULT_WORK_QUEUE_STACK_SIZE, 5);

            MyWorkItem *work1;
            MyWorkItem *work2;
            int count = 1;
            
            while (true) {
            
                Delay(Ticks::SecondsToTicks(DelayInSeconds));
                cout << "\n[t:" << id <<"] making work"<< endl;

                work1 = new MyWorkItem(count++, true);
                work2 = new MyWorkItem(count++, true);

                wq_low.QueueWork(work1);
                wq_high.QueueWork(work2);

                work1 = new MyWorkItem(count++, true);
                wq_low.QueueWork(work1);

                work1 = new MyWorkItem(count++, true);
                wq_low.QueueWork(work1);

                cout << "[t" << id <<"] done\n"<< endl;
            }
        };

    private:
        int id;
        int DelayInSeconds;
};

// Make sure to extern "C" your main app 
extern "C" void app_main(void) {
    cout << "Testing FreeRTOS C++ wrappers" << endl;
    cout << "Workqueues" << endl;

    TestThread thread(1, 1);

    // This doesn't actually start the scheduler, but it does set a
    // set a flag so our threads know to run
    Thread::StartScheduler();

    // Not sure if this is necessary, but I think the idf creates a
    // task for app main. As long as we create a new thread either
    // with FreeRTOS (C) or FreeRTOS-AddOns (C++), we can safely
    // delete this. This will free up some memory and open up Core 0 
    // for more processor time
    vTaskDelete(NULL);
}
```

