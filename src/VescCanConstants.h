#ifndef RESCUE_VESCCANCONSTANTS_H
#define RESCUE_VESCCANCONSTANTS_H

#define BUFFER_SIZE 65535

// CAN commands
typedef enum {
	CAN_PACKET_SET_DUTY = 0,
	CAN_PACKET_SET_CURRENT,
	CAN_PACKET_SET_CURRENT_BRAKE,
	CAN_PACKET_SET_RPM,
	CAN_PACKET_SET_POS,
	CAN_PACKET_FILL_RX_BUFFER,
	CAN_PACKET_FILL_RX_BUFFER_LONG,
	CAN_PACKET_PROCESS_RX_BUFFER,
	CAN_PACKET_PROCESS_SHORT_BUFFER,
	CAN_PACKET_STATUS,
	CAN_PACKET_SET_CURRENT_REL,
	CAN_PACKET_SET_CURRENT_BRAKE_REL,
	CAN_PACKET_SET_CURRENT_HANDBRAKE,
	CAN_PACKET_SET_CURRENT_HANDBRAKE_REL,
	CAN_PACKET_STATUS_2,
	CAN_PACKET_STATUS_3,
	CAN_PACKET_STATUS_4,
	CAN_PACKET_PING,
	CAN_PACKET_PONG,
	CAN_PACKET_DETECT_APPLY_ALL_FOC,
	CAN_PACKET_DETECT_APPLY_ALL_FOC_RES,
	CAN_PACKET_CONF_CURRENT_LIMITS,
	CAN_PACKET_CONF_STORE_CURRENT_LIMITS,
	CAN_PACKET_CONF_CURRENT_LIMITS_IN,
	CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN,
	CAN_PACKET_CONF_FOC_ERPMS,
	CAN_PACKET_CONF_STORE_FOC_ERPMS,
	CAN_PACKET_STATUS_5,
	CAN_PACKET_POLL_TS5700N8501_STATUS,
	CAN_PACKET_CONF_BATTERY_CUT,
	CAN_PACKET_CONF_STORE_BATTERY_CUT,
	CAN_PACKET_SHUTDOWN,
	CAN_PACKET_IO_BOARD_ADC_1_TO_4,
	CAN_PACKET_IO_BOARD_ADC_5_TO_8,
	CAN_PACKET_IO_BOARD_ADC_9_TO_12,
	CAN_PACKET_IO_BOARD_DIGITAL_IN,
	CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL,
	CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM,
	CAN_PACKET_BMS_V_TOT,
	CAN_PACKET_BMS_I,
	CAN_PACKET_BMS_AH_WH,
	CAN_PACKET_BMS_V_CELL,
	CAN_PACKET_BMS_BAL,
	CAN_PACKET_BMS_TEMPS,
	CAN_PACKET_BMS_HUM,
	CAN_PACKET_BMS_SOC_SOH_TEMP_STAT,
	CAN_PACKET_PSW_STAT,
	CAN_PACKET_PSW_SWITCH,
	CAN_PACKET_BMS_HW_DATA_1,
	CAN_PACKET_BMS_HW_DATA_2,
	CAN_PACKET_BMS_HW_DATA_3,
	CAN_PACKET_BMS_HW_DATA_4,
	CAN_PACKET_BMS_HW_DATA_5,
	CAN_PACKET_BMS_AH_WH_CHG_TOTAL,
	CAN_PACKET_BMS_AH_WH_DIS_TOTAL,
	CAN_PACKET_UPDATE_PID_POS_OFFSET,
	CAN_PACKET_POLL_ROTOR_POS,
	CAN_PACKET_NOTIFY_BOOT,
	CAN_PACKET_STATUS_6,
	CAN_PACKET_GNSS_TIME,
	CAN_PACKET_GNSS_LAT,
	CAN_PACKET_GNSS_LON,
	CAN_PACKET_GNSS_ALT_SPEED_HDOP,
	CAN_PACKET_BMS_STATE,
	CAN_PACKET_MAKE_ENUM_32_BITS = 0xFFFFFFFF,
} CAN_PACKET_ID;

typedef enum {
	BMS_FAULT_CODE_NONE = 0,
	BMS_FAULT_CODE_PACK_OVER_VOLTAGE,
	BMS_FAULT_CODE_PACK_UNDER_VOLTAGE,
	BMS_FAULT_CODE_LOAD_OVER_VOLTAGE,
	BMS_FAULT_CODE_LOAD_UNDER_VOLTAGE,
	BMS_FAULT_CODE_CHARGER_OVER_VOLTAGE,
	BMS_FAULT_CODE_CHARGER_UNDER_VOLTAGE,
	BMS_FAULT_CODE_CELL_HARD_OVER_VOLTAGE,
	BMS_FAULT_CODE_CELL_HARD_UNDER_VOLTAGE,
	BMS_FAULT_CODE_CELL_SOFT_OVER_VOLTAGE,
	BMS_FAULT_CODE_CELL_SOFT_UNDER_VOLTAGE,
	BMS_FAULT_CODE_MAX_UVP_OVP_ERRORS,
	BMS_FAULT_CODE_MAX_UVT_OVT_ERRORS,
	BMS_FAULT_CODE_OVER_CURRENT,
	BMS_FAULT_CODE_OVER_TEMP_BMS,
	BMS_FAULT_CODE_UNDER_TEMP_BMS,
	BMS_FAULT_CODE_DISCHARGE_OVER_TEMP_CELLS,
	BMS_FAULT_CODE_DISCHARGE_UNDER_TEMP_CELLS,
	BMS_FAULT_CODE_CHARGE_OVER_TEMP_CELLS,
	BMS_FAULT_CODE_CHARGE_UNDER_TEMP_CELLS,
	BMS_FAULT_CODE_PRECHARGE_TIMEOUT,
	BMS_FAULT_CODE_DISCHARGE_RETRY,
	BMS_FAULT_CODE_CHARGE_RETRY,
	BMS_FAULT_CODE_CAN_DELAYED_POWER_DOWN,
	BMS_FAULT_CODE_NOT_USED_TIMEOUT,
	BMS_FAULT_CODE_CHARGER_DISCONNECT,
	BMS_FAULT_CODE_CHARGER_CURRENT_THRESHOLD_TIMEOUT
} bms_fault_state;

typedef enum {
        BMS_OP_STATE_INIT = 0,		// 0
	BMS_OP_STATE_CHARGING,		// 1
	BMS_OP_STATE_PRE_CHARGE,		// 2
	BMS_OP_STATE_LOAD_ENABLED,		// 3
	BMS_OP_STATE_BATTERY_DEAD,		// 4
	BMS_OP_STATE_POWER_DOWN,		// 5
	BMS_OP_STATE_EXTERNAL,		// 6
	BMS_OP_STATE_ERROR,			// 7
	BMS_OP_STATE_ERROR_PRECHARGE,	// 8
	BMS_OP_STATE_BALANCING,		// 9
	BMS_OP_STATE_CHARGED,		// 10
	BMS_OP_STATE_FORCEON,		// 11
	BMS_OP_STATE_UNKNOWN = 255
} bms_op_state;

typedef struct {
	float v_tot;
	float v_charge;
	float i_in;
	float i_in_ic;
	float ah_cnt;
	float wh_cnt;
	int cell_num;
	float v_cell[32];
	bool bal_state[32];
	int temp_adc_num;
	float temps_adc[10];
	float temp_ic;
	float temp_hum;
	float hum;
	float temp_max_cell;
	float soc;
	float soh;
	int can_id;
	uint32_t update_time;
	bms_op_state op_state;
	bms_fault_state fault_state;
} bms_values;

#endif //RESCUE_VESCCANCONSTANTS_H
