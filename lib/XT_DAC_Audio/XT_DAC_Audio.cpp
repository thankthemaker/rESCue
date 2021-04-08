// File: XT_DAC_Audio.cpp
// Description: XTronical DAC Audio Library, currently supporting ESP32
// Version: DAC Audio V4.2.1
// Last Update: Oct-22-2019
//
// Patches by TEB. Sept 16, 2019, Sep-29-2019, Oct-10-2019, Oct-21-2019, Oct-22-2019
// Updates:
//   Revised dacWrite() in ontimer() ISR to use inline code. This prevents crash during EEPROM writes.
//   Fixed FillBuffer() so that buffer size can be successfully changed.
//   Eliminated Pop click noise at start and stop of audio playback.
//   Fixed Intermittant garbled audio.
//   Revised pointer declarations to use cpp nullptr instead of NULL/zero.
//   Added ClearAfterPlay (Mod by Steve).
//   Fixed XT_Wav_Class and XT_Sequence_Class sub-volume controls.
//
// May work with other processors and/or DAC's with or without modifications
// (c) XTronical 2018, Licensed under GNU GPL 3.0 and later, under this license absolutely no warranty given
// See www.xtronical.com for documentation
// This software is currently under active development (Jan 2018) and may change and break your code with new versions
// No responsibility is taken for this. Stick with the version that works for you, if you need newer commands from later versions
// you may have to alter your original code

#include "esp32-hal-timer.h"
#include "XT_DAC_Audio.h"
#include "HardwareSerial.h"
#include "soc/sens_reg.h"  // For dacWrite() patch, TEB Sep-16-2019



/*  Every variable that is used in the mainline code and in the onTImer interrupt code
 *	must be declared volatile.
 *	Also, variables used by onTimer must be regular variables and cannot be
 *	the CLASSES,because it is an interrupt routine and accessing variables
 *  belonging to objects was causing crashing in the ISR
 *  To avoid re-entrancy problems, the ISR onTimer() must change only the "play" index and the
 *	mainline code must change only the "fill" index.   Otherwise we can get corruption if
 *	onTimer() does a pull while FillBuffer() is putting data in.
 *	Each can _look_ at the other's variables, but both cannot _change_ the same variable.
 */

volatile int32_t NextPlayPos=0;							// position in buffer of next byte to play
volatile uint8_t *Buffer;								// The buffer to store the data that will be sent to the
volatile uint16_t BufferSize;							// Actual size if buffer used (default can be overridden in function call)
volatile uint8_t DacPin;                       			// pin to send DAC data to, presumably one of the DAC pins!
volatile uint8_t LastDacValue = 0x7f;				    // Last value sent to DAC, if next is same we don't write to DAC again
XT_PlayListItem_Class *FirstPlayListItem=nullptr;       // first play list item to play in linked list. TEB, Oct-02-2019.
XT_PlayListItem_Class *LastPlayListItem=nullptr;        // last play list item to play in linked list. TEB, Oct-02-2019.

XT_Instrument_Class DEFAULT_INSTRUMENT;

int BufferUsed = 0;										// NB : Not working yet. Amount of bytes sent out to play since last buffer fill, this value
														// can help you determine an optimum buffer size if you don't want to
														// stick with the default set in BUFFER_SIZE_DEFAULT


// interrupt stuff
hw_timer_t * timer = nullptr;                           // TEB, Oct-10-2019
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


// The FNOTE "defines" below contain actual frequencies for notes which range from a few Hz (around 30) to around 4000Hz
// But in essence there are only 89 different notes , we collect them into an array so that we can store a note as
// a number from 0 to 90, (0 being no 0Hz, or silence). In this way we can store musical notes in single bytes.
// To benefit from this the music data in your code should be greater than 90 bytes otherwise your technically using more
// memory than you would if you used the raw frequencies (which use 2 bytes per note). It is estimated in simple projects you
// would not gain but generally a simple project would have more spare memory anyway. For larger projects where memory could
// be more of an issue you will actually save memory, so hopefully a win!
uint16_t XT_Notes[90]={1,FNOTE_B0,FNOTE_C1,FNOTE_CS1,FNOTE_D1,FNOTE_DS1,FNOTE_E1,FNOTE_F1,FNOTE_FS1,FNOTE_G1,FNOTE_GS1,FNOTE_A1,FNOTE_AS1,FNOTE_B1,FNOTE_C2,FNOTE_CS2,FNOTE_D2,FNOTE_DS2,FNOTE_E2,FNOTE_F2,FNOTE_FS2,FNOTE_G2,FNOTE_GS2,FNOTE_A2,FNOTE_AS2,FNOTE_B2,FNOTE_C3,FNOTE_CS3,FNOTE_D3,FNOTE_DS3,FNOTE_E3,FNOTE_F3,FNOTE_FS3,FNOTE_G3,FNOTE_GS3,FNOTE_A3,FNOTE_AS3,FNOTE_B3,FNOTE_C4,FNOTE_CS4,FNOTE_D4,FNOTE_DS4,FNOTE_E4,FNOTE_F4,FNOTE_FS4,FNOTE_G4,FNOTE_GS4,FNOTE_A4,FNOTE_AS4,FNOTE_B4,FNOTE_C5,FNOTE_CS5,FNOTE_D5,FNOTE_DS5,FNOTE_E5,FNOTE_F5,FNOTE_FS5,FNOTE_G5,FNOTE_GS5,FNOTE_A5,FNOTE_AS5,FNOTE_B5,FNOTE_C6,FNOTE_CS6,FNOTE_D6,FNOTE_DS6,FNOTE_E6,FNOTE_F6,FNOTE_FS6,FNOTE_G6,FNOTE_GS6,FNOTE_A6,FNOTE_AS6,FNOTE_B6,FNOTE_C7,FNOTE_CS7,FNOTE_D7,FNOTE_DS7,FNOTE_E7,FNOTE_F7,FNOTE_FS7,FNOTE_G7,FNOTE_GS7,FNOTE_A7,FNOTE_AS7,FNOTE_B7,FNOTE_C8,FNOTE_CS8,FNOTE_D8,FNOTE_DS8
};


// Precalculate sine values in an 8 bit range (i.e. usually 0 to 360Deg), we want a new measure of a Degree that
// defines a circle as 256 parts. We'll call these 8Bit Degrees!
// As we're using an 8 bit value for the DAC 0-255 (256 parts) that means for us
// there are 256 'bits' to a complete circle not 360 (as in degrees)

int SineValues[256];       // an array to store our values for sine

void InitSineValues()
{
	float ConversionFactor=(2.0*3.142)/256.0;           // convert my 0-255 bits in a circle to radians
														// there are 2 x PI radians in a circle hence the 2*PI
														// Then divide by 256 to get the value in radians
														// for one of my 0-255 bits.
	float RadAngle;                           			// Angle in Radians, have to have this as computers love radians!
	// calculate sine values
	for(int MyAngle=0;MyAngle<256;MyAngle++) {
		RadAngle=MyAngle*ConversionFactor;              // 8 bit angle converted to radians
		SineValues[MyAngle]=(sin(RadAngle)*127)+128;  	// get the sine of this angle and 'shift' up so
														// there are no negative values in the data
														// as the DAC does not understand them and would
														// convert to positive values.
	}
}





uint8_t SetVolume(uint8_t Value,uint8_t Volume)
{
	// returns the sound byte value adjusted for the volume passed, 0 min, 127 max
	// a value of 127 will not boost the sound higher than the original, it will
	// mean the value returned is the original, and any other value below 127 will
	// be a proportional fraction of the original
	// remember that the mid point ( no sound, speaker at rest) is 7f not 0

	uint8_t AdjustedValue;
	if(Volume>127)							// Max is 127
		Volume=127;
	if(Value==0x7f)
		return Value;  // a speaker at mid point, cannot change the volume of that, it is effectively 0
	if(Value>0x7f)
	{
		// +ve part of wave above 0, runs 1 to 128
		// value will actually be in range 128 to 255,
		AdjustedValue=Value-127;
		// Now adjust the volume be the ratio passed in
		AdjustedValue=AdjustedValue*(float(Volume)/127.0f);
		// finally put back in range 1 to 128
		return AdjustedValue+127;
	}
	// if we get this far then wave below 0, range 127 to 0 (127 being mid of wave, 0 nearest bottom(trough))
	// just flip the value so it's in range 1 to 127 with 1 being the low point
	AdjustedValue=0x7f-Value;
	// Then add in volume adjustment
	AdjustedValue=AdjustedValue*(float(Volume)/127.0f);
	// adjust back to correct range before returning
	return 0x7f-AdjustedValue;

}





// The main interrupt routine called "BytesPerSec" times per second (132300 BPS).
// WARNING: Do not use Arduino function dacWrite() in this isr. It will cause
// crash/reboot if EEPROM.commit() is used during the audio playback.
void IRAM_ATTR onTimer()
{
	// Sound playing code, plays whatever's in the buffer continuously. Big change from previous versions
	if(LastDacValue!=Buffer[NextPlayPos])		// Send value to DAC only if changed since last value else no need
	{
		// value to DAC has changed, send to actual hardware, else we just leave setting as is as it's not changed
		LastDacValue=Buffer[NextPlayPos];

//		dacWrite(DacPin,LastDacValue);			// Don't use, will cause reboot during E2Prom writes. See emulation code below.

        // Start of dacWrite() emulation ...
        CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);  //Disable Tone
        if(DacPin==25) {
//          pinMode(DacPin, ANALOG);  // Don't enable, will cause reboot during E2Prom writes. Pre-Set by dacWrite() in XT_DAC_Audio_Class init.
            CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
            SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, LastDacValue, RTC_IO_PDAC1_DAC_S);
            SET_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC | RTC_IO_PDAC1_DAC_XPD_FORCE);
        }
        else if(DacPin==26) {
//          pinMode(DacPin, ANALOG);   // Don't enable, will cause reboot during E2Prom writes. Pre-Set by dacWrite() in XT_DAC_Audio_Class init.
            CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
            SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, LastDacValue, RTC_IO_PDAC2_DAC_S);
            SET_PERI_REG_MASK(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC | RTC_IO_PDAC2_DAC_XPD_FORCE);
        }
        // End of dacWrite() emulation.

	}
	Buffer[NextPlayPos]=0x7f;					// Set this buffer byte to silence, TEB Sep-23-2019.
	NextPlayPos++;								// Move play pos to next byte in buffer
	if(NextPlayPos >= BufferSize)				// If gone past end of buffer,
		NextPlayPos=0;							// set back to beginning
}


int XT_DAC_Audio_Class::BufferUsage()
{
   return BufferUsed;
}

void XT_DAC_Audio_Class::FillBuffer()
{
	// Fill the buffer, another routine that has changed almost completely since versions before 3.2
	// This is because of the requirement to mix sounds together so we can emit more than one at once
	// For each play item mix its next byte into the correct place in the buffer. Remember on a large
	// buffer the playing pos may be some distance from where the last sound is filling. Filling at
	// a position far ahead of the play pos would give an inaccurate and unknown mix effect
	// Note mixing always occurs even if just 1 sound being played as it will be effectively
	// mixed with the silence in the buffer.
	// To mix waves you add the waves together.... That is presuming they are "correct" waves
	// that have both positive and negative peaks and troughs. However these sounds produce
	// as 8bit unsigned value, i.e. everything above 0,
	// so therefore there is no negative, we need to adjust them back to having
	// correct positive and negative peaks and troughs first before doing the "mixing",
	// and then convert back to a wave with values 0 - 255.

	XT_PlayListItem_Class *PlayItem,*NextPlayItem;
	int16_t ByteToPlay,BufferByteToMixWith;							// Need 16 bit signed so we can have a range of
																	// -255 to +256
//	uint8_t NextByte,BufferByte;									// raw unsigned values of naext byte to plat and mix with
    uint8_t AudioVolume;                                            // Mod by TEB, Sep-15-2019
	uint8_t NextByte;
	uint32_t avail;													// # of bytes that we can put into the buffer.
	bool LogAvail=true;
	static uint16_t LastPlayPos=0;

	PlayItem=FirstPlayListItem;

	while(PlayItem!=nullptr)
	{
		if(PlayItem->NewSound)										// A new unplayed sound, set initial fill buffer position
		{
			PlayItem->NextFillPos=NextPlayPos+1;					// to one in front of actual playpos
			PlayItem->NewSound=false;								// No longer a new sound now.
			LogAvail=false;
		}

		avail = BufferSize- (PlayItem->NextFillPos - NextPlayPos); 	// Work out how much buffer we can fill
		if (avail >=BufferSize)										// This indicates the next fill pos was behind play pos,
			avail -= BufferSize;									//  bring into correct range
		if(LogAvail)												// calc how much of the play buffer was used since the
		{															// Last call to FillBuffer
			BufferUsed=NextPlayPos-LastPlayPos;
			if(BufferUsed<0)
				BufferUsed=BufferUsed+BufferSize;
		}
		while((PlayItem->Playing) && (avail > 0))					// while space in buffer and item still active.  TEB, Sep-29-2019.
		{
            avail--;
			NextByte=PlayItem->NextByte();							// Get next byte from this sound to play
			if(PlayItem->Filter!=0)
				NextByte=PlayItem->Filter->FilterWave(NextByte);	// Adjust the byte by any set filter, a Work in Progress

			ByteToPlay=(NextByte-127);  							// -127 to make it's a wave that swings above and below 0
			BufferByteToMixWith=Buffer[PlayItem->NextFillPos]-127;  // -127 to make it's a wave that swings above and below 0

			ByteToPlay=BufferByteToMixWith+ByteToPlay;				// Mix them together and store back in ByteToPlay
			if(ByteToPlay>128)
				ByteToPlay=128;										// Sound has topped out, adjust to max top out
			if(ByteToPlay<-127)
				ByteToPlay=-127;									// Sound has bottomed out, adjust to min bottom out

            AudioVolume = DacVolume;                                // Audio Volume patch by TEB, Sep-16-2019. 0=Off, 100=Full Volume.
            if(AudioVolume > 100) AudioVolume = 100;                // Constrain volume to 0-100%
            ByteToPlay = ByteToPlay * ((float)(AudioVolume)/100.0f);// Apply volume setting to dac data.

			Buffer[PlayItem->NextFillPos]=uint8_t(ByteToPlay+127);	// Store back in buffer adjusting so back in 0-255 range

			// Move the fill position for this sound item to next pos
			PlayItem->NextFillPos++;								// move to next buffer position
			if(PlayItem->NextFillPos>=BufferSize)					// Set to start of buffer if end reached
			   PlayItem->NextFillPos=0;

		}
		NextPlayItem=PlayItem->NextItem;							// move to next play item
		if(PlayItem->Playing==false)								// If this play item completed
		{
			if(PlayItem->RepeatForever)
			{
				PlayItem->Init();						// initialise for playing again
				PlayItem->Playing=true;
			}
			else
			{
				if(PlayItem->RepeatIdx>0)				// Repeat sound X amount of times
				{
					PlayItem->RepeatIdx--;
					PlayItem->Init();						// initialise for playing again
					PlayItem->Playing=true;
				}
				else
					RemoveFromPlayList(PlayItem);		// no repeats, remove from play list
			}
		}
		PlayItem=NextPlayItem;										// Set to next item
	}
}


XT_PlayListItem_Class::XT_PlayListItem_Class()
{
	RepeatForever=false;
	Repeat=0;
	RepeatIdx=0;
}

// Despite being marked virtual and being overridden in desendents without these I get vtable error
// issues which I really don't understand, sl I put these in even though never called and all is well
uint8_t XT_PlayListItem_Class::NextByte()
{
	return 0;
}
void XT_PlayListItem_Class::Init()
{
}


XT_DAC_Audio_Class::XT_DAC_Audio_Class(uint8_t TheDacPin, uint8_t TimerNo):XT_DAC_Audio_Class(TheDacPin,TimerNo,BUFFER_SIZE_DEFAULT)
{
	// calls other constructor as shown in function definition
}


XT_DAC_Audio_Class::XT_DAC_Audio_Class(uint8_t TheDacPin, uint8_t TimerNo,uint16_t PassedBufferSize)
{
	// Using a prescaler of 80 gives a counting frequency of 1,000,000 (1MHz) and using
	// and calling the function every 20 counts of the frequency (the 20 below) means
	// that we will call our onTimer function 50,000 times a second

	// reserve some memory for the buffer
	Buffer=new uint8_t[PassedBufferSize];
	BufferSize=PassedBufferSize;					// Buffer Size is global var used in other routines that need to know size
	for(uint32_t i=0;i<PassedBufferSize;i++)		// Set all bytes in buffer to 0x7f (DAC silence), TEB Sep-23-2019
		Buffer[i]=0x7f;
	FirstPlayListItem=nullptr;                      // TEB, Oct-10-2019
	DacPin=TheDacPin;								// set dac pin to use
	LastDacValue=0x7F;								// set to mid point
	dacWrite(DacPin,LastDacValue);					// Set speaker to mid point, stops click at start of first sound
	InitSineValues();								// create our table of sine values for our special circular
													// angles of 0 - 255 "DAC Degrees". Speeds things up to if
													// you pre-calculate
	// Set up interrupt routine
	timer = timerBegin(TimerNo, 80, true);          // use timer TimerNo, pre-scaler is 80 (divide by 8000), count up
	timerAttachInterrupt(timer, &onTimer, true);    // P3= edge triggered
	timerAlarmWrite(timer, 20, true);               // will trigger 250,000 times per second,
	timerAlarmEnable(timer);                        // enable
	delay(1);                             			// Allow system to settle, otherwise garbage can play for first second

}


void XT_DAC_Audio_Class::PrintPlayList()
{
	// For debugging purposes, lists addresses of all items in playlist
    Serial.println("Current play list");
	XT_PlayListItem_Class *PlayItem = nullptr;  // TEB, Oct-10-2019
	PlayItem=FirstPlayListItem;
	while(PlayItem!=nullptr)                    // TEB, Oct-10-2019
	{
		Serial.print(PlayItem->Name);
		Serial.print(" @ ");
		Serial.print((unsigned long)PlayItem, HEX);
		PlayItem=PlayItem->NextItem;
		Serial.print("->");
	}
	Serial.println();

}


void XT_DAC_Audio_Class::RemoveFromPlayList(XT_PlayListItem_Class *ItemToRemove)
{
	// removes a play item from the play list
	if(ItemToRemove->PreviousItem!=nullptr)   // TEB, Oct-10-2019
		ItemToRemove->PreviousItem->NextItem=ItemToRemove->NextItem;
	else
		FirstPlayListItem=ItemToRemove->NextItem;
	if(ItemToRemove->NextItem!=nullptr)       // TEB, Oct-10-2019
		ItemToRemove->NextItem->PreviousItem=ItemToRemove->PreviousItem;
	else
		LastPlayListItem=ItemToRemove->PreviousItem;

	ItemToRemove->PreviousItem=nullptr;       // TEB, Oct-10-2019
	ItemToRemove->NextItem=nullptr;           // TEB, Oct-10-2019

}


void XT_DAC_Audio_Class::Play(XT_PlayListItem_Class *Sound)
{
	Play(Sound,true);				// Sounds mix by default
}


void XT_DAC_Audio_Class::Play(XT_PlayListItem_Class *Sound,bool Mix)
{

	// check if this sound is already playing, if so it is removed first and will be re-played
	// This limitation will be removed in later version so that multiple versions of the same
	// sound can be played at once. Trying to do that now will corrupt the list of items to play

	if(AlreadyPlaying(Sound))
		RemoveFromPlayList(Sound);
	if(Mix==false)  							// stop all currently playing sounds and just have this one
		StopAllSounds();

	Sound->NewSound=true;						// Flags to fill buffer routine that this is brand new sound
												// with nothing yet put into buffer for playing
	Sound->RepeatIdx=Sound->Repeat;				// Initialise any repeats

	// set up this sound to play, different types of sound may initialise differently
	Sound->Init();

	// add to list of currently playing items
	// create a new play list entry
	if(FirstPlayListItem==nullptr) // no items to play in list yet. TEB, Oct-10-2019
	{
		FirstPlayListItem=Sound;
		LastPlayListItem=Sound;
	}
	else{
		// add to end of list
		LastPlayListItem->NextItem=Sound;
		Sound->PreviousItem=LastPlayListItem;
		LastPlayListItem=Sound;

	}
	Sound->Playing=true;					// Will start it playing
}


bool XT_DAC_Audio_Class::AlreadyPlaying(XT_PlayListItem_Class *Item)
{
	// returns true if sound already in list of items to play else false
	XT_PlayListItem_Class* PlayItem;
	PlayItem=FirstPlayListItem;
	while(PlayItem!=nullptr)            // TEB, Oct-10-2019
	{
		if(PlayItem==Item)
			return true;
		PlayItem=PlayItem->NextItem;
	}
	return false;
}


void XT_DAC_Audio_Class::StopAllSounds()
{
	// stop all sounds and clear the play list

	XT_PlayListItem_Class* PlayItem;
	PlayItem=FirstPlayListItem;
	while(PlayItem!=nullptr)       // TEB, Oct-10-2019
	{
		PlayItem->Playing=false;
		RemoveFromPlayList(PlayItem);
		PlayItem=FirstPlayListItem;
	}
	FirstPlayListItem=nullptr;     // TEB, Oct-10-2019
}




// Wav Class functions

#define DATA_CHUNK_ID 0x61746164
#define FMT_CHUNK_ID 0x20746d66
// Convert 4 byte little-endian to a long.
#define longword(bfr, ofs) (bfr[ofs+3] << 24 | bfr[ofs+2] << 16 |bfr[ofs+1] << 8 |bfr[ofs+0])

XT_Wav_Class::XT_Wav_Class(const unsigned char *WavData)
{
   // create a new wav class object
   unsigned long ofs, siz;

   /* Process the chunks.  "fmt " is format, "data" is the samples, ignore all else. */
   ofs = 12;
   siz = longword(WavData, 4);
   SampleRate = DataStart = 0;
   while (ofs < siz) {
      if (longword(WavData, ofs) == DATA_CHUNK_ID) {
	 DataSize = longword(WavData, ofs+4);
	 DataIdx = DataStart = ofs +8;
      }
      if (longword(WavData, ofs) == FMT_CHUNK_ID) {
	 SampleRate = longword(WavData, ofs+12);
      }
      ofs += longword(WavData, ofs+4) + 8;
   }
   IncreaseBy=float(SampleRate)/BytesPerSec;
   PlayingTime = (1000 * DataSize) / (uint32_t)(SampleRate);
   Data=WavData;
   Speed=1.0;
}


void XT_Wav_Class::Init()
{
	LastIntCount=0;
	DataIdx=DataStart;
	Count=0;
	SpeedUpCount=0;
	TimeElapsed = 0;
	TimeLeft = PlayingTime;
}


uint8_t XT_Wav_Class::NextByte()
{
	// Returns the next byte to be played, note that this routine will return values suitable to
	// be played back at 50,000Hz. Even if this sample is at a lesser rate than that it will be
	// padded out as required so that it will appear to have a 50Khz sample rate


	uint32_t IntPartOfCount;
	uint8_t ReturnValue;
	float ActualIncreaseBy;

	// increase the counter, if it goes to a new integer digit then write to DAC

	ActualIncreaseBy=IncreaseBy;      // default if not playing slower than normal
	if(Speed<=1.0)	// manipulate IncreaseBy
		ActualIncreaseBy=IncreaseBy*Speed;
	Count+=ActualIncreaseBy;
	IntPartOfCount=floor(Count);
//	ReturnValue=Data[DataIdx];				// by default we return previous value;
    ReturnValue = SetVolume(Data[DataIdx], Volume);  // TEB, Oct-22-2019

	if(IntPartOfCount>LastIntCount)
	{
		if(Speed>1.0)
		{
			// for speeding up we need to basically go through the data quicker as upping the frequency
			// that this routine is called could put too much strain on the CPU.
			// First we subtract 1 from IncreaseBy as code below will handling the basic increment through
			// the data.

			// we now get the integer and decimal parts of this number and move the DataIdx on by "int" amount first
			double IntPartAsFloat,DecimalPart,TempSpeed;
			TempSpeed=Speed-1.0;
			DecimalPart=modf(TempSpeed,&IntPartAsFloat);
			DataIdx+=int(IntPartAsFloat);						// always increase by the integer part
			SpeedUpCount+=DecimalPart;
			// If SpeedUpCount >1 then add this extra "1" to the DataIdx too and subtract 1 from SpeedUpCount
			// This allows us "apparently" increment the DataIdx by a decimal amount

			if(SpeedUpCount>1)
			{
				DataIdx++;				// move another pos into data
				SpeedUpCount--;			// Take it off SpeedUpCount
			}
		}
		// gone to a new integer of count, we need to send a new value to the DAC next time
		// update the DataIDx counter
		LastIntCount=IntPartOfCount;
		DataIdx++;
		TimeElapsed = 1000 * DataIdx / SampleRate;
		TimeLeft = PlayingTime - TimeElapsed;
		if(DataIdx>=DataSize)  				// end of data, flag end
		{
			Count=0;						// reset frequency counter
			DataIdx=DataStart;				// reset data pointer back to beginning of WAV data
			Playing=false;  				// mark as completed
			TimeLeft = 0;
		}
	}

	return ReturnValue;

}







// Instrument class routines

XT_Instrument_Class::XT_Instrument_Class():XT_Instrument_Class(INSTRUMENT_PIANO,127)
{
	  // See main constructor routine for description of passed parameters
	  // and defaults
}


XT_Instrument_Class::XT_Instrument_Class(int16_t InstrumentID):XT_Instrument_Class(InstrumentID,127)
{
	  // See main constructor routine for description of passed parameters
	  // and defaults
}


XT_Instrument_Class::XT_Instrument_Class(int16_t InstrumentID, uint8_t Volume)
{
	// Volume : Volume of sound 0- 127 (max)
	this->Note=abs(NOTE_C4);					// default note
	this->SoundDuration=1000;					// default note length, ignored if using envelopes
	this->Duration=1000;						// default length of entire play action (i.e. after any decay) ignored if using envelopes
	this->Volume=Volume;
	SetInstrument(InstrumentID);				// The default

}


void XT_Instrument_Class::SetNote(int8_t Note)
{
	this->Note=abs(Note);
}


void XT_Instrument_Class::SetInstrument(uint16_t Instrument)
{
	// To create your new "permanent" instrument to the code, add a line here that calls its
	// set up routine and add this as a private function to the header file for the
	// Instrument class. Also add a suitable # define to the Instruments.h header file
	// in the instruments section.
	// Any envelopes and waveforms will have their memory cleared here, no need to worry!

	delete(FirstEnvelope);				// remove old envelope definition
	FirstEnvelope=nullptr;              // TEB, Oct-10-2019

	switch (Instrument)
	{
		case(INSTRUMENT_NONE) 			: SetDefaultInstrument();break;
		case(INSTRUMENT_PIANO)			: SetPianoInstrument();break;
		case(INSTRUMENT_HARPSICHORD)	: SetHarpsichordInstrument();break;
		case(INSTRUMENT_ORGAN)			: SetOrganInstrument();break;
		case(INSTRUMENT_SAXOPHONE)		: SetSaxophoneInstrument();break;

		default: // compilation error, default to just square wave
			SetDefaultInstrument();break;
	}
}


void XT_Instrument_Class::SetDefaultInstrument()
{
	SetWaveForm(WAVE_SQUARE);
}


void XT_Instrument_Class::SetPianoInstrument()
{
	SetWaveForm(WAVE_TRIANGLE);
	FirstEnvelope=new XT_Envelope_Class();
	FirstEnvelope->AddPart(25,127);
	FirstEnvelope->AddPart(20,20);
	FirstEnvelope->AddPart(15,75);
	FirstEnvelope->AddPart(10,15);
	FirstEnvelope->AddPart(5,50);
	FirstEnvelope->AddPart(300,0);
}

void XT_Instrument_Class::SetHarpsichordInstrument()
{
	SetWaveForm(WAVE_SAWTOOTH);
	FirstEnvelope=new XT_Envelope_Class();
	FirstEnvelope->AddPart(15,127);
	FirstEnvelope->AddPart(80,100);
	FirstEnvelope->AddPart(300,0);
}


void XT_Instrument_Class::SetOrganInstrument()
{
	SetWaveForm(WAVE_SINE);
	FirstEnvelope=new XT_Envelope_Class();
	FirstEnvelope->AddPart(15,127);
	FirstEnvelope->AddPart(3000,0);	// An organ maintains until key released
}



void XT_Instrument_Class::SetSaxophoneInstrument()
{
	SetWaveForm(WAVE_SQUARE);
	FirstEnvelope=new XT_Envelope_Class();
	FirstEnvelope->AddPart(15,127);
	FirstEnvelope->AddPart(3000,0);	// An organ maintains until key released
}


void XT_Instrument_Class::SetWaveForm(uint8_t WaveFormType)
{
	// Sets the wave form for this instrument

	delete (WaveForm);										// free any previous memory for a previous waveform
	switch (WaveFormType)
	{
		case WAVE_SQUARE : WaveForm=new XT_SquareWave_Class();break;
		case WAVE_TRIANGLE : WaveForm=new XT_TriangleWave_Class();break;
		case WAVE_SAWTOOTH : WaveForm=new XT_SawToothWave_Class();break;
		case WAVE_SINE : WaveForm=new XT_SineWave_Class();break;

		default: // compilation error, default to square
			WaveForm=new XT_SquareWave_Class();break;
	}
}

void XT_Instrument_Class::SetFrequency(uint16_t Freq)
{
	// only if not using note property
	Note=-1;
	WaveForm->Frequency=Freq;
	WaveForm->Init(-1);
}


void XT_Instrument_Class::Init()
{
	CurrentByte=0;
	WaveForm->Init(Note);
	// Note : These next two lines calculating the counters for the note duration are not used
	// if there are envelopes attached to this instrument, the timings of the envelopes taken
	// priority
	SoundDurationCounter=SoundDuration*50;
	DurationCounter=Duration*50;          // converts to how many samples to return before stopping
	if(FirstEnvelope!=nullptr)            // TEB, Oct-10-2019
	{
		CurrentEnvelope=FirstEnvelope;
		CurrentEnvelope->Init();

	}
}

uint8_t XT_Instrument_Class::NextByte()
{
	uint8_t ByteToPlay;

	// If no envelope then use normal duration settings
	if(CurrentEnvelope==nullptr)           // TEB, Oct-10-2019
	{
		if(DurationCounter==0)									// If completed mark as so and return no sound
		{
			Playing=false;
			return 0;
		}
		DurationCounter--;

		if(SoundDurationCounter==0)								// If sound completed return no sound
			return 0;
		SoundDurationCounter--;
	}

	if(WaveForm->Frequency==0)										// If no frequency then no sound
		return 0;

	// This next bit handles basic wave form if a sound is being produced

	ByteToPlay=WaveForm->NextByte();

	// Next add in envelope control if there is one
	if(CurrentEnvelope!=nullptr)                               // TEB, Oct-10-2019
	{
		ByteToPlay=CurrentEnvelope->NextByte(ByteToPlay);
		if(CurrentEnvelope->EnvelopeCompleted)
		{
			// check if another envelope in chain
			if(CurrentEnvelope->NextEnvelope!=nullptr)         // TEB, Oct-10-2019
			{
				CurrentEnvelope=CurrentEnvelope->NextEnvelope;
				CurrentEnvelope->Init();
			}
			else
			{
				Playing=false;		// All envelopes played
			}
		}
	}

	// adjust for current volume of this sound
	return SetVolume(ByteToPlay,Volume);
}


XT_Envelope_Class *XT_Instrument_Class::AddEnvelope()
{
	// Adds am envelope to this instrument, you will populate the envelope with it's parts
	// using the Envelope class AddPart function

	XT_Envelope_Class* Env=new XT_Envelope_Class();
	XT_Envelope_Class* ThisEnv,PreviousEnv;

	if(FirstEnvelope==nullptr)                // TEB, Oct-10-2019
	{
		// no envelopes at all yet, set to first and return
		FirstEnvelope=Env;
		return Env;
	}
	// find last envelope and add another to the chain
	ThisEnv=FirstEnvelope;
	while(ThisEnv->NextEnvelope!=nullptr)     // TEB, Oct-10-2019
		ThisEnv=ThisEnv->NextEnvelope;
	ThisEnv->NextEnvelope=Env;
	return Env;
}


void XT_Instrument_Class::SetDuration(uint32_t Length)			// in millis
{
	// If you are using a default instrument then this routine need not be set
	// will set duration of sound and duration that it plays to same value, i.e. when
	// sound physical sound ends the note is deemed finished
	SoundDuration=Length;
	Duration=Length;
}






//Music score class

XT_MusicScore_Class::XT_MusicScore_Class(int8_t* Score):XT_MusicScore_Class(Score,TEMPO_ALLEGRO,&DEFAULT_INSTRUMENT)
{
	// set tempo and instrument to default, number of plays to 1 (once)
}


XT_MusicScore_Class::XT_MusicScore_Class(int8_t* Score,uint16_t Tempo):XT_MusicScore_Class(Score,Tempo,&DEFAULT_INSTRUMENT)
{
	// set instrument to default and plays to 1
}


XT_MusicScore_Class::XT_MusicScore_Class(int8_t* Score,uint16_t Tempo,XT_Instrument_Class* Instrument)
{
	// Create music score
	this->Score=Score;
	this->Tempo=Tempo;
	this->Instrument=Instrument;
}


XT_MusicScore_Class::XT_MusicScore_Class(int8_t* Score,uint16_t Tempo,uint16_t InstrumentID)
{
	// Create music score
	this->Score=Score;
	this->Tempo=Tempo;
	this->Instrument=&DEFAULT_INSTRUMENT;
	Instrument->SetInstrument(InstrumentID);
}


void XT_MusicScore_Class::Init()
{
	// Called before as score starts playing

	Instrument->Init(); // Initialise instrument

	// convert the tempo in BPM into a value that counts down in 50,000's of a second as this is the sample
	// rate sent to the DAC
	ChangeNoteEvery=(60/float(Tempo))*BytesPerSec;
	ChangeNoteCounter=0;					// ensures starts by playing first note with no delay
	ScoreIdx=0;								// set position to first no
}


uint8_t XT_MusicScore_Class::NextByte()
{
	// returns next byte for the DAC

	// are we ready to play another note?
	if(ChangeNoteCounter==0)  {
		// Yes, new note
		if(Score[ScoreIdx]==SCORE_END)  // end of data
		{
			Playing=false;
			return 0;    	// return silence
		}

		Instrument->Note=abs(Score[ScoreIdx]);			// convert the negative value to positive index.
		ScoreIdx++;										// move to next note

		// set length of play for instrument
		// Check next data value to see if it is a beat value
		if(Score[ScoreIdx]>0) 							// positive value, therefore not default beat length
		{
			// Set the duration, beat value of 1=0.25 beat, 2 0.5 beat etc. So just divide by 4
			// to get the real beat length,
			Instrument->Duration=(ChangeNoteEvery*(float(Score[ScoreIdx])/4));
			// Then times by 0.8 to allow for natural movement
			// of players finger on the instrument.
			Instrument->SoundDuration=Instrument->Duration*0.8;
			ChangeNoteCounter=Instrument->Duration;      	// set back to start of count ready for next note
			ScoreIdx++;										// point to next note, ready for next time
		}
		else
		{
			// default single beat values
			Instrument->Duration=ChangeNoteEvery;
			Instrument->SoundDuration=Instrument->Duration*0.8;  	// By default a note plays for 80% of tempo
																// this allows for the natural movement of the
																// player performing the next note
			ChangeNoteCounter=ChangeNoteEvery;              	// set back to start of count ready for next note
		}
		// Note that we do not call DacAudio.Play() here as it's already been done for this music score
		// and it's this music score that effectively controls the note playing, however we still need
		// to initialise the instrument for each new note played
		Instrument->Init();
	}
	ChangeNoteCounter--;
	// return the next byte for this instrument
	return Instrument->NextByte();
}




void XT_MusicScore_Class::SetInstrument(uint16_t InstrumentID)
{
	Instrument->SetInstrument(InstrumentID);
}





// Envelope class, see www.xtronical.com for description of how to use envelopes with
// the instrument class

XT_Envelope_Class::XT_Envelope_Class()
{
	// nothing yet
}

XT_Envelope_Class::~XT_Envelope_Class()
{
	// delete any further linked envelopes and associated envelope parts for all envelopes found
	// Note if you delete from an envelope that it is not the start envelope then the previous
	// envelopes will still exist and in fact the last one before the one you delete will still
	// point to this one. If for some reason you wish to delete a none first envelope then you
	// must tidy up the last of the previous envelopes yourself.

	XT_EnvelopePart_Class* Part,*NextPart;

	if(NextEnvelope!=0)
		delete(NextEnvelope);				// if another envelope in the chain delete this also

	// delete the parts for this envelope
	Part=FirstPart;
	while(Part!=0)
	{
		NextPart=Part->NextPart;
		delete(Part);
		Part=NextPart;
	}
}


void XT_Envelope_Class::Init()
{
	// Reset envelope ready for next note to play
	CurrentEnvelopePart=0;
	EnvelopeCompleted=false;
	RepeatCounter=Repeats;
	DurationCounter=1;
}

uint8_t XT_Envelope_Class::NextByte(uint8_t ByteToPlay)
{
	// are their any parts, if not do nothing
	if(FirstPart!=0)
	{
		// OK we have some envelope parts, manipulate the wave form volume according to the
		// current envelope part in use for this instrument
		if(EnvelopeCompleted==false)
		{
			if(CurrentEnvelopePart==0)  				// start of envelope
			{
				CurrentEnvelopePart=FirstPart;
				InitEnvelopePart(CurrentEnvelopePart,CurrentVolume);
			}
			if(CurrentEnvelopePart->Completed)
			{
				CurrentEnvelopePart=CurrentEnvelopePart->NextPart;
				if(CurrentEnvelopePart==0)  // no more parts, do we need to repeat the envelope
				{
					if(RepeatCounter>0)
						RepeatCounter--;
					else
						EnvelopeCompleted=true;
				}
				else
					InitEnvelopePart(CurrentEnvelopePart,CurrentVolume);
			}
			if(CurrentEnvelopePart!=0)
			{
				CurrentVolume+=VolumeIncrement;
				ByteToPlay=SetVolume(ByteToPlay,int(CurrentVolume));
				DurationCounter--;
				if(DurationCounter==0)
					CurrentEnvelopePart->Completed=true;
			}
		}
	}
	return ByteToPlay;
}




void XT_Envelope_Class::InitEnvelopePart(XT_EnvelopePart_Class* EPart,uint8_t LastVolume)
{
	// initialises the properties in preparation to starting to use this envelope object
	// in note production

	if(EPart->StartVolume!=-1)					// do we have a start vol
		LastVolume=EPart->StartVolume;			// yes , set to this
	DurationCounter=EPart->RawDuration;
	// calculate how much the volume should increment per sample, this depends on what
	// the last volume reached was for the last envelope part, initially for the first
	// envelope part this would be 0. Volume is in the range 0-127
	VolumeIncrement=float((EPart->TargetVolume-LastVolume))/float(DurationCounter);
	CurrentVolume=LastVolume;
	EPart->Completed=false;
}



XT_EnvelopePart_Class* XT_Envelope_Class::AddPart(uint16_t Duration,uint16_t StartVolume,uint16_t TargetVolume)
{
	XT_EnvelopePart_Class* EPart;
	EPart=AddPart(Duration,TargetVolume);
	EPart->StartVolume=StartVolume;
	return EPart;
}



XT_EnvelopePart_Class* XT_Envelope_Class::AddPart(uint16_t Duration,uint16_t TargetVolume)
{
	// creates and adds an envelope part to this current envelope
	XT_EnvelopePart_Class* EPart=new XT_EnvelopePart_Class();

	EPart->SetDuration(Duration);
	EPart->TargetVolume=TargetVolume;
	if(FirstPart==0)											// First in list of envelope parts
	{
		FirstPart=EPart;
		LastPart=EPart;
	}
	else														// Add to end of list of envelope parts
	{
		LastPart->NextPart=EPart;
		LastPart=EPart;
	}
	return EPart;
}





// Envelope part class
void XT_EnvelopePart_Class::SetDuration(uint16_t Duration)
{
	this->Duration=Duration;
	this->RawDuration=50*Duration;
}

uint16_t XT_EnvelopePart_Class::GetDuration()
{
	return Duration;
}





// Wave type classes

// Square wave

uint8_t XT_SquareWave_Class::NextByte()
{
	// returns the next byte for this frequency of a square wave
	Counter--;
	if(Counter<0)
	{
		Counter+=CounterStartValue;				// as this can be a decimal, any amount extra below zero
												// is taken away from next starting value, in this way
												// for higher frequencies we do not lose as much
												// accuracy over a few waves, they average out to be
												// about right "on average"
		CurrentByte^=255;
		return CurrentByte;
	}
	else
		return CurrentByte;
}

void XT_SquareWave_Class::Init(int8_t Note)
{
	if(Note!=-1)								// use the note not a raw frequency
		Frequency=XT_Notes[Note];  				// if -1 then use the raw frequency
	if(Frequency>25000)
		Frequency=25000;
	if(Frequency!=0)							// avoid divide by 0, a freq of 0 means no sound
	{
		CounterStartValue=(25000/float(Frequency));
		Counter=CounterStartValue;
	}
	else
		Counter=0;
	ChangeAmplitudeBy=255;   					// amount to add on to change wave every time waveform needs changing
												// this value effectivly swaps the value from 0 to 255 and then 255 to 0
}





// Triangle wave functions
// Note that their

uint8_t XT_TriangleWave_Class::NextByte()
{
	// returns the next byte for this frequency of a square wave
	// called at the raw sample rate
	NextAmplitude+=ChangeAmplitudeBy;
	if(NextAmplitude>255)
	{	// top of a peak, reverse direction
		ChangeAmplitudeBy=-ChangeAmplitudeBy;    	// reverse direction
		NextAmplitude=255;
	}
	else
	{
		if(NextAmplitude<0)
		{	// bottom of a trough, reverse direction
			ChangeAmplitudeBy=-ChangeAmplitudeBy;    	// reverse direction
			NextAmplitude=0;
		}
	}
	return int(NextAmplitude);
}

void XT_TriangleWave_Class::Init(int8_t Note)
{
	NextAmplitude=127;
	if(Note!=-1)								// use the note not a raw frequency
		Frequency=XT_Notes[Note];  				// if -1 then use the raw frequency

	if(Frequency>25000)
		Frequency=25000;
	if(Frequency!=0)							// avoid divide by 0, a freq of 0 means no sound
	{
		ChangeAmplitudeBy=256/(25000/float(Frequency));// has to be 25K as has to ramp up and down to return to peak

	}
}




uint8_t XT_SawToothWave_Class::NextByte()
{
	// returns the next byte for this frequency of a square wave

	NextAmplitude+=ChangeAmplitudeBy;
	if(NextAmplitude>255)  // top of a peak, right down to bottom again
		NextAmplitude=0;
	return int(NextAmplitude);
}

void XT_SawToothWave_Class::Init(int8_t Note)
{
	NextAmplitude=0;
	if(Note!=-1)								// use the note not a raw frequency
		Frequency=XT_Notes[Note];  				// if -1 then use the raw frequency

	if(Frequency>25000)
		Frequency=25000;
	if(Frequency!=0)							// avoid divide by 0, a freq of 0 means no sound
		ChangeAmplitudeBy=256/(BytesPerSec/float(Frequency)); // determines amplitude change per sample
}






///////////////////////////////////////////////
uint8_t XT_SineWave_Class::NextByte()
{
	// returns the next byte for this frequency of a sine wave
	CurrentAngle+=AngleIncrement;
	if(CurrentAngle>255)
		CurrentAngle=0;							// Set to start
	return SineValues[int(CurrentAngle)];
}

void XT_SineWave_Class::Init(int8_t Note)
{
	CurrentAngle=0;
	if(Note!=-1)								// use the note not a raw frequency
		Frequency=XT_Notes[Note];  				// if -1 then use the raw frequency

	if(Frequency>25000)
		Frequency=25000;
	if(Frequency!=0)							// avoid divide by 0, a freq of 0 means no sound
		AngleIncrement=256/(BytesPerSec/float(Frequency));   // determines frequency
}



//////////////////////////////////////////////
void XT_Sequence_Class::Init()
{
	if(FirstItem!=nullptr)          // TEB, Oct-10-2019.
		CurrentItem=FirstItem;
	else
	{
		CurrentItem=nullptr;        // TEB, Oct-10-2019.
		return;
	}
	while(CurrentItem!=nullptr)     // TEB, Oct-10-2019.
	{
		CurrentItem->PlayItem->RepeatIdx=CurrentItem->PlayItem->Repeat;				// Initialise any repeats
		CurrentItem->PlayItem->Init();
		CurrentItem=CurrentItem->NextItem;
	}
	CurrentItem=FirstItem; 					// set back to start
	CurrentItem->PlayItem->Playing=true;	// mark as playing, init is called just prior to
											// first play

}


void XT_Sequence_Class::AddPlayItem(XT_PlayListItem_Class *PlayItem)
{
	// we copy the item so there are no conflicts with the item being played by the main Play routine
	// which also uses the linked lists and other properties of the play object etc. This will only
	// be a problem when we implement mixing of sounds.


	XT_SequenceItem_Class *SequenceItem;
	// create a new sequence item
	SequenceItem=new XT_SequenceItem_Class();
	SequenceItem->PlayItem=PlayItem;
	if(FirstItem==0) // no items to play in list yet
	{
		FirstItem=SequenceItem;
		LastItem=SequenceItem;
	}
	else
	{
		// add to end of list
		LastItem->NextItem=SequenceItem;
		LastItem=SequenceItem;
	}
}


void XT_Sequence_Class::RemoveAllPlayItems()
{
	// Remove all items in linked list and free associated memory

	XT_SequenceItem_Class *ThisItem,*NextItem;
	// Ensure not currently playing
	Playing=false;
	ThisItem=FirstItem;
	while(ThisItem!=nullptr)  // TEB, Oct-10-2019.
	{
		NextItem=ThisItem->NextItem;			// Get Next Item in list before removing this one
		delete(ThisItem);
		ThisItem=NextItem;
	}
	FirstItem=nullptr;   // TEB, Oct-10-2019.
	LastItem= nullptr;   // TEB, Oct-10-2019.
}


uint8_t XT_Sequence_Class::NextByte()
{
	if(CurrentItem == nullptr )     // TEB, Oct-10-2019.
		return 0;
	if(CurrentItem->PlayItem->Playing)
	//	return CurrentItem->PlayItem->NextByte();
        return SetVolume(CurrentItem->PlayItem->NextByte(), Volume);  // TEB, Oct-22-2019

	// If we get this far then the current item has stopped playing, time to play next
	// item if this one does not repeat, note RepeatForever is NOT ignored, as although
	// it would make no sense in most parts of a sequence as it would prevent any
	// more from playing, it does make sense if used on the last item where you may
	// want the last item to repeat forever, so it's up to the coder to not use
	// forever on anything but the last item

	// Check if set to play forever
	if(CurrentItem->PlayItem->RepeatForever)
	{
		CurrentItem->PlayItem->Init();
		CurrentItem->PlayItem->Playing=true;
	//	return CurrentItem->PlayItem->NextByte();
        return SetVolume(CurrentItem->PlayItem->NextByte(), Volume);  // TEB, Oct-22-2019
	}

	// check if any repeats
	if(CurrentItem->PlayItem->RepeatIdx>0)
	{
		CurrentItem->PlayItem->RepeatIdx--;			// decrement number left
		// repeat this item
		CurrentItem->PlayItem->Init();
		CurrentItem->PlayItem->Playing=true;
	//	return CurrentItem->PlayItem->NextByte();
        return SetVolume(CurrentItem->PlayItem->NextByte(), Volume);  // TEB, Oct-22-2019
	}
	// If got this far then current item stopped playing and no repeats, move to next item
	CurrentItem->PlayItem->Init();		// Init in case needed again. Steve, Oct-13-2019.
	CurrentItem=CurrentItem->NextItem;
	if(CurrentItem!=nullptr)
	{
		CurrentItem->PlayItem->Playing=true;
	//	return CurrentItem->PlayItem->NextByte();
        return SetVolume(CurrentItem->PlayItem->NextByte(), Volume);  // TEB, Oct-22-2019
	}
	else {
		Playing=false;					    // completed
        if(ClearAfterPlay && !RepeatForever)// Clear the Sequence list, Start Anew. Steve, Oct-21-2019.
			RemoveAllPlayItems();
    }

    return 0;  // Otherwise return "Silence" as default value. Added by TEB, Sep-15-2019.

}





// Filter class routines



XT_FilterNoise_Class::XT_FilterNoise_Class(int8_t AmountOfNoise)
{
	this->Min=-AmountOfNoise;
	this->Max=AmountOfNoise;
}


XT_FilterNoise_Class::XT_FilterNoise_Class(int8_t Min,int8_t Max)
{
	this->Min=Min;
	this->Max=Max;
}


uint8_t XT_FilterNoise_Class::FilterWave(uint8_t TheByte)
{
	// returns TheByte adjusted either up or down by a random amount, determined
	// by the min and max properties
	// using random gives the effect of noise

	int8_t TheRand=(rand()%((Max+1)-Min))+Min;
	int16_t TheResult=int16_t(TheByte)+int16_t(TheRand);

	if(TheResult>255)
		TheResult=255;
	else
		if(TheResult<0)
			TheResult=0;
	return uint8_t(TheResult);

}
