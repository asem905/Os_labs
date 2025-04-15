#ifndef CALTRAIN_H
#define CALTRAIN_H

#include <pthread.h>

struct station {
    
    int seatsAvailable;     
    int passengrsWaiting;  
    int boardedPassngrs;  // Passengers who have boarded but haven't signaled

    pthread_mutex_t lock;
    pthread_cond_t allBoarded;
    pthread_cond_t trainArrived;
    
};

void station_init(struct station *station);
void station_load_train(struct station *station, int count);
void station_wait_for_train(struct station *station);
void station_on_board(struct station *station);

#endif
