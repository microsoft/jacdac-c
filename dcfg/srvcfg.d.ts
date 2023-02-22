declare module "@devicescript/srvcfg" {
    type integer = number
    /**
     * Hardware pin number. In future we may allow labels.
     * Naming convention - fields with `Pin` type start with `pin*`
     */
    type Pin = integer
    type HexInt = integer | string

    type ServiceConfig =
        | RotaryEncoderConfig
        | ButtonConfig
        | RelayConfig
        | PowerConfig
        | LightLevelConfig
        | ReflectedLightConfig
        | WaterLevelConfig
        | SoundLevelConfig
        | SoilMoistureConfig
        | PotentiometerConfig
        | AccelerometerConfig

    interface DeviceHardwareInfo {
        /**
         * Name of the device.
         *
         * @examples ["Acme Corp. SuperIoT v1.3"]
         */
        devName: string

        /**
         * Identifier for a particular firmware running on a particular device, typically given as a hex number starting with 0x3.
         *
         * @examples ["0x379ea214"]
         */
        productId: HexInt

        jacdac?: JacdacConfig

        led?: LedConfig

        log?: LogConfig

        i2c?: I2CConfig

        pins?: PinLabels
    }

    interface JsonComment {
        /**
         * All fields starting with '#' arg ignored
         */
        "#"?: string
    }

    interface JacdacConfig extends JsonComment {
        $connector?: "Jacdac" | "Header"
        pin: Pin
    }

    interface I2CConfig extends JsonComment {
        pinSDA: Pin
        pinSCL: Pin

        /**
         * How I2C devices are attached.
         */
        $connector?: "Qwiic" | "Grove" | "Header"

        /**
         * Hardware instance index.
         */
        inst?: integer

        /**
         * How fast to run the I2C, typically either 100 or 400.
         *
         * @default 100
         */
        kHz?: integer
    }


    interface LogConfig extends JsonComment {
        /**
         * Where to send logs.
         */
        pinTX: Pin

        /**
         * Speed to use.
         *
         * @default 115200
         */
        baud?: integer
    }

    interface LedConfig extends JsonComment {
        /**
         * If a single mono LED, or programmable RGB LED.
         */
        pin?: Pin

        /**
         * Clock pin, if any.
         */
        pinCLK?: Pin

        /**
         * RGB LED driven by PWM.
         *
         * @minItems 3
         * @maxItems 3
         */
        rgb?: LedChannel[]

        /**
         * Applies to all LED channels/pins.
         */
        activeHigh?: boolean

        /**
         * Defaults to `true` if `pin` is set and `type` is 0.
         * Otherwise `false`.
         */
        isMono?: boolean

        /**
         * 0 - use `pin` or `rgb` as regular pins
         * 1 - use `pin` as WS2812B (supported only on some boards)
         * 2 - use `pin` as APA102 DATA, and `pinCLK` as CLOCK
         * 3 - use `pin` as SK9822 DATA, and `pinCLK` as CLOCK
         * Other options (in range 100+) are also possible on some boards.
         *
         * @default 0
         */
        type?: number
    }

    interface LedChannel extends JsonComment {
        pin: Pin
        /**
         * Multiplier to compensate for different LED strengths.
         * @minimum 0
         * @maximum 255
         */
        mult?: integer
    }

    interface PinLabels extends JsonComment {
        TX?: Pin
        RX?: Pin
        VP?: Pin
        VN?: Pin
        BOOT?: Pin
        LED?: Pin
        LED0?: Pin
        LED1?: Pin
        LED2?: Pin
        LED3?: Pin

        SDA?: Pin
        SCL?: Pin

        MISO?: Pin
        MOSI?: Pin
        SCK?: Pin
        CS?: Pin

        A0?: Pin
        A1?: Pin
        A2?: Pin
        A3?: Pin
        A4?: Pin
        A5?: Pin
        A6?: Pin
        A7?: Pin
        A8?: Pin
        A9?: Pin
        A10?: Pin
        A11?: Pin
        A12?: Pin
        A13?: Pin
        A14?: Pin
        A15?: Pin

        D0?: Pin
        D1?: Pin
        D2?: Pin
        D3?: Pin
        D4?: Pin
        D5?: Pin
        D6?: Pin
        D7?: Pin
        D8?: Pin
        D9?: Pin
        D10?: Pin
        D11?: Pin
        D12?: Pin
        D13?: Pin
        D14?: Pin
        D15?: Pin

        P0?: Pin
        P1?: Pin
        P2?: Pin
        P3?: Pin
        P4?: Pin
        P5?: Pin
        P6?: Pin
        P7?: Pin
        P8?: Pin
        P9?: Pin
        P10?: Pin
        P11?: Pin
        P12?: Pin
        P13?: Pin
        P14?: Pin
        P15?: Pin
        P16?: Pin
        P17?: Pin
        P18?: Pin
        P19?: Pin
        P20?: Pin
        P21?: Pin
        P22?: Pin
        P23?: Pin
        P24?: Pin
        P25?: Pin
        P26?: Pin
        P27?: Pin
        P28?: Pin
        P29?: Pin
        P30?: Pin
        P31?: Pin
        P32?: Pin
        P33?: Pin
        P34?: Pin
        P35?: Pin
        P36?: Pin
        P37?: Pin
        P38?: Pin
        P39?: Pin
        P40?: Pin
        P41?: Pin
        P42?: Pin
        P43?: Pin
        P44?: Pin
        P45?: Pin
        P46?: Pin
        P47?: Pin
        P48?: Pin
        P49?: Pin
        P50?: Pin
        P51?: Pin
        P52?: Pin
        P53?: Pin
        P54?: Pin
        P55?: Pin
        P56?: Pin
        P57?: Pin
        P58?: Pin
        P59?: Pin
        P60?: Pin
        P61?: Pin
        P62?: Pin
        P63?: Pin
    }

    interface BaseServiceConfig extends JsonComment {
        service: string

        /**
         * Instance/role name to be assigned to service.
         * @examples ["buttonA", "activityLed"]
         */
        name: string

        /**
         * Service variant (see service definition for possible values).
         */
        variant?: integer
    }

    interface RotaryEncoderConfig extends BaseServiceConfig {
        service: "rotaryEncoder"
        pin0: Pin
        pin1: Pin
        /**
         * How many clicks for full 360 turn.
         * @default 12
         */
        clicksPerTurn?: integer
        /**
         * Encoder supports "half-clicks".
         */
        dense?: boolean
        /**
         * Invert direction.
         */
        inverted?: boolean
    }

    interface ButtonConfig extends BaseServiceConfig {
        service: "button"
        pin: Pin
        /**
         * This pin is set high when the button is pressed.
         */
        pinBacklight?: Pin
        /**
         * Button is normally active-low and pulled high.
         * This makes it active-high and pulled low.
         */
        activeHigh?: boolean
    }

    interface RelayConfig extends BaseServiceConfig {
        service: "relay"

        /**
         * The driving pin.
         */
        pin: Pin

        /**
         * When set, the relay is considered 'active' when `pin` is low.
         */
        pinActiveLow?: boolean

        /**
         * Active-high pin that indicates the actual state of the relay.
         */
        pinFeedback?: Pin

        /**
         * This pin will be driven when relay is active.
         */
        pinLed?: Pin

        /**
         * Which way to drive the `pinLed`
         */
        ledActiveLow?: boolean

        /**
         * Whether to activate the relay upon boot.
         */
        initalActive?: boolean

        /**
         * Maximum switching current in mA.
         */
        maxCurrent?: integer
    }

    interface PowerConfig extends BaseServiceConfig {
        service: "power"

        /**
         * Always active low.
         */
        pinFault: Pin
        pinEn: Pin
        /**
         * Active-low pin for pulsing battery banks.
         */
        pinPulse?: Pin
        /**
         * Operation mode of pinEn
         * 0 - `pinEn` is active high
         * 1 - `pinEn` is active low
         * 2 - transistor-on-EN setup, where flt and en are connected at limiter
         * 3 - EN should be pulsed at 50Hz (10ms on, 10ms off) to enable the limiter
         */
        mode: integer

        /**
         * How long (in milliseconds) to ignore the `pinFault` after enabling.
         *
         * @default 16
         */
        faultIgnoreMs: integer

        /**
         * Additional power LED to pulse.
         */
        pinLedPulse?: Pin

        /**
         * Pin that is high when we are connected to USB or similar power source.
         */
        pinUsbDetect?: Pin
    }

    interface AccelerometerConfig extends BaseServiceConfig {
        service: "accelerometer"

        /**
         * Transform for X axis. Values of 1, 2, 3 will return the corresponding raw axis (indexed from 1).
         * Values -1, -2, -3 will do likewise but will negate the axis.
         *
         * @default 1
         */
        trX?: integer

        /**
         * Transform for Y axis. Values of 1, 2, 3 will return the corresponding raw axis (indexed from 1).
         * Values -1, -2, -3 will do likewise but will negate the axis.
         *
         * @default 2
         */
        trY?: integer

        /**
         * Transform for Z axis. Values of 1, 2, 3 will return the corresponding raw axis (indexed from 1).
         * Values -1, -2, -3 will do likewise but will negate the axis.
         *
         * @default 3
         */
        trZ?: integer
    }

    interface AnalogConfig extends BaseServiceConfig {
        /**
         * Pin to analog read.
         */
        pin: Pin

        /**
         * Pin to pull low before analog read and release afterwards.
         */
        pinLow?: Pin

        /**
         * Pin to pull high before analog read and release afterwards.
         */
        pinHigh?: Pin

        /**
         * Reading is `offset + (raw_reading * scale) / 1024`
         *
         * @default 1024
         */
        offset?: integer

        /**
         * Reading is `offset + (raw_reading * scale) / 1024`
         *
         * @default 0
         */
        scale?: integer

        /**
         * Interval in milliseconds between samplings of the sensor.
         *
         * @default 9
         */
        samplingItv?: integer

        /**
         * How many milliseconds to wait after setting `pinLow`/`pinHigh` before sampling.
         *
         * @default 0
         */
        samplingDelay?: integer

        /**
         * Default interval in milliseconds between streaming samples.
         *
         * @default 100
         */
        streamingItv?: integer
    }

    interface LightLevelConfig extends AnalogConfig {
        service: "analog:lightLevel"
    }

    interface ReflectedLightConfig extends AnalogConfig {
        service: "analog:reflectedLight"
    }

    interface WaterLevelConfig extends AnalogConfig {
        service: "analog:waterLevel"
    }

    interface SoundLevelConfig extends AnalogConfig {
        service: "analog:soundLevel"
    }

    interface SoilMoistureConfig extends AnalogConfig {
        service: "analog:soilMoisture"
    }

    interface PotentiometerConfig extends AnalogConfig {
        service: "analog:potentiometer"
    }
}
