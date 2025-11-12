#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_

/* 1. CAN Configuration (TODO: Update IDs from their CAN_IDs.h) */
#define CAN_ID_AD2CAN_F1_1 0x510 // (افتراض - يجب تحديثه)
#define CAN_ID_AD2CAN_F1_2 0x511 // (افتراض - يجب تحديثه)
#define CAN_ID_DV_SENSORS_FRONT 0x512 // (افتراض - يجب تحديثه)
#define CAN_ID_AD2CAN_STARTUP 0x700 // (افتراض - يجب تحديثه)

// (هذا ID جديد لخاصية الفرامل التي أضفناها)
#define CAN_ID_RX_BRAKE   0x200
#define CAN_RX_FILTER_MASK 0x7FF

/* 2. Sensor Calculation Constants (منسوخة من كودهم) */
#define PRESSURE_MAX_ADC_VAL 	3610
#define PRESSURE_MIN_ADC_VAL 	400
#define PRESSURE_MAX_PRESSURE	20000

#define STEERING_RAW_LEFT 	1736
#define STEERING_RAW_RIGHT 	3561

/* 3. ADC Configuration */
#define ADC_NUMBER_OF_CHANNELS 6 // !!! مهم: 6 قنوات فقط !!!
#define ADC_VREF                3.3f
#define ADC_MAX_VALUE           4095.0f

/* 4. RPM Filter Configuration (منسوخة من كودهم) */
#define WS_FILTER_SIZE 10

/* 5. Task Configuration */
#define CAN_TX_PERIOD_MS 100

#endif /* INC_APP_CONFIG_H_ */
