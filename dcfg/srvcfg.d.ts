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

    interface DeviceConfig extends DeviceHardwareInfo {
        $schema?: string

        /**
         * Services to mount.
         */
        _?: ServiceConfig[]
    }

    interface DeviceHardwareInfo {
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

        jacdac?: JacdacConfig

        led?: LedConfig

        i2c?: I2CConfig
    }

    interface JacdacConfig {
        pin: Pin
    }

    interface I2CConfig {
        pinSDA: Pin
        pinSCL: Pin

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

        /**
         * Force alignment of the last page in the patched UF2 file.
         * Set to 4096 on RP2040 because wof RP2040-E14.
         */
        uf2Align: HexInt
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

    interface BaseServiceConfig {
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
