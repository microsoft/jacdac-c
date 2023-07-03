declare module "@devicescript/srvcfg" {
    type integer = number
    /**
     * Hardware pin number or label.
     * Naming convention - fields with `Pin` type start with `pin*`
     */
    type Pin = integer | string
    type IOPin = Pin
    type InputPin = Pin
    type OutputPin = Pin
    type AnalogInPin = Pin
    type HexInt = integer | string

    type ServiceConfig =
        | RotaryEncoderConfig
        | ButtonConfig
        | SwitchConfig
        | FlexConfig
        | RelayConfig
        | PowerConfig
        | MotionConfig
        | MotorConfig
        | LightBulbConfig
        | BuzzerConfig
        | ServoConfig
        | LightLevelConfig
        | ReflectedLightConfig
        | WaterLevelConfig
        | SoundLevelConfig
        | SoilMoistureConfig
        | PotentiometerConfig
        | AccelerometerConfig
        | HidJoystickConfig
        | HidKeyboardConfig
        | HidMouseConfig
        | GamepadConfig

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

        /**
         * Enable legacy auto-scan for I2C devices.
         */
        scanI2C?: boolean

        pins?: PinLabels

        /**
         * Initial values for pins.
         */
        sPin?: Record<string, 0 | 1>
    }

    interface JsonComment {
        /**
         * All fields starting with '#' arg ignored
         */
        "#"?: string
    }

    interface JacdacConfig extends JsonComment {
        $connector?: "Jacdac" | "Header"
        pin: IOPin
    }

    interface I2CConfig extends JsonComment {
        pinSDA: IOPin
        pinSCL: OutputPin

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
        pinTX: OutputPin

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
        pin?: OutputPin

        /**
         * Clock pin, if any.
         */
        pinCLK?: OutputPin

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
        pin: OutputPin
        /**
         * Multiplier to compensate for different LED strengths.
         * @minimum 0
         * @maximum 255
         */
        mult?: integer
    }

    /**
     * Typical pin labels, others are allowed.
     * Pin labels starting with '@' are hidden to the user.
     */
    interface PinLabels extends JsonComment {
        TX?: integer
        RX?: integer
        VP?: integer
        VN?: integer
        BOOT?: integer
        PWR?: integer
        LED_PWR?: integer
        LED?: integer
        LED0?: integer
        LED1?: integer
        LED2?: integer
        LED3?: integer

        CS?: integer

        A0?: integer
        A1?: integer
        A2?: integer
        A3?: integer
        A4?: integer
        A5?: integer
        A6?: integer
        A7?: integer
        A8?: integer
        A9?: integer
        A10?: integer
        A11?: integer
        A12?: integer
        A13?: integer
        A14?: integer
        A15?: integer

        D0?: integer
        D1?: integer
        D2?: integer
        D3?: integer
        D4?: integer
        D5?: integer
        D6?: integer
        D7?: integer
        D8?: integer
        D9?: integer
        D10?: integer
        D11?: integer
        D12?: integer
        D13?: integer
        D14?: integer
        D15?: integer

        P0?: integer
        P1?: integer
        P2?: integer
        P3?: integer
        P4?: integer
        P5?: integer
        P6?: integer
        P7?: integer
        P8?: integer
        P9?: integer
        P10?: integer
        P11?: integer
        P12?: integer
        P13?: integer
        P14?: integer
        P15?: integer
        P16?: integer
        P17?: integer
        P18?: integer
        P19?: integer
        P20?: integer
        P21?: integer
        P22?: integer
        P23?: integer
        P24?: integer
        P25?: integer
        P26?: integer
        P27?: integer
        P28?: integer
        P29?: integer
        P30?: integer
        P31?: integer
        P32?: integer
        P33?: integer
        P34?: integer
        P35?: integer
        P36?: integer
        P37?: integer
        P38?: integer
        P39?: integer
        P40?: integer
        P41?: integer
        P42?: integer
        P43?: integer
        P44?: integer
        P45?: integer
        P46?: integer
        P47?: integer
        P48?: integer
        P49?: integer
        P50?: integer
        P51?: integer
        P52?: integer
        P53?: integer
        P54?: integer
        P55?: integer
        P56?: integer
        P57?: integer
        P58?: integer
        P59?: integer
        P60?: integer
        P61?: integer
        P62?: integer
        P63?: integer

        GP0?: integer
        GP1?: integer
        GP2?: integer
        GP3?: integer
        GP4?: integer
        GP5?: integer
        GP6?: integer
        GP7?: integer
        GP8?: integer
        GP9?: integer
        GP10?: integer
        GP11?: integer
        GP12?: integer
        GP13?: integer
        GP14?: integer
        GP15?: integer
        GP16?: integer
        GP17?: integer
        GP18?: integer
        GP19?: integer
        GP20?: integer
        GP21?: integer
        GP22?: integer
        GP23?: integer
        GP24?: integer
        GP25?: integer
        GP26?: integer
        GP27?: integer
        GP28?: integer
        GP29?: integer
        GP30?: integer
        GP31?: integer
        GP32?: integer
        GP33?: integer
        GP34?: integer
        GP35?: integer
        GP36?: integer
        GP37?: integer
        GP38?: integer
        GP39?: integer
        GP40?: integer
        GP41?: integer
        GP42?: integer
        GP43?: integer
        GP44?: integer
        GP45?: integer
        GP46?: integer
        GP47?: integer
        GP48?: integer
        GP49?: integer
        GP50?: integer
        GP51?: integer
        GP52?: integer
        GP53?: integer
        GP54?: integer
        GP55?: integer
        GP56?: integer
        GP57?: integer
        GP58?: integer
        GP59?: integer
        GP60?: integer
        GP61?: integer
        GP62?: integer
        GP63?: integer

        // RP2040
        GP26_A0?: integer
        GP27_A1?: integer
        GP28_A2?: integer

        // Xiao/QT-Py pinout
        A0_D0?: integer
        A1_D1?: integer
        A2_D2?: integer
        A3_D3?: integer
        SDA_D4?: integer
        SCL_D5?: integer
        TX_D6?: integer
        RX_D7?: integer
        SCK_D8?: integer
        MISO_D9?: integer
        MOSI_D10?: integer

        // Feather pin names (in addition to A0, A1, ..., D10, ...)
        A4_D24?: integer
        A5_D25?: integer
        RX_D0?: integer
        TX_D1?: integer
        SDA?: integer
        SCL?: integer
        MISO?: integer
        MOSI?: integer
        SCK?: integer
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
        pin0: InputPin
        pin1: InputPin
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
        pin: InputPin
        /**
         * This pin is set high when the button is pressed.
         */
        pinBacklight?: OutputPin
        /**
         * Button is normally active-low and pulled high.
         * This makes it active-high and pulled low.
         */
        activeHigh?: boolean
    }

    interface SwitchConfig extends BaseServiceConfig {
        service: "switch"
        pin: InputPin
    }

    interface FlexConfig extends BaseServiceConfig {
        service: "flex"
        pinL: OutputPin
        pinM: AnalogInPin
        pinH: OutputPin
    }

    interface RelayConfig extends BaseServiceConfig {
        service: "relay"

        /**
         * The driving pin.
         */
        pin: OutputPin

        /**
         * When set, the relay is considered 'active' when `pin` is low.
         */
        activeLow?: boolean

        /**
         * Active-high pin that indicates the actual state of the relay.
         */
        pinFeedback?: InputPin

        /**
         * This pin will be driven when relay is active.
         */
        pinLed?: OutputPin

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

    interface LightBulbConfig extends BaseServiceConfig {
        service: "lightBulb"

        /**
         * The driving pin.
         */
        pin: OutputPin

        /**
         * When set, the relay is considered 'active' when `pin` is low.
         */
        activeLow?: boolean

        /**
         * When set, the pin will be operated with PWM at a few kHz.
         */
        dimmable?: boolean
    }

    interface BuzzerConfig extends BaseServiceConfig {
        service: "buzzer"

        /**
         * The driving pin.
         */
        pin: OutputPin

        /**
         * When set, the `pin` will be set to `1` when no sound is playing
         * (because power flows through speaker when `pin` is set to `0`).
         * When unset, the `pin` will be set to `0` when no sound is playing.
         */
        activeLow?: boolean
    }

    interface MotorConfig extends BaseServiceConfig {
        service: "motor"

        /**
         * The channel 1 pin.
         */
        pin1: OutputPin

        /**
         * The channel 2 pin.
         */
        pin2?: OutputPin

        /**
         * The enable (NSLEEP) pin if any, active high.
         */
        pinEnable?: OutputPin
    }

    interface ServoConfig extends BaseServiceConfig {
        service: "servo"

        /**
         * The driving (PWM) pin.
         */
        pin: OutputPin

        /**
         * The pin to set low when servo is used. Floating otherwise.
         */
        powerPin?: OutputPin

        /**
         * When set, the min/maxAngle/Pulse can't be changed by the user.
         */
        fixed?: boolean

        /**
         * Minimum angle supported by the servo in degrees.
         * Always minAngle < maxAngle
         *
         * @default -90
         */
        minAngle?: number

        /**
         * Pulse value to use to reach minAngle in us.
         * Typically minPulse > maxPulse
         *
         * @default 2500
         */
        minPulse?: number

        /**
         * Maximum angle supported by the servo in degrees.
         * Always minAngle < maxAngle
         *
         * @default 90
         */
        maxAngle?: number

        /**
         * Pulse value to use to reach maxAngle in us.
         * Typically minPulse > maxPulse
         *
         * @default 600
         */
        maxPulse?: number
    }

    interface MotionConfig extends BaseServiceConfig {
        service: "motion"

        /**
         * The input pin.
         */
        pin: InputPin

        /**
         * When set, the sensor is considered 'active' when `pin` is low.
         */
        activeLow?: boolean

        /**
         * Sensing angle in degrees.
         *
         * @default 120
         */
        angle?: number

        /**
         * Maximum sensing distance in centimeters.
         *
         * @default 1200
         */
        minDistance?: number
    }

    interface PowerConfig extends BaseServiceConfig {
        service: "power"

        /**
         * Always active low.
         */
        pinFault: InputPin
        pinEn: OutputPin
        /**
         * Active-low pin for pulsing battery banks.
         */
        pinPulse?: OutputPin
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
        pinLedPulse?: OutputPin

        /**
         * Pin that is high when we are connected to USB or similar power source.
         */
        pinUsbDetect?: InputPin
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

    interface GamepadConfig extends BaseServiceConfig {
        service: "gamepad"

        // Digital

        /**
         * Pin for button Left
         */
        pinLeft?: InputPin
        /**
         * Pin for button Up
         */
        pinUp?: InputPin
        /**
         * Pin for button Right
         */
        pinRight?: InputPin
        /**
         * Pin for button Down
         */
        pinDown?: InputPin
        /**
         * Pin for button A
         */
        pinA?: InputPin
        /**
         * Pin for button B
         */
        pinB?: InputPin
        /**
         * Pin for button Menu
         */
        pinMenu?: InputPin
        /**
         * Pin for button Select
         */
        pinSelect?: InputPin
        /**
         * Pin for button Reset
         */
        pinReset?: InputPin
        /**
         * Pin for button Exit
         */
        pinExit?: InputPin
        /**
         * Pin for button X
         */
        pinX?: InputPin
        /**
         * Pin for button Y
         */
        pinY?: InputPin

        /**
         * Buttons are normally active-low and pulled high.
         * This makes them active-high and pulled low.
         */
        activeHigh?: boolean

        // Analog
        /**
         * When gamepad is analog, set this to horizontal pin.
         */
        pinAX?: AnalogInPin

        /**
         * When gamepad is analog, set this to vertical pin.
         */
        pinAY?: AnalogInPin

        /**
         * Pin to pull low before analog read and release afterwards.
         */
        pinLow?: OutputPin

        /**
         * Pin to pull high before analog read and release afterwards.
         */
        pinHigh?: OutputPin
    }

    interface AnalogConfig extends BaseServiceConfig {
        /**
         * Pin to analog read.
         */
        pin: AnalogInPin

        /**
         * Pin to pull low before analog read and release afterwards.
         */
        pinLow?: OutputPin

        /**
         * Pin to pull high before analog read and release afterwards.
         */
        pinHigh?: OutputPin

        /**
         * Reading is `offset + (raw_reading * scale) / 1024`
         *
         * @default 0
         */
        offset?: integer

        /**
         * Reading is `offset + (raw_reading * scale) / 1024`
         *
         * @default 1024
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

    interface HidMouseConfig extends BaseServiceConfig {
        service: "hidMouse"
    }
    interface HidKeyboardConfig extends BaseServiceConfig {
        service: "hidKeyboard"
    }
    interface HidJoystickConfig extends BaseServiceConfig {
        service: "hidJoystick"
    }
}
