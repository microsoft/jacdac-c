# Jacdac firmware library


This library implements platform-agnostic aspects of the  [Jacdac](https://aka.ms/jacdac) protocol.
It's meant to be included as submodule, and use the build system of the parent repository.

It's currently used in the following projects:
* https://github.com/microsoft/jacdac-stm32x0 (which has some better docs on building)
* https://github.com/microsoft/jacdac-esp32 (which is quite experimental)

This library is part of [Jacdac Device Development Kit](https://github.com/microsoft/jacdac-ddk).

## Adding new services

It's best to start from an existing service, and do a search-replace (eg., `servo -> rocket`)
* [services/servo.c](services/servo.c) has a simple example of registers
* [services/buzzer.c](services/buzzer.c) has a simple example of how a command is handled (in `buzzer_handle_packet()`)
* [services/thermometer.c](services/thermometer.c) is a very simple sensor
* [services/power.c](services/power.c) is a more involved sensor (with custom registers)

Once you add a service, make sure to add its `*_init()` function to 
[services/jd_services.h](services/jd_services.h).


## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
