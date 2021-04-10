#include "config.h"
#include "ILedController.h"
#include <Logger.h>

void ILedController::loop(int* new_forward, int* old_forward, int* new_backward, int* old_backward){
   // is there a change detected
  if(*(old_forward) != *(new_forward) || *(old_backward) != *(new_backward)) { 
    if(Logger::getLogLevel() == Logger::VERBOSE) {
      char buf[512];
      snprintf(buf, 512, "change detected: forward is %d was %d, backward is %d was %d", 
         *(new_forward),*(old_forward),*(new_backward),*(old_backward));
      Logger::verbose(LOG_TAG_LED, buf);
    }

    this->fade((*new_forward) == HIGH);
      
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