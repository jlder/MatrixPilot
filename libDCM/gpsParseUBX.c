// This file is part of MatrixPilot.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009-2011 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "libDCM.h"
#include "gpsData.h"
#include "gpsParseCommon.h"
#include "../libUDB/serialIO.h"
#include "../libUDB/magnetometer.h"
#include "mag_drift.h"
#include "rmat.h"
#include "hilsim.h"

// Ajout gfm pout Aid_Ini
union longbbbb p_Acc_;// 30m
union longbbbb time_Acc_;
union longbbbb f_Acc_;
union longbbbb clkd_Acc_;
union longbbbb Flags_;// position precise:0x21
union longbbbb clkd_;
union longbbbb ftow_;
union intbb tncfg_;
uint8_t CK_A;
uint8_t CK_B;
uint16_t  AID_INI_length = 56;
// PVT Variables needed
union intbb year_, pdop_;
uint8_t month_, day_, hour_, min_, sec_;
union longbbbb nano_;
uint8_t fixtype_,flags_,flags2_;
union longbbbb height_;
union longbbbb vAcc_;
union longbbbb velN_,velE_,velD_,gSpeed_,headMot_,sAcc_,headAcc_;
// RelPosNED Variables needed
union longbbbb 	relposN_,relposE_,relposD_,relposLength_,relposHead_;
uint8_t relposHPN_,relposHPE_,relposHPD_,relposHPL_;     // High-precision components of relative position vector.
union longbbbb 	accN_,accE_,accD_,accL_,accH_;
union longbbbb 	FlagsRPN_;     // 

// fin ajout gfm
void send_msg_AID_INI(uint8_t* AID_INI)
{
	CK_A = 0;
	CK_B = 0;

        AID_INI[0]=0xB5;
        AID_INI[1]=0x62;
        AID_INI[2]=0x0B;
        AID_INI[3]=0x01;
        AID_INI[4]=0x30;
        AID_INI[5]=0x00;
        AID_INI[6]=lat_gps.__.B0;
        AID_INI[7]=lat_gps.__.B1;
        AID_INI[8]=lat_gps.__.B2;
        AID_INI[9]=lat_gps.__.B3;// lon
        AID_INI[10]=lon_gps.__.B0;
        AID_INI[11]=lon_gps.__.B1;
        AID_INI[12]=lon_gps.__.B2;
        AID_INI[13]=lon_gps.__.B3;                   // lat
        AID_INI[14]=alt_sl_gps.__.B0;
        AID_INI[15]=alt_sl_gps.__.B1;
        AID_INI[16]=alt_sl_gps.__.B2;
        AID_INI[17]=alt_sl_gps.__.B3;             // hMSL
        AID_INI[18]=p_Acc_.__.B0;
        AID_INI[19]=p_Acc_.__.B1;
        AID_INI[20]=p_Acc_.__.B2;
        AID_INI[21]=p_Acc_.__.B3;                       // pAcc
        AID_INI[22]=tncfg_._.B0;
        AID_INI[23]=tncfg_._.B1;                          //tncfg
        AID_INI[24]=week_no._.B0;
        AID_INI[25]=week_no._.B1;                     // week
        AID_INI[26]=tow.__.B0;
        AID_INI[27]=tow.__.B1;
        AID_INI[28]=tow.__.B2;
        AID_INI[29]=tow.__.B3; // iTOW
        AID_INI[30]=ftow_.__.B0;
        AID_INI[31]=ftow_.__.B1;
        AID_INI[32]=ftow_.__.B2;
        AID_INI[33]=ftow_.__.B3, // fTOW
        AID_INI[34]=time_Acc_.__.B0;
        AID_INI[35]=time_Acc_.__.B1;
        AID_INI[36]=time_Acc_.__.B2;
        AID_INI[37]=time_Acc_.__.B3;                 // Time Accuracy
        AID_INI[38]=f_Acc_.__.B0;
        AID_INI[39]=f_Acc_.__.B1;
        AID_INI[40]=f_Acc_.__.B2;
        AID_INI[41]=f_Acc_.__.B3;                       // f Accuracy
        AID_INI[42]=clkd_.__.B0;
        AID_INI[43]=clkd_.__.B1;
        AID_INI[44]=clkd_.__.B2;
        AID_INI[45]=clkd_.__.B3;                         // Clock Drift
        AID_INI[46]=clkd_Acc_.__.B0;
        AID_INI[47]=clkd_Acc_.__.B1;
        AID_INI[48]=clkd_Acc_.__.B2;
        AID_INI[49]=clkd_Acc_.__.B3;                 // ClkD Accuracy
        if (tow_.WW==0) {
              AID_INI[50]=0x21;}// Only position is precise
        else {AID_INI[50]=0x23;}// Both time and positions are precise
        AID_INI[51]=Flags_.__.B1;
        AID_INI[52]=Flags_.__.B2;
        AID_INI[53]=Flags_.__.B3;                       // Flags
        int i;
        for (i=0;i<AID_INI_length-2;i++)
        {
            CK_A += AID_INI[i];
            CK_B += CK_A;
        }
        AID_INI[54]=CK_A;                                           // Checksum
        AID_INI[55]=CK_B;                                           // Checksum
}
// fin modif gfm


#if (GPS_TYPE == GPS_UBX_2HZ || GPS_TYPE == GPS_UBX_4HZ || GPS_TYPE == GPS_UBX_10HZ|| GPS_TYPE == GPS_ALL)

// Parse the GPS messages, using the binary interface.
// The parser uses a state machine implemented via a pointer to a function.
// Binary values received from the GPS are directed to program variables via a table
// of pointers to the variable locations.
// Unions of structures are used to be able to access the variables as long, ints, or bytes.

static union intbb payloadlength;
static union intbb checksum;
//Modif gfm
//static uint16_t msg_class;
//static uint16_t msg_id;
static uint8_t msg_class;
static uint8_t msg_id;
//static uint8_t msg_sync1;
//static uint8_t msg_sync2;
// fin modif gfm
static uint16_t ack_class; // set but never used - RobD
static uint16_t ack_id; // set but never used - RobD
static uint16_t ack_type; // set but never used - RobD
//static uint8_t CK_A;
//static uint8_t CK_B;

static void msg_B3(uint8_t inchar);
static void msg_SYNC1(uint8_t inchar);
static void msg_SYNC2(uint8_t inchar);
static void msg_CLASS(uint8_t inchar);
static void msg_ID(uint8_t inchar);
static void msg_PL1(uint8_t inchar);
static void msg_POSLLH(uint8_t inchar);
static void msg_DOP(uint8_t inchar);
static void msg_SOL(uint8_t inchar);
static void msg_VELNED(uint8_t inchar);
static void msg_CS0(uint8_t inchar);
static void msg_CS1(uint8_t inchar);
// Modif gfm
static void msg_AID_INI(uint8_t inchar);
static void msg_PVT(uint8_t inchar);
static void msg_RELPOSNED(uint8_t inchar);
// UDB to send AID_INI message to GPS UBX
// fin modif gfm

#if (HILSIM == 1)
	static void msg_BODYRATES(uint8_t inchar);
	static void msg_KEYSTROKE(uint8_t gpschar);
#endif

static void msg_MSGU(uint8_t inchar);
static void msg_ACK_CLASS(uint8_t inchar);
static void msg_ACK_ID(uint8_t inchar);

//void bin_out(char outchar);

const char bin_mode_withnmea[] = "$PUBX,41,1,0003,0003,19200,0*21\r\n"; // turn on UBX + NMEA, 19200 baud
const char bin_mode_nonmea[] = "$PUBX,41,1,0003,0001,19200,0*23\r\n";   // turn on UBX only, 19200 baud
const char disable_GSV[] = "$PUBX,40,GSV,0,0,0,0,0,0*59\r\n"; //Disable the $GPGSV NMEA message
const char disable_VTG[] = "$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n"; //Disable the $GPVTG NMEA message
const char disable_GLL[] = "$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n"; //Disable the $GPGLL NMEA message
const char disable_GSA[] = "$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n"; //Disable the $GPGSA NMEA message

#if (GPS_TYPE == GPS_UBX_4HZ)
const uint8_t set_rate[] = {
	0xB5, 0x62, // Header
	0x06, 0x08, // ID
	0x06, 0x00, // Payload Length
	0xFA, 0x00, // measRate 4 Hz
	0x01, 0x00, // navRate
	0x01, 0x00, // timeRef
	0x10, 0x96  // Checksum
};
#elif (GPS_TYPE == GPS_UBX_10HZ)
const uint8_t set_rate[] = {
	0xB5, 0x62, // Header
	0x06, 0x08, // ID
	0x06, 0x00, // Payload Length
	0x64, 0x00, // measRate 10 Hz
	0x01, 0x00, // navRate
	0x01, 0x00, // timeRef
	0x7A, 0x12  // Checksum
};
#else
const uint8_t set_rate[] = {
	0xB5, 0x62, // Header
	0x06, 0x08, // ID
	0x06, 0x00, // Payload Length
	0xF4, 0x01, // measRate 2 Hz
	0x01, 0x00, // navRate
	0x01, 0x00, // timeRef
	0x0B, 0x77  // Checksum
};
#endif

const uint8_t enable_UBX_only[] = {
	0xB5, 0x62, // Header
	0x06, 0x00, // ID
	0x14, 0x00, // Payload length
	0x01,       // Port ID
	0x00,       // res0
	0x00, 0x00, // res1
	0xD0, 0x08, 0x00, 0x00, // mode
	0x00, 0x4B, 0x00, 0x00, // baudrate
	0x03, 0x00, // inProtoMask
	0x01, 0x00, // outProtoMask
	0x00, 0x00, // Flags - reserved, set to 0
	0x00, 0x00, // Pad - reserved, set to 0
	0x42, 0x2B  // checksum
};

const uint8_t enable_UBX_NMEA[] = {
	0xB5, 0x62, // Header
	0x06, 0x00, // ID
	0x14, 0x00, // Payload length
	0x01,       // Port ID
	0x00,       // res0
	0x00, 0x00, // res1
	0xD0, 0x08, 0x00, 0x00, // mode
	0x00, 0x4B, 0x00, 0x00, // baudrate
	0x03, 0x00, // inProtoMask
	0x03, 0x00, // outProtoMask
	0x00, 0x00, // Flags - reserved, set to 0
	0x00, 0x00, // Pad - reserved, set to 0
	0x44, 0x37  // checksum
};

const uint8_t enable_NAV_SOL[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x06,       // SOL message ID
	0x00,       // Rate on I2C
	0x01,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x17, 0xDA  // Checksum
};

const uint8_t enable_NAV_POSLLH[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x02,       // POSLLH message ID
	0x00,       // Rate on I2C
	0x01,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x13, 0xBE  // Checksum
};

const uint8_t enable_NAV_VELNED[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x12,       // VELNED message ID
	0x00,       // Rate on I2C
	0x01,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x23, 0x2E  // Checksum
};

#if (GPS_TYPE == GPS_UBX_4HZ)
const uint8_t enable_NAV_DOP[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x04,       // DOP message ID
	0x00,       // Rate on I2C
	0x04,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x18, 0xDB  // Checksum
};
#elif (GPS_TYPE == GPS_UBX_10HZ)
const uint8_t enable_NAV_PVT[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x07,       // DOP message ID
	0x00,       // Rate on I2C
	0x0A,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x21, 0x0E  // Checksum
};
const uint8_t enable_NAV_RELPOSNED[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x3C,       // DOP message ID
	0x00,       // Rate on I2C
	0x0A,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x56, 0x81  // Checksum
};
#else
const uint8_t enable_NAV_DOP[] = {
	0xB5, 0x62, // Header
	0x06, 0x01, // ID
	0x08, 0x00, // Payload length
	0x01,       // NAV message class
	0x04,       // DOP message ID
	0x00,       // Rate on I2C
	0x02,       // Rate on UART 1
	0x00,       // Rate on UART 2
	0x00,       // Rate on USB
	0x00,       // Rate on SPI
	0x00,       // Rate on ???
	0x16, 0xD1  // Checksum
};
#endif

const uint8_t enable_SBAS[] = {
	0xB5, 0x62, // Header
	0x06, 0x16, // ID
	0x08, 0x00, // Payload length
	0x01,       // Enable SBAS
	0x03,       //
	0x01,       //
	0x00,       //
	0x00,       //
	0x00,       //
	0x00,       //
	0x00,       //
	0x29, 0xAD  // Checksum
};

const uint8_t config_NAV5[] = {
	0xB5, 0x62, // Header
	0x06, 0x24, // ID
	0x24, 0x00, // Payload length
	0xFF, 0xFF, // Bit Mask, 0XFF means apply all of the config below
	0x08,       // Dynamic Model Number, Airborne with <4g Acceleration
	0x02,       // Position Fixing Mode. 3D only.
	0x00, 0x00, // Fixed altitude (mean sea level) for 2D fix mode only.
	0x00, 0x00, // Part of fixed altitude above (4 bytes in total)
	0x10, 0x27, // Fixed Altitude variance for 2D Mode (4 bytes)
	0x00, 0x00, //
	0x05, 0x00, // Minimum Elevation of Sats in degrees (1 byte). Dead Reckoning Limit in Seconds.
	0xFA, 0x00, // Position DOP Mask (2 bytes)
	0xFA, 0x00, // Time DOP Mask (2 bytes))
	0x64, 0x00, // Position Accuracy Mask (2 bytes) Meters)
	0x2C, 0x01, // Time Accuracy Mask (2 bytes))
	0x00, 0x00, // Static Hold Threshold (1 bytes). DGPS timeout (1 bytes))
	0x00, 0x00, // Reserved
	0x00, 0x00, // Reserved
	0x00, 0x00, // Reserved
	0x00, 0x00, // Reserved
	0x00, 0x00, // Reserved
	0x00, 0x00, // Reserved
	0x17, 0xFF  // Checksum
};

const uint16_t set_rate_length = 14;
const uint16_t enable_NAV_SOL_length = 16;
const uint16_t enable_NAV_POSLLH_length = 16;
const uint16_t enable_NAV_VELNED_length = 16;
const uint16_t enable_NAV_DOP_length = 16;
const uint16_t enable_NAV_PVT_length = 16;
const uint16_t enable_NAV_RELPOSNED_length = 16;
const uint16_t enable_UBX_only_length = 28;
const uint16_t enable_SBAS_length = 16;
const uint16_t config_NAV5_length = 44;

void (*msg_parse)(uint8_t gpschar) = &msg_B3;

uint8_t un;
//static union longbbbb xpg_, ypg_, zpg_;
//static union longbbbb xvg_, yvg_, zvg_;
//static uint8_t mode1_, mode2_;
static uint8_t svs_, nav_valid_;
//static union longbbbb lat_gps_, lon_gps_, alt_sl_gps_;
static union longbbbb sog_gps_, cog_gps_, climb_gps_;
//static union longbbbb tow_;
static union longbbbb as_sim_;
//static union intbb hdop_;
static union intbb week_no_;

uint8_t svsmin = 24;
uint8_t svsmax = 0;
static int16_t store_index = 0;
static int16_t nmea_passthru_countdown = 0; // used by nmea_passthru to count how many more bytes are passed through
static uint8_t nmea_passthrough_char = 0;
//static int16_t frame_errors = 0;

#if (HILSIM == 1)
static union intbb g_a_x_sim_, g_a_y_sim_, g_a_z_sim_;
static union intbb g_a_x_sim,  g_a_y_sim,  g_a_z_sim;
static union intbb p_sim_,     q_sim_,     r_sim_;
static union intbb p_sim,      q_sim,      r_sim;
static uint8_t x_ckey_, x_vkey_;
static void commit_bodyrate_data(void);
static void commit_keystroke_data(void);
#endif

#if (HILSIM == 1 && MAG_YAW_DRIFT == 1)
extern uint8_t magreg[6];
#endif

uint8_t* const msg_SOL_parse[] = {
	&tow_.__.B0, &tow_.__.B1, &tow_.__.B2, &tow_.__.B3, // iTOW
	&un, &un, &un, &un,                                 // fTOW
	&week_no_._.B0, &week_no_._.B1,                     // week
	&nav_valid_,                                        // gpsFix
	&un,                                                // flags
	&un, &un, &un, &un,                                 // ecefX
	&un, &un, &un, &un,                                 // ecefY
	&un, &un, &un, &un,                                 // ecefZ
#if (HILSIM == 1 && MAG_YAW_DRIFT == 1)
	&magreg[1], &magreg[0], &magreg[3], &magreg[2],     // simulate the magnetometer with HILSIM, and use these slots
	                                                    // note: mag registers come out high:low from magnetometer
#else
	&un, &un, &un, &un,                                 // pAcc
#endif
	&un, &un, &un, &un,                                 // ecefVX
	&un, &un, &un, &un,                                 // ecefVY
	&un, &un, &un, &un,                                 // ecefVZ

#if (HILSIM == 1 && MAG_YAW_DRIFT == 1)
	&magreg[5], &magreg[4], &un, &un,                   // simulate the magnetometer with HILSIM, and use these slots
	                                                    // note: mag registers come out high:low from magnetometer
#else
	&un, &un, &un, &un,                                 // sAcc
#endif
	&un, &un,                                           // pDOP
	&un,                                                // res1
	&svs_,                                              // numSV
	&un, &un, &un, &un,                                 // res2
};

uint8_t* const msg_DOP_parse[] = {
	&un, &un, &un, &un,                                 // iTOW
	&un, &un,                                           // gDOP
	&un, &un,                                           // pDOP
	&un, &un,                                           // tDOP
	&vdop_._.B0, &vdop_._.B1,                           // vDOP
	&hdop_._.B0, &hdop_._.B1,                           // hDOP
	&un, &un,                                           // nDOP
	&un, &un,                                           // eDOP
};

uint8_t* const msg_POSLLH_parse[] = {
	&un, &un, &un, &un,                                 // iTOW
	&lon_gps_.__.B0, &lon_gps_.__.B1,
	&lon_gps_.__.B2, &lon_gps_.__.B3,                   // lon
	&lat_gps_.__.B0, &lat_gps_.__.B1,
	&lat_gps_.__.B2, &lat_gps_.__.B3,                   // lat
	&un, &un, &un, &un,                                 // height
	&alt_sl_gps_.__.B0, &alt_sl_gps_.__.B1,
	&alt_sl_gps_.__.B2, &alt_sl_gps_.__.B3,             // hMSL
	&un, &un, &un, &un,                                 // hAcc
	&un, &un, &un, &un,                                 // vAcc
};

uint8_t* const msg_VELNED_parse[] = {
	&un, &un, &un, &un,                                 // iTOW
	&un, &un, &un, &un,                                 // velN
	&un, &un, &un, &un,                                 // velE
	&climb_gps_.__.B0, &climb_gps_.__.B1,
	&climb_gps_.__.B2, &climb_gps_.__.B3,               // velD
	&as_sim_.__.B0, &as_sim_.__.B1,
	&as_sim_.__.B2, &as_sim_.__.B3,                     // air speed
	&sog_gps_.__.B0, &sog_gps_.__.B1,
	&sog_gps_.__.B2, &sog_gps_.__.B3,                   // gSpeed
	&cog_gps_.__.B0, &cog_gps_.__.B1,
	&cog_gps_.__.B2, &cog_gps_.__.B3,                   // heading
	&un, &un, &un, &un,                                 // sAcc
	&un, &un, &un, &un,                                 // cAcc
};
// modif gfm : mmessage AID_INI definition

uint8_t* const msg_AID_INI_parse[] = {
	//0xB5, 0x62, // Header
	//0x0B, 0x01, // ID
	//0x30, 0x00, // Payload length
	&lon_gps_.__.B0, &lon_gps_.__.B1,
	&lon_gps_.__.B2, &lon_gps_.__.B3,                 // lon
	&lat_gps_.__.B0, &lat_gps_.__.B1,
	&lat_gps_.__.B2, &lat_gps_.__.B3,                   // lat
	&alt_sl_gps_.__.B0, &alt_sl_gps_.__.B1,
	&alt_sl_gps_.__.B2, &alt_sl_gps_.__.B3,             // hMSL
	&p_Acc_.__.B0, &p_Acc_.__.B1,
        &p_Acc_.__.B2, &p_Acc_.__.B3,                       // pAcc
	&tncfg_._.B0,&tncfg_._.B1,                          //tncfg
	&week_no_._.B0, &week_no_._.B1,                     // week
	&tow_.__.B0, &tow_.__.B1, &tow_.__.B2, &tow_.__.B3, // iTOW
	&ftow_.__.B0, &ftow_.__.B1, &ftow_.__.B2, &ftow_.__.B3, // fTOW
	&time_Acc_.__.B0, &time_Acc_.__.B1,
	&time_Acc_.__.B2, &time_Acc_.__.B3,                 // Time Accuracy
	&f_Acc_.__.B0, &f_Acc_.__.B1,
	&f_Acc_.__.B2, &f_Acc_.__.B3,                       // f Accuracy
	&clkd_.__.B0, &clkd_.__.B1,
	&clkd_.__.B2, &clkd_.__.B3,                         // Clock Drift
	&clkd_Acc_.__.B0, &clkd_Acc_.__.B1,
	&clkd_Acc_.__.B2, &clkd_Acc_.__.B3,                 // ClkD Accuracy
	&Flags_.__.B0, &Flags_.__.B1,
	&Flags_.__.B2, &Flags_.__.B3,                       // Flags
	//0xCC, 0x0B  // Checksum
};

uint8_t  AID_INI[] = {
	0xB5, 0x62, // Header
	0x0B, 0x01, // ID
	0x30, 0x00, // Payload length
	0x5A, 0x9F, 0x1B, 0x1d, //lon
	0x6C, 0x6D, 0x6F, 0x01, //lat
	0xD8, 0x27, 0x00, 0x00, //alt
	0xB8, 0x0B, 0x00, 0x00, //p_Acc
	0x00, 0x00, //tncfg
	0x00, 0x00, //week_no
	0x00, 0x00,0x00, 0x00, //tow
	0x00, 0x00,0x00, 0x00, //ftow
	0x00, 0x00,0x00, 0x00, //time_Acc
	0x00, 0x00,0x00, 0x00, //f_Acc
	0x00, 0x00,0x00, 0x00, //clkd
	0x00, 0x00,0x00, 0x00, //clkd_Acc
	0x21, 0x00,0x00, 0x00, //Flags
	0x59, 0xE7,  // Checksum
};

//extern uint8_t  AID_INI[];
uint8_t* const msg_PVT_parse[] = {
	&tow_.__.B0, &tow_.__.B1, &tow_.__.B2, &tow_.__.B3,// iTOW
    &year_._.B0,&year_._.B1,&month_, &day_,//Date
    &hour_,&min_,&sec_,//time
    &un,//time valid
    &time_Acc_.__.B0,&time_Acc_.__.B1,&time_Acc_.__.B2,&time_Acc_.__.B3,// time accuracy (ns))
    &nano_.__.B0,&nano_.__.B1,&nano_.__.B2,&nano_.__.B3,//
    &nav_valid_,&flags_,&flags2_,//nav_valid to replace FixType
    &svs_,
	&lon_gps_.__.B0, &lon_gps_.__.B1,&lon_gps_.__.B2, &lon_gps_.__.B3, // lon
	&lat_gps_.__.B0, &lat_gps_.__.B1,&lat_gps_.__.B2, &lat_gps_.__.B3, // lat
	&height_.__.B0, &height_.__.B1, &height_.__.B2, &height_.__.B3,     // height
	&alt_sl_gps_.__.B0, &alt_sl_gps_.__.B1,	&alt_sl_gps_.__.B2, &alt_sl_gps_.__.B3,// hMSL
	&p_Acc_.__.B0, &p_Acc_.__.B1, &p_Acc_.__.B2, &p_Acc_.__.B3,     // horizontal Accuracy
	&vAcc_.__.B0, &vAcc_.__.B1, &vAcc_.__.B2, &vAcc_.__.B3,     // vertical Accuracy
	&velN_.__.B0, &velN_.__.B1, &velN_.__.B2, &velN_.__.B3,     // North velocity
	&velE_.__.B0, &velE_.__.B1, &velE_.__.B2, &velE_.__.B3,     // East velocity
	&velD_.__.B0, &velD_.__.B1, &velD_.__.B2, &velD_.__.B3,     // Down velocity
	&gSpeed_.__.B0, &gSpeed_.__.B1, &gSpeed_.__.B2, &gSpeed_.__.B3,     // Ground speed
	&headMot_.__.B0, &headMot_.__.B1, &headMot_.__.B2, &headMot_.__.B3,     // Heading motio
	&sAcc_.__.B0, &sAcc_.__.B1, &sAcc_.__.B2, &sAcc_.__.B3,     // Speed Accuracy
	&headAcc_.__.B0, &headAcc_.__.B1, &headAcc_.__.B2, &headAcc_.__.B3,     // vertical Accuracy
	&pdop_._.B0, &pdop_._.B1,     // position dilution of precision
	&un, &un, &un, &un, &un, &un,                                 // reserved
	&un, &un, &un, &un,                                 // HeadVeh
	&un, &un,                                 // Mag Declination
	&un, &un                                 // Mag Accuracy
};
uint8_t* const msg_RELPOSNED_parse[] = {
    &un,// version
    &un,//reserved0
    &un, &un, //ref Station ID
	&tow_.__.B0, &tow_.__.B1, &tow_.__.B2, &tow_.__.B3,// iTOW
	&relposN_.__.B0, &relposN_.__.B1,&relposN_.__.B2, &relposN_.__.B3, // North component of relative position vector cm
	&relposE_.__.B0, &relposE_.__.B1,&relposE_.__.B2, &relposE_.__.B3, // Est component of relative position vector cm
	&relposD_.__.B0, &relposD_.__.B1, &relposD_.__.B2, &relposD_.__.B3,// Down component of relative position vector cm
	&relposLength_.__.B0, &relposLength_.__.B1,	&relposLength_.__.B2, &relposLength_.__.B3,// Length  of relative position vector cm
	&relposHead_.__.B0, &relposHead_.__.B1, &relposHead_.__.B2, &relposHead_.__.B3,     // Heading of relative position vector Accuracy 10-5 deg
	&un, &un, &un, &un,                                 // reserved
	&relposHPN_,&relposHPE_,&relposHPD_,&relposHPL_,     // High-precision components of relative position vector 0.1 mm.
	&accN_.__.B0, &accN_.__.B1,&accN_.__.B2, &accN_.__.B3, // Accuracy of relative position North component mm
	&accE_.__.B0, &accE_.__.B1,&accE_.__.B2, &accE_.__.B3, // Accuracy of relative position East component mm
	&accD_.__.B0, &accD_.__.B1, &accD_.__.B2, &accD_.__.B3,     // Accuracy of relative position Down component mm
	&accL_.__.B0, &accL_.__.B1,	&accL_.__.B2, &accL_.__.B3,// Accuracy of relative position Length mm
	&accH_.__.B0, &accH_.__.B1, &accH_.__.B2, &accH_.__.B3,     // Accuracy of relative position Heading 10-5 deg
	&un, &un, &un, &un,                                 // reserved
	&FlagsRPN_.__.B0, &FlagsRPN_.__.B1, &FlagsRPN_.__.B2, &FlagsRPN_.__.B3     // Flags RelPosNED
};
// fin modif gfm

#if (HILSIM == 1)
// These are the data being delivered from the hardware-in-the-loop simulator
uint8_t* const msg_BODYRATES_parse[] = {
	&p_sim_._.B0, &p_sim_._.B1,         // roll rate
	&q_sim_._.B0, &q_sim_._.B1,         // pitch rate
	&r_sim_._.B0, &r_sim_._.B1,         // yaw rate
	&g_a_x_sim_._.B0, &g_a_x_sim_._.B1, // x accel reading (grav - accel, body frame)
	&g_a_y_sim_._.B0, &g_a_y_sim_._.B1, // y accel reading (grav - accel, body frame)
	&g_a_z_sim_._.B0, &g_a_z_sim_._.B1, // z accel reading (grav - accel, body frame)
};
uint8_t* const msg_KEYSTROKE_parse[] = {
	&x_ckey_, &x_vkey_, // control code, virtual keystroke code
};
#endif // HILSIM

void gps_startup_sequence(int16_t gpscount)
{
	if (gpscount == 980)
	{
#if (HILSIM == 1)
		udb_gps_set_rate(HILSIM_BAUD);
#else
		udb_gps_set_rate(57600);
#endif
	}
#if (GPS_TYPE != GPS_UBX_10HZ)
	else if (dcm_flags._.nmea_passthrough && gpscount == 200)
		gpsoutline(disable_GSV);
	else if (dcm_flags._.nmea_passthrough && gpscount == 190)
		gpsoutline(disable_GSA);
	else if (dcm_flags._.nmea_passthrough && gpscount == 180)
		gpsoutline(disable_GLL);
	else if (dcm_flags._.nmea_passthrough && gpscount == 170)
		gpsoutline(disable_VTG);
	else if (dcm_flags._.nmea_passthrough && gpscount == 160)
		// set the UBX to use binary and nmea
		gpsoutline(bin_mode_withnmea);
	else if (!dcm_flags._.nmea_passthrough && gpscount == 160)
		// set the UBX to use binary mode
		gpsoutline(bin_mode_nonmea); // Set GPS baud rate 19200
	#if (HILSIM != 1)
        else if (gpscount == 150)
	         udb_gps_set_rate(19200);// UART1 baudrate = 19200 instead of default value 9600 of GT-635T default baudrate
    else if (gpscount == 140)
		gpsoutbin(set_rate_length, set_rate); // Is this command redundant with bin_mode_nonmea?
	else if (dcm_flags._.nmea_passthrough && gpscount == 130)
		gpsoutbin(enable_UBX_only_length, enable_UBX_NMEA);
	else if (!dcm_flags._.nmea_passthrough && gpscount == 130)
		gpsoutbin(enable_UBX_only_length, enable_UBX_only);
	else if (gpscount == 120)
		// command GPS to select which messages are sent, using UBX interface
		gpsoutbin(enable_NAV_SOL_length, enable_NAV_SOL);
	else if (gpscount == 120)
		gpsoutbin(enable_NAV_POSLLH_length, enable_NAV_POSLLH);
	else if (gpscount == 110)
		gpsoutbin(enable_NAV_POSLLH_length, enable_NAV_POSLLH);
	else if (gpscount == 100)
		gpsoutbin(enable_NAV_VELNED_length, enable_NAV_VELNED);
	else if (gpscount == 90)
		gpsoutbin(enable_NAV_DOP_length, enable_NAV_DOP);
	else if (dcm_flags._.nmea_passthrough && gpscount == 90)
		gpsoutbin(enable_UBX_only_length, enable_UBX_NMEA);
	else if (!dcm_flags._.nmea_passthrough && gpscount == 90)
		gpsoutbin(enable_UBX_only_length, enable_UBX_only);
	else if (gpscount == 80)
		gpsoutbin(enable_SBAS_length, enable_SBAS);
	else if (gpscount == 70)
		gpsoutbin(config_NAV5_length, config_NAV5);
       // modif gfm
	else if (gpscount == 60) 
       { // Ne faut-ilpas envoyer AID_INI que lorsque l'heure est envoy�e par le GCS?
//        send_msg_AID_INI(AID_INI);
//  		gpsoutbin(AID_INI_length, AID_INI);
        
        }
#endif
#endif
 // fin modif gfm
}

boolean gps_nav_valid(void)
{
	return (nav_valid_ >= 3);
}

boolean differential_gps(void)
{
	return ( (FlagsRPN_.__.B0 & 2 )== 2) ;
}


/*
int16_t hex_count = 0;
const char convert[] = "0123456789ABCDEF";
const char endchar = 0xB5;

// Used for debugging purposes, converts to HEX and outputs to the debugging USART
// Only the first 5 bytes following a B3 are displayed.
void hex_out(char outchar)
{
	if (hex_count > 0) 
	{
		U1TXREG = convert[((outchar>>4) & 0x0F)];
		U1TXREG = convert[(outchar & 0x0F)];
		U1TXREG = ' ';
		hex_count--;
	}
	if (outchar == endchar)
	{
		hex_count = 5;
		U1TXREG = '\r';
		U1TXREG = '\n';
	}
}
 */

// The parsing routines follow. Each routine is named for the state in which the routine is applied.
// States correspond to the portions of the binary messages.
// For example, msg_B3 is the routine that is applied to the byte received after a B3 is received.
// If an A0 is received, the state machine transitions to the A0 state.

void nmea_passthru(uint8_t gpschar)
{
	nmea_passthrough_char = gpschar;
	gpsoutbin(1, &nmea_passthrough_char);

	nmea_passthru_countdown--;
/* removed in favor of the line ending mechanism, see below. While this is compliant with
   (published) standards, the issue appears to center around the end of line.

	if (gpschar == '*')
	{ // * indicates the start of the checksum, 2 characters remain in the message
		nmea_passthru_countdown = 2;
	}
 */
	if (gpschar == 0x0A)
	{ // end of line appears to always be 0x0D, 0x0A (\r\n)
		msg_parse = &msg_B3; // back to the inital state
	}
	else if (nmea_passthru_countdown == 0)
	{
		msg_parse = &msg_B3; // back to the inital state
	}
}

static void msg_B3(uint8_t gpschar)
{
	if (gpschar == 0xB5)
	{
		//bin_out(0x01);
		msg_parse = &msg_SYNC1;
	}
	else if (dcm_flags._.nmea_passthrough && gpschar == '$' && udb_gps_check_rate(19200))
	{
		nmea_passthru_countdown = 128; // this limits the number of characters we will passthrough. (Most lines are 60-80 chars long.)
		msg_parse = &nmea_passthru;
		nmea_passthru (gpschar);
	}
	else
	{
		// error condition
	}
}

static void msg_SYNC1(uint8_t gpschar)
{
	if (gpschar == 0x62)
	{
		//bin_out(0x02);
		store_index = 0;
		msg_parse = &msg_SYNC2;
	}
	else
	{
		msg_parse = &msg_B3;        // error condition
	}
}

static void msg_SYNC2(uint8_t gpschar)
{
	//bin_out(0x03);
	msg_class = gpschar;
	CK_A = 0;
	CK_B = 0;
	CK_A += gpschar;
	CK_B += CK_A;
	msg_parse = &msg_CLASS;
}

static void msg_CLASS(uint8_t gpschar)
{
	//bin_out(0x04);
	msg_id = gpschar;
	CK_A += gpschar;
	CK_B += CK_A;
	msg_parse = &msg_ID;
}

static void msg_ID(uint8_t gpschar)
{
	//bin_out(0x05);
	payloadlength._.B0 = gpschar;   // UBX stored payload length in little endian order
	CK_A += gpschar;
	CK_B += CK_A;
	msg_parse = &msg_PL1;
}

static void msg_PL1(uint8_t gpschar)
{
	//bin_out(0x06);
	payloadlength._.B1 = gpschar;   // UBX stored payload length in little endian order
	CK_A += gpschar;
	CK_B += CK_A;

	switch (msg_class) {
		case 0x01 : {
			switch (msg_id) {
				case 0x02 : { // NAV_POSLLH message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_POSLLH_parse))
					{
						msg_parse = &msg_POSLLH;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
				case 0x04 : { // NAV_DOP message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_DOP_parse))
					{
						msg_parse = &msg_DOP;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
				case 0x06 : { // NAV_SOL message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_SOL_parse))
					{
						msg_parse = &msg_SOL;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
				case 0x12 : { // NAV_VELNED message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_VELNED_parse))
					{
						msg_parse = &msg_VELNED;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
// modif gfm
				case 0x07 : { // NAV_PVT message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_PVT_parse))
					{
						msg_parse = &msg_PVT;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
                case 0x0B : { // AID-INI message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_AID_INI_parse))
					{
						msg_parse = &msg_AID_INI;
					}
					else
					{
						msg_parse = &msg_B3;    // error condition
					}
					msg_parse = &msg_AID_INI;    // TODO: this does not look right (wipes out error setting above) - RobD
					break;
				}
				case 0x3C : { // NAV_RELPOSNED message
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_RELPOSNED_parse))
					{
						msg_parse = &msg_RELPOSNED;
					}
					else
					{
						gps_parse_errors++;
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
// fin modif gfm
#if (HILSIM == 1)
				case 0xAB : { // NAV_BODYRATES message - THIS IS NOT AN OFFICIAL UBX MESSAGE
					// WE ARE FAKING THIS FOR HIL SIMULATION
					if (payloadlength.BB  == NUM_POINTERS_IN(msg_BODYRATES_parse))
					{
						msg_parse = &msg_BODYRATES;
					}
					else
					{
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
				case 0xAC : { // NAV_KEYSTROKE message - THIS IS NOT AN OFFICIAL UBX MESSAGE
					// WE ARE FAKING THIS FOR HIL SIMULATION
					if (payloadlength.BB ==  NUM_POINTERS_IN(msg_KEYSTROKE_parse))
					{
						msg_parse = &msg_KEYSTROKE;
					}
					else
					{
						printf("NAV_KEYSTROKE, bad payloadlength %i != %i\r\n", payloadlength.BB, (int)NUM_POINTERS_IN(msg_KEYSTROKE_parse));
						msg_parse = &msg_B3;    // error condition
					}
					break;
				}
#endif
				default : {     // some other NAV class message
					msg_parse = &msg_MSGU;
					break;
				}
			}
			break;
		}
		case 0x05 : {
			switch (msg_id) {
				case 0x00 : {   // NACK message
					ack_type = 0;
					msg_parse = &msg_ACK_CLASS;
					break;
				}
				case 0x01 : {   // ACK message
					ack_type = 1;
					msg_parse = &msg_ACK_CLASS;
					break;
				}
				default : {     // There are no other messages in this class, so this is an error
					msg_parse = &msg_B3;
					break;
				}
			}
			break;
		}
		default : {             // a non NAV class message
			msg_parse = &msg_MSGU;
			break;
		}
	}
}

static void msg_POSLLH(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_POSLLH_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x08);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

static void msg_DOP(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_DOP_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x09);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

static void msg_SOL(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_SOL_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x0A);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

static void msg_VELNED(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_VELNED_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x0B);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}
// Modif gfm
static void msg_AID_INI(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_AID_INI_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x0B);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}
static void msg_PVT(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_PVT_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x0B);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}
static void msg_RELPOSNED(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_RELPOSNED_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x0B);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

#if (HILSIM == 1)
static void msg_BODYRATES(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_BODYRATES_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

static void msg_KEYSTROKE(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		*msg_KEYSTROKE_parse[store_index++] = gpschar;
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

#endif // HILSIM

static void msg_ACK_CLASS(uint8_t gpschar)
{
	//bin_out(0xAA);
	ack_class = gpschar;
	CK_A += gpschar;
	CK_B += CK_A;
	msg_parse = &msg_ACK_ID;
}

static void msg_ACK_ID(uint8_t gpschar)
{
	//bin_out(0xBB);
	ack_id = gpschar;
	CK_A += gpschar;
	CK_B += CK_A;
	msg_parse = &msg_CS0;
}

static void msg_MSGU(uint8_t gpschar)
{
	if (payloadlength.BB > 0)
	{
		CK_A += gpschar;
		CK_B += CK_A;
		payloadlength.BB--;
	}
	else
	{
		// If the payload length is zero, we have received the entire payload, or the payload length
		// was zero to start with. either way, the byte we just received is the first checksum byte.
		//gpsoutchar2(0x08);
		checksum._.B1 = gpschar;
		msg_parse = &msg_CS1;
	}
}

static void msg_CS0(uint8_t gpschar)
{
	checksum._.B1 = gpschar;
	msg_parse = &msg_CS1;
}

static void msg_CS1(uint8_t gpschar)
{
	checksum._.B0 = gpschar;
	if ((checksum._.B1 == CK_A) && (checksum._.B0 == CK_B))
	{
			gps_parse_common(); // parsing is complete, schedule navigation
#if (HILSIM == 1)
		else if (msg_id == 0xAB)
		{
			// If we got the correct checksum for bodyrates, commit that data immediately
			commit_bodyrate_data();
		}
		else if (msg_id == 0xAC)
		{
			// If we got the correct checksum for keystroke, commit that data immediately
			commit_keystroke_data();
		}
#endif
	}
	else
	{
		gps_parse_errors++;
		gps_data_age = GPS_DATA_MAX_AGE+1;  // if the checksum is wrong then the data from this packet is invalid. 
		                                    // setting this ensures the nav routine does not try to use this data.
	}
	msg_parse = &msg_B3;
}

void gps_update_basic_data(void)
{
//	week_no         = week_no_;
	svs             = svs_;
}

void gps_commit_data(void)
{
	//bin_out(0xFF);
	date_gps_.WW=100*year_.BB+100*month_+day_;
    if (week_no.BB == 0)
	{
		week_no.BB = calculate_week_num(date_gps_.WW);
	}
	tow             = tow_;
	lat_gps         = lat_gps_;
	lon_gps         = lon_gps_;
	alt_sl_gps.WW   = alt_sl_gps_.WW / 10;          // SIRF provides altMSL in cm, UBX provides it in mm
    vel_N.WW           = velN_.WW / 10;     //vel en mm/s -> cm/s
    vel_E.WW           = velE_.WW / 10;
    vel_D.WW           = velD_.WW / 10;
#if (GPS_TYPE == GPS_UBX_10HZ)
	sog_gps.WW      = gSpeed_.WW / 10;                // SIRF uses 2 byte SOG, UBX PVT in mm/s (max 65 m/s)
	cog_gps.WW      = (uint16_t)(headMot_.WW / 1000);// SIRF uses 2 byte COG, 10^-2 deg, UBX PVT 10^-5 deg in centi�me de degr�, (max 655 degr�s)
	climb_gps.WW    = - velD_.WW / 10;            // SIRF uses 2 byte climb rate, UBX provides 4 bytes
	hdop            = (uint8_t)(hdop_.BB / 20);     // SIRF scales HDOP by 5, UBX by 10^-2
	vdop		= (uint8_t)(vAcc_.WW / 20);    // Vertical accuracy is not vertical DOP but can be used instead
    relposN = relposN_;
    relposE = relposE_;
    relposD = relposD_;
#else    
	sog_gps.BB      = sog_gps_._.W0;                // SIRF uses 2 byte SOG, UBX provides 4 bytes
#if (HILSIM == 1)
	hilsim_airspeed.BB = as_sim_._.W0;              // provided by HILSIM, simulated airspeed
#endif
	cog_gps.BB      = (uint16_t)(cog_gps_.WW / 1000);// SIRF uses 2 byte COG, 10^-2 deg, UBX provides 4 bytes, 10^-5 deg

	climb_gps.BB    = - climb_gps_._.W0;            // SIRF uses 2 byte climb rate, UBX provides 4 bytes
	hdop            = (uint8_t)(hdop_.BB / 20);     // SIRF scales HDOP by 5, UBX by 10^-2
	vdop		= (uint8_t)(vdop_.BB / 20);
#endif
    
#if (HILSIM == 1)
	hilsim_airspeed.BB = as_sim_._.W0;              // provided by HILSIM, simulated airspeed
#endif

	// SIRF provides position in m, UBX provides cm
//	xpg.WW          = xpg_.WW / 100;
//	ypg.WW          = ypg_.WW / 100;
//	zpg.WW          = zpg_.WW / 100;
	// SIRF provides 2 byte velocity in m scaled by 8,
	// UBX provides 4 bytes in cm
//	xvg.BB          = (int16_t)(xvg_.WW / 100 * 8);
//	yvg.BB          = (int16_t)(yvg_.WW / 100 * 8);
//	zvg.BB          = (int16_t)(zvg_.WW / 100 * 8);
//	mode1           = mode1_;
//	mode2           = mode2_;
	svs             = svs_;

#if (HILSIM == 1 && MAG_YAW_DRIFT == 1)
	HILSIM_MagData(mag_drift_callback); // run the magnetometer computations
#endif // HILSIM
}
void init_gps_data(void)
{
	//bin_out(0xFF);
// as there is no absolute time in the UDB5, week_no and tow are initialized to 0
	week_no_.BB  = 0;
	tow_.WW      = 0;
	p_Acc_.WW    = 3000;       //pAcc
	tncfg_.BB    = 0;          //tncfg
	ftow_.WW     = 0;
	time_Acc_.WW =0;
    f_Acc_.WW    =0;           // f Accuracy
	clkd_.WW     =0;
    clkd_Acc_.WW =0;           // ClkD Accuracy
	Flags_.__.B0 =0x21;
    Flags_.__.B1 =0;
	Flags_.__.B2 =0;
    Flags_.__.B3 =0;            // Flags

    dcm_set_origin_location(-20073534,486224810,1522);
	lat_gps_         = lat_origin;
	lon_gps_        = lon_origin;
	alt_sl_gps_.WW   = alt_origin.WW;          // SIRF provides altMSL in cm, UBX provides it in mm gfm mm->cm
    relposN_.WW = 0;
    relposE_.WW = 0;
    relposD_.WW = 0;
    // � faire dans la fonction get_fixed_origin de flightplan.c
}

#if (HILSIM == 1)
static void commit_bodyrate_data(void)
{
	g_a_x_sim = g_a_x_sim_;
	g_a_y_sim = g_a_y_sim_;
	g_a_z_sim = g_a_z_sim_;
	p_sim = p_sim_;
	q_sim = q_sim_;
	r_sim = r_sim_;
}

void HILSIM_saturate(int16_t size, int16_t vector[3])
{
	// hardware 16 bit signed integer gyro and accelerometer data and offsets
	// are divided by 2 prior to subtracting offsets from values.
	// This is done to prevent overflow.
	// However, it limits range to approximately +- RMAX.
	// Data coming in from Xplane is in 16 bit signed integer format with range +-2*RMAX,
	// so it needs to be passed through a saturation computation that limits to +-RMAX.
	uint16_t index;
	for (index = 0; index < size; index ++)
	{
		if (vector[index ] > RMAX)
		{
			vector[index] = RMAX;
		}
		if (vector[index] < -RMAX)
		{
			vector[index] = -RMAX;
		}
	}
}

void commit_keystroke_data(void)
{
//	if ((x_vkey_ != 0) && ((x_ckey_ & 0x08) || (x_ckey_ & 0x00))) // key down or key repeat
	if ((x_vkey_ != 0) && ((x_ckey_ & 0x08) || (x_ckey_ == 0x00))) // key down or key repeat
	{
/*
xplm_ShiftFlag      1   The shift key is down
xplm_OptionAltFlag  2   The option or alt key is down
xplm_ControlFlag    4   The control key is down*
xplm_DownFlag       8   The key is being pressed down
xplm_UpFlag         16  The key is being released
 */
//		printf("HILSIM keystroke %u %02x\r\n", x_vkey_, x_ckey_);
		hilsim_handle_key_input(x_vkey_);
	}
}

void HILSIM_set_gplane(fractional gplane[])
{
	gplane[0] = g_a_x_sim.BB;
	gplane[1] = g_a_y_sim.BB;
	gplane[2] = g_a_z_sim.BB;
	HILSIM_saturate(3, gplane);
}

void HILSIM_set_omegagyro(void)
{
	omegagyro[0] = q_sim.BB;
	omegagyro[1] = p_sim.BB;
	omegagyro[2] = r_sim.BB;
	HILSIM_saturate(3, omegagyro);
}
#endif // HILSIM

void init_gps_ubx(void)
{
// modif gfm
        init_gps_data();
// fin modif gfm
}

#endif // (GPS_TYPE == GPS_UBX_2HZ || GPS_TYPE == GPS_UBX_4HZ || GPS_TYPE == GPS_UBX_10HZ || GPS_TYPE == GPS_ALL)
