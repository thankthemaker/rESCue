// Demo's how to play multiple sounds one after the other, those sounds can be WAVs
// or Music Score's or a mixture, here we demo wavs, showing that you can add the same wav
// more than once and also clear out the sequence and create a different sequence 
// Compile and upload the code, open the serial monitor and type in any number. The number will
// be spoke back to you a digit at a time and also displayed on the serial monitor.
// See www.xtronical.com for write ups on sound and the hardare required


#include "SoundData.h"
#include "XT_DAC_Audio.h"

                                      
XT_DAC_Audio_Class DacAudio(25,0);          // Create the main player class object. Use GPIO 25, one of the 2 DAC pins and timer 0

// All the number sounds, my voice - Sorry!
XT_Wav_Class Zero(ZeroWav);          
XT_Wav_Class One(OneWav);          
XT_Wav_Class Two(TwoWav);           
XT_Wav_Class Three(ThreeWav);           
XT_Wav_Class Four(FourWav);           
XT_Wav_Class Five(FiveWav);       
XT_Wav_Class Six(SixWav);        
XT_Wav_Class Seven(SevenWav);          
XT_Wav_Class Eight(EightWav);           
XT_Wav_Class Nine(NineWav);                                     
                                      

XT_Sequence_Class Sequence;               // The sequence object, you add your sounds above to this object (see setup below)


void setup() {
  Serial.begin(115200);
}

void loop() {
  DacAudio.FillBuffer();          // This needs only be in your main loop once, suggest here at the top.

  // Get a number entered from the user on the serial port
  if(Serial.available()) 
    PlayNumber(Serial.readString().c_str());
}

void PlayNumber(char const *Number)
{  
  int NumChars=strlen(Number);              // could lose this line of put strlen in loop below, but bad form to do so
  Sequence.RemoveAllPlayItems();            // Clear out any previous playlist
  for(int i=0;i<NumChars;i++)
    AddNumberToSequence(Number[i]);         // For each number add in the sound for that number to the sequence
  DacAudio.Play(&Sequence);                 // Play the sequence, will not wait here to complete, works independently of your code
  Serial.println(Number);                   // Confirm number entered to the user over the serial
}

void AddNumberToSequence(char TheNumber)
{
  // Adds in the wav for the single 0-9 number passed in as a char

  switch(TheNumber)
  {
    case '0' : Sequence.AddPlayItem(&Zero);break;
    case '1' : Sequence.AddPlayItem(&One);break;
    case '2' : Sequence.AddPlayItem(&Two);break;
    case '3' : Sequence.AddPlayItem(&Three);break;
    case '4' : Sequence.AddPlayItem(&Four);break;
    case '5' : Sequence.AddPlayItem(&Five);break;
    case '6' : Sequence.AddPlayItem(&Six);break;
    case '7' : Sequence.AddPlayItem(&Seven);break;
    case '8' : Sequence.AddPlayItem(&Eight);break;
    case '9' : Sequence.AddPlayItem(&Nine);break;
  }
}
