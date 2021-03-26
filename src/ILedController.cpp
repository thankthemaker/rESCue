#include "config.h"
#include "ILedController.h"

void ILedController::loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward){
   // is there a change detected
  if(*(old_forward) != *(new_forward) || *(old_backward) != *(new_backward)) { 
#if DEBUG > 1
    Serial.print("change detected: ");
    Serial.print(", forward is "  + String(*new_forward)  + " was " + String(*old_forward));
    Serial.println(", backward is " + String(*new_backward) + " was " + String(*old_backward));
#endif

    this->fade(new_forward);
      
    *(old_forward) = *(new_forward);
    *(old_backward) = *(new_backward);
    return;
  } 

  //idle state???
  if(*(old_forward) == LOW && *(old_backward) == LOW && 
     *(new_forward) == LOW && *(new_backward) == LOW) {
    this->idleSequence();
  }
}