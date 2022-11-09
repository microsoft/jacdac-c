# Jacdac Client SDK

## Multi-threading considerations

Most of the functions are meant to be run from Jacdac thread.
Some functions can be called from anywhere, including interrupts.

If there's no RTOS there is only one thread and there's nothing to worry about.

In normal circumstances, the Jacdac thread runs approximately every 10ms.
This includes running servers' `*_process()` functions as well as client process
function and processing any incoming packets.

In non-RTOS environment any interrupt wakes the main thread breaking its 10ms sleep.
This includes incoming packets and external events.
When running under an RTOS, the macro `JD_WAKE_MAIN()` is meant to wake the Jacdac thread 
from its 10ms sleep.

## Service clients

Service clients are meant to make consumption of Jacdac easier when running under an RTOS.
In particular, service client methods are safe to call from any thread.
The methods can also block until a response is available.

## Usage scenarios

* on service-bound set some config registers
* on service-unbound notify user
* on incoming sample record it somewhere (and maybe interact with some other service)
* on incoming event from service (change event, or sth) interact with other service or the same

## Thoughts

* jacdac main thread + jacdac callback thread
* figure out thread-safety of client functions - should be mostly all right? protect stuff with a mutex?

* derive hclient -> hclient - reference count in the parent - for multi-threaded operation

* short timeouts
* connection timeout vs service response timeout
* handlers shouldn't block too much; RTOS devs are probably used to this?

* handlers blocking too much

* race between creating client, setting userdata etc, and events firing on it