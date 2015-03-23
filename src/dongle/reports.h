#ifndef REPORTS_H
#define REPORTS_H

// we need to tell the compiler to pack the struct members as close as possible
#ifdef _MSC_VER
# pragma pack(push)
# pragma pack(1)
#endif

// This is the main HID joystick report.
// It contains the axis data we're sending to the PC.

#define JOYSTICK_REPORT_ID				1

typedef struct
{
	uint8_t	report_id;		// == JOYSTICK_REPORT_ID

	int16_t	x;
	int16_t	y;
	int16_t	z;

} hid_joystick_report_t;

// the HID keyboard report
extern hid_joystick_report_t	usb_joystick_report;

void reset_joystick_report(void);

// *****************************************************************
// *****************************************************************
// *****************************************************************

#define DONGLE_SETTINGS_REPORT_ID			2

#define MAG_MATRIX_SCALE_BITS			12

// data direction: dongle <-> PC
typedef struct
{
	uint8_t		report_id;		// == DONGLE_SETTINGS_REPORT_ID

	uint8_t		autocenter;		// 0 to 3 for autocenter values
	uint8_t		is_linear;		// linear or exponential

	// axis factors
	int16_t		fact[3];

	// drift compensation per 1024 samples
	int16_t		drift_per_1k;
	
	// magnetometer calibration
	int16_t		mag_offset[3];
	int16_t		mag_matrix[3][3];

} FeatRep_DongleSettings;

// *****************************************************************
// *****************************************************************
// *****************************************************************

#define COMMAND_REPORT_ID		3

enum head_tracker_commands_t
{
	// the first two are sent to the head tracker over radio
	CMD_CALIBRATE				= 1,
	CMD_READ_TRACKER_SETTINGS	= 2,
	
	// these are send from the PC to the dongle
	CMD_RECENTER				= 3,
	CMD_SAVE_DRIFT				= 4,
	CMD_INC_DRIFT_COMP			= 5,
	CMD_DEC_DRIFT_COMP			= 6,
	
	/*
	// these are sent from the config program, through the dongle to the tracker
	CMD_RF_PWR_LOWEST			= 7,
	CMD_RF_PWR_LOWER			= 8,
	CMD_RF_PWR_HIGHER			= 9,
	CMD_RF_PWR_HIGHEST			= 10,
	*/
};

// direction: PC -> dongle
typedef struct
{
	uint8_t		report_id;		// == COMMAND_REPORT_ID
	uint8_t		command;
} FeatRep_Command;

// *****************************************************************
// *****************************************************************
// *****************************************************************

#define TRACKER_SETTINGS_REPORT_ID		4

// direction: dongle -> PC
typedef struct
{
	uint8_t		report_id;		// == TRACKER_SETTINGS_REPORT_ID

	uint8_t		has_tracker_responded;
	
	uint8_t		is_calibrated;	// if gyro and accel have been calibrated

	int16_t		gyro_bias[3];
	int16_t		accel_bias[3];
	
	uint8_t		rf_power;		// CMD_RF_PWR_LOWEST
								// CMD_RF_PWR_LOWER
								// CMD_RF_PWR_HIGHER
								// CMD_RF_PWR_HIGHEST
} FeatRep_TrackerSettings;

// *****************************************************************
// *****************************************************************
// *****************************************************************

#define STATUS_REPORT_ID				5

// direction: dongle -> PC
typedef struct
{
	uint8_t		report_id;		// == STATUS_REPORT_ID

	uint8_t		num_packets;	// number of packets received in the last second
	
	int32_t		sample_cnt;		// number of samples since reset
	int32_t		yaw_value;		// current yaw value
	
	uint16_t	battery_voltage;
	uint16_t	temperature;

} FeatRep_Status;

// *****************************************************************
// *****************************************************************
// *****************************************************************

#define RAW_MAG_REPORT_ID		6

// direction: dongle -> PC
typedef struct
{
	int16_t		x;
	int16_t		y;
	int16_t		z;
} mag_sample_t;

#define MAX_RAW_MAG_SAMPLES		10

typedef struct
{
	uint8_t			report_id;		// == RAW_MAG_REPORT_ID
	
	uint8_t			num_samples;
	mag_sample_t	mag[MAX_RAW_MAG_SAMPLES];

} FeatRep_RawMagSamples;

// *****************************************************************
// *****************************************************************
// *****************************************************************

#ifdef _MSC_VER
# pragma pack(pop)
#endif

#endif	// REPORTS_H