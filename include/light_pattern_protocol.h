/**********************************************************************

  light_pattern_protocol.h - implementation of communications interface
    agreed upon by lighting system users. This is the file which will be 
    updated when interface changes are required, and is not meant to be
    portable, but rather a very application-specific implementation.


  Authors: 
    Nate Fisher

  Created: 
    Wed Oct 1, 2014

**********************************************************************/

#ifndef  LIGHT_PATTERN_PROTOCOL_H
#define  LIGHT_PATTERN_PROTOCOL_H

#include "pattern_generator.h"
#include "utilities.h"

// for use in solo oreoleds, this number must match the period defined 
//   in the synchro clock header
#define MAX_PATTERN_PERIOD 4000.0

// Nonce used to verify reset command is valid
#define RESET_NONCE		0x2A

// Preset colour mixes
#define COLOUR_MAX		0xFF

#define COLOUR_WHITE_R	COLOUR_MAX*0.50
#define COLOUR_WHITE_G	COLOUR_MAX*0.60
#define COLOUR_WHITE_B	COLOUR_MAX*0.28

#define COLOUR_AMBER_R	COLOUR_MAX*1.00
#define COLOUR_AMBER_G	COLOUR_MAX*0.40
#define COLOUR_AMBER_B	0

typedef enum _Light_Protocol_Parameter {
    PARAM_BIAS_RED,             // 0
    PARAM_BIAS_GREEN,           // 1
    PARAM_BIAS_BLUE,            // 2
    PARAM_AMPLITUDE_RED,        // 3
    PARAM_AMPLITUDE_GREEN,      // 4
    PARAM_AMPLITUDE_BLUE,       // 5
    PARAM_PERIOD,               // 6
    PARAM_REPEAT,               // 7
    PARAM_PHASEOFFSET,          // 8
    PARAM_MACRO,                // 9
    PARAM_RESET,                // 10
    PARAM_ENUM_COUNT            // 11
} LightProtocolParameter;

typedef enum _Light_Param_Macro {
    PARAM_MACRO_RESET,          // 0
    PARAM_MACRO_FWUPDATE,       // 1
    PARAM_MACRO_BREATHE,		// 2
    PARAM_MACRO_CALIBRATE,      // 3
    PARAM_MACRO_POWERON,        // 4
    PARAM_MACRO_POWEROFF,       // 5
    PARAM_MACRO_RED,            // 6
    PARAM_MACRO_GREEN,          // 7
    PARAM_MACRO_BLUE,           // 8
    PARAM_MACRO_AMBER,          // 9
    PARAM_MACRO_WHITE,				// 10
    PARAM_MACRO_AUTOMOBILE_COLORS,  // 11
    PARAM_MACRO_AVIATION_COLORS,    // 12
    PARAM_MACRO_ENUM_COUNT      // 13
} LightParamMacro;

static const short int LightParameterSize[PARAM_ENUM_COUNT] = {
    1,  // Bias 
    1,  // Bias 
    1,  // Bias   
    1,  // Amp 
    1,  // Amp 
    1,  // Amp 
    2,  // Period
    1,  // Repeat
    2,  // Phase Offset
    1,  // Param Macro
    1,  // Param Reset
};

typedef struct _Light_Pattern_Protocol {
    uint8_t isCommandFresh;
	int8_t	cyclesRemaining;
    PatternGenerator* redPattern;
    PatternGenerator* greenPattern;
    PatternGenerator* bluePattern;
} LightPatternProtocol;

LightPatternProtocol LPP_pattern_protocol;

uint8_t LPP_processBuffer(void);
void LPP_setParamMacro(LightParamMacro);
void _LPP_processParameterUpdate(LightProtocolParameter, int);
void _LPP_setPattern(int);


#endif
