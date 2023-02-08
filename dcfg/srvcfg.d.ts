declare module "@devicescript/srvcfg" {
    type integer = number
    type Pin = integer | string
    type HexInt = integer | string

    type ServiceConfig = RotaryEncoderConfig | ButtonConfig | RelayConfig

    interface DeviceConfig {
        "$schema"?: string
        
        /**
         * Name of the device.
         * 
         * @example "Acme Corp. SuperIoT v1.3"
         */
        devName: string

        /**
         * Device class code, typically given as a hex number starting with 0x3.
         * 
         * @example "0x379ea214"
         */
        devClass: HexInt

        /**
         * Services to mount.
         */
        _?: ServiceConfig[]
    }

    interface BaseConfig {
        service: string

        /**
         * Instance/role name to be assigned to service.
         * @example "buttonA"
         */
        name?: string

        /**
         * Service variant (see service definition for possible values).
         */
        variant?: integer
    }

    interface RotaryEncoderConfig extends BaseConfig {
        service: "rotary"
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
}
