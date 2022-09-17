/****************************************************************************
 *
 *  Copyright (c) 2017, Michael Becker (michael.f.becker@gmail.com)
 *
 *  This file is part of the FreeRTOS Add-ons project.
 *
 *  Source Code:
 *  https://github.com/michaelbecker/freertos-addons
 *
 *  Project Page:
 *  http://michaelbecker.github.io/freertos-addons/
 *
 *  On-line Documentation:
 *  http://michaelbecker.github.io/freertos-addons/docs/html/index.html
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files
 *  (the "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so,subject to the
 *  following conditions:
 *
 *  + The above copyright notice and this permission notice shall be included
 *    in all copies or substantial portions of the Software.
 *  + Credit is appreciated, but not required, if you find this project
 *    useful enough to include in your application, product, device, etc.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/


#include "tickhook.hpp"

#if ( configUSE_TICK_HOOK == 1 )

#if ( CONFIG_FREERTOS_LEGACY_HOOKS != 1 )
#include <esp_freertos_hooks.h>
#endif

using namespace std;
using namespace cpp_freertos;


list<TickHook *> TickHook::Callbacks;
portMUX_TYPE tick_hook_spinlock = portMUX_INITIALIZER_UNLOCKED;

TickHook::TickHook()
    : Enabled(true)
{
#if ( CONFIG_FREERTOS_LEGACY_HOOKS != 1 )
    esp_register_freertos_tick_hook(RunAllCallbacks);
#endif
}


TickHook::~TickHook()
{
    portENTER_CRITICAL(&tick_hook_spinlock);
    Callbacks.remove(this);
    portEXIT_CRITICAL(&tick_hook_spinlock);
}


void TickHook::Register()
{
    portENTER_CRITICAL(&tick_hook_spinlock);
    Callbacks.push_front(this);
    portEXIT_CRITICAL(&tick_hook_spinlock);
}


void TickHook::Disable()
{
    portENTER_CRITICAL(&tick_hook_spinlock);
    Enabled = false;
    portEXIT_CRITICAL(&tick_hook_spinlock);
}


void TickHook::Enable()
{
    portENTER_CRITICAL(&tick_hook_spinlock);
    Enabled = true;
    portEXIT_CRITICAL(&tick_hook_spinlock);
}

void TickHook::RunAllCallbacks() {
    for (list<TickHook *>::iterator it = TickHook::Callbacks.begin();
         it != TickHook::Callbacks.end();
         ++it) {

        TickHook *tickHookObject = *it;

        if (tickHookObject->Enabled){
            tickHookObject->Run();
        }
    }
}


#if ( CONFIG_FREERTOS_LEGACY_HOOKS == 1 )
/**
 *  We are a friend of the Tick class, which makes this much simplier.
 */
void vApplicationTickHook(void)
{
    TickHook::RunAllCallbacks();
}
#endif

#endif


