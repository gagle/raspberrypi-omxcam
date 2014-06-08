#include "omxcam.h"
#include "internal.h"

void omxcam__event_error (omxcam__component_t* component){
  component->event.error = -1;
}

int omxcam__event_create (omxcam__component_t* component){
  if (pthread_mutex_init (&component->event.mutex, 0)){
    omxcam__error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&component->event.cond, 0)){
    omxcam__error ("pthread_cond_init");
    return -1;
  }
  
  component->event.error = component->event.flags = 0;
  
  return 0;
}

int omxcam__event_destroy (omxcam__component_t* component){
  if (pthread_mutex_destroy (&component->event.mutex)){
    omxcam__error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&component->event.cond)){
    omxcam__error ("pthread_cond_destroy");
    return -1;
  }
  
  return 0;
}

int omxcam__event_wake (omxcam__component_t* component, omxcam__event event){
  if (pthread_mutex_lock (&component->event.mutex)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  component->event.flags |= event;
  
  if (pthread_cond_signal (&component->event.cond)){
    omxcam__error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&component->event.mutex)){
    omxcam__error ("pthread_mutex_unlock");
    return -1;
  }
  
  return 0;
}

int omxcam__event_wait (
    omxcam__component_t* component,
    omxcam__event events,
    omxcam__event* current_events){
  //Wait for the events
  if (pthread_mutex_lock (&component->event.mutex)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  if (!(events & component->event.flags)){
    //Include the events to wait for
    component->event.flags |= events;
    
    if (pthread_cond_wait (&component->event.cond, &component->event.mutex)){
      omxcam__error ("pthread_cond_wait");
      return -1;
    }
  }
  
  uint32_t flags = component->event.flags;
  component->event.flags &= ~events;
  
  if (pthread_mutex_unlock (&component->event.mutex)){
      omxcam__error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (flags & OMXCAM_EVENT_ERROR){
    //omxcam__error() was called from the the EventHandler
    return -1;
  }
  
  if (current_events){
    *current_events = flags;
  }
  
  return component->event.error;
}