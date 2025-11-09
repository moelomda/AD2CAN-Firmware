#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_

/* 1. CAN Configuration (Assumed) */
#define CAN_ID_TX_TEMPS   0x100
#define CAN_ID_RX_BRAKE   0x200
#define CAN_RX_FILTER_MASK 0x7FF // Match standard ID exactly

/* 2. NTC Calculation (Assumed) */
// Placeholder coefficients (Generic 10k NTC) - MUST BE UPDATED
#define NTC_COEFF_A 0.001129148f
#define NTC_COEFF_B 0.000234125f
#define NTC_COEFF_C 0.0000000876741f

// Assumed R15 = 47k pull-down - MUST BE UPDATED
#define NTC_DIVIDER_RESISTANCE  47000.0f
#define NTC_DIVIDER_VOLTAGE     3.3f
#define ADC_VREF                3.3f
#define ADC_MAX_VALUE           4095.0f

/* 3. ADC Rank to Logical Sensor Mapping (0-7 index) */
// Maps the DMA buffer index (Rank order) to the logical AN_IN_X index
#define ADC_NUMBER_OF_CHANNELS 8
#define ADC_RANK_TO_AN_IN_1   0 // Rank 1 (CH1) -> AN_IN_1
#define ADC_RANK_TO_AN_IN_2   1 // Rank 2 (CH2) -> AN_IN_2
#define ADC_RANK_TO_AN_IN_3   2 // Rank 3 (CH3) -> AN_IN_3
#define ADC_RANK_TO_AN_IN_4   7 // Rank 8 (CH15)-> AN_IN_4
#define ADC_RANK_TO_AN_IN_5   3 // Rank 4 (CH5) -> AN_IN_5
#define ADC_RANK_TO_AN_IN_6   4 // Rank 5 (CH11)-> AN_IN_6
#define ADC_RANK_TO_AN_IN_7   5 // Rank 6 (CH12)-> AN_IN_7
#define ADC_RANK_TO_AN_IN_8   6 // Rank 7 (CH14)-> AN_IN_8

/* 4. Task Configuration */
#define CAN_TX_PERIOD_MS 100

#endif /* INC_ADC_CONFIG_H_ */
