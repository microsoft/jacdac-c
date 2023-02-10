declare module "@devicescript/srvcfg" {
    type integer = number
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

    interface DeviceConfig {
        $schema?: string

        /**
         * Name of the device.
         *
         * @examples ["Acme Corp. SuperIoT v1.3"]
         */
        devName: string

        /**
         * Device class code, typically given as a hex number starting with 0x3.
         *
         * @examples ["0x379ea214"]
         */
        devClass: HexInt

        pinJacdac?: Pin

        led?: LedConfig

        /**
         * Services to mount.
         */
        _?: ServiceConfig[]
    }

    interface ArchConfig {
        $schema?: string

        /**
         * Short identification of architecture.
         *
         * @examples ["rp2040", "rp2040w", "esp32c3"]
         */
        id: string

        /**
         * Full name of architecture.
         *
         * @examples ["RP2040 + CYW43 WiFi", "ESP32-C3"]
         */
        name: string

        /**
         * Where to find a generic (without DCFG) UF2 or BIN file.
         */
        genericUrl?: string

        /**
         * Where should DCFG be embedded in generic file.
         */
        dcfgOffset: HexInt
    }

    interface LedConfig {
        /**
         * If a single mono LED.
         */
        pin?: Pin

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
         * Other options (in range 100+) are also possible on some boards.
         *
         * @default 0
         */
        type?: number
    }

    interface LedChannel {
        pin: Pin
        /**
         * Multiplier to compensate for different LED strengths.
         * @minimum 0
         * @maximum 255
         */
        mult?: integer
    }

    interface BaseConfig {
        service: string

        /**
         * Instance/role name to be assigned to service.
         * @examples ["buttonA", "activityLed"]
         */
        name?: string

        /**
         * Service variant (see service definition for possible values).
         */
        variant?: integer
    }

    interface RotaryEncoderConfig extends BaseConfig {
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

    interface ButtonConfig extends BaseConfig {
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

    interface RelayConfig extends BaseConfig {
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

    interface PowerConfig extends BaseConfig {
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

    interface AnalogConfig extends BaseConfig {
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
