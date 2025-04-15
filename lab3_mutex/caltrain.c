#include <pthread.h>
#include "caltrain.h"

void station_init(struct station *station) {
    station->passengrsWaiting= 0;
    station->seatsAvailable = 0;
    station->boardedPassngrs = 0;

    pthread_mutex_init(&station->lock, NULL);//to prevent any other train from interfer
    pthread_cond_init(&station->trainArrived, NULL);//train arrive so init every thing
    pthread_cond_init(&station->allBoarded, NULL);//waiting for train to be fill
}

void station_load_train(struct station *station, int count) {
    pthread_mutex_lock(&station->lock);

    station->seatsAvailable = count;

    // Wake up waiting passengers if there are available seats
    while (station->seatsAvailable > 0 && station->passengrsWaiting> 0) {
        pthread_cond_broadcast(&station->trainArrived);//to tell all that train has arrived
        pthread_cond_wait(&station->allBoarded, &station->lock);
    }

    station->seatsAvailable = 0;
    pthread_mutex_unlock(&station->lock);
}

void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&station->lock);

    station->passengrsWaiting++;

    // Wait until a train arrives with free seats
    while (station->seatsAvailable == 0) {
        pthread_cond_wait(&station->trainArrived, &station->lock);
    }

    station->passengrsWaiting--;
    station->seatsAvailable--;
    station->boardedPassngrs++;

    pthread_mutex_unlock(&station->lock);
}

void station_on_board(struct station *station) {
    pthread_mutex_lock(&station->lock);

    station->boardedPassngrs--;

    if (station->boardedPassngrs == 0 && 
        (station->seatsAvailable == 0 || station->passengrsWaiting== 0)) {
        pthread_cond_signal(&station->allBoarded);
    }

    pthread_mutex_unlock(&station->lock);
}
