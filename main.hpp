#ifndef MAINH
#define MAINH

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <vector>

#define debug(FORMAT, ...) printf("%c[%d;%dm [%d, %d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamport, ##__VA_ARGS__, 27, 0, 37);

extern int rank;
extern int size;
extern int lamport;

extern std::vector<int> lamportyWyslania;

extern int C;
extern int wanted;
extern int owned;
extern int taken;
extern int AckCounterTake;
extern std::vector<int> FightQueue;
extern int AckCounterOpponent;
extern int AckCounterReturn;

extern std::vector<bool> ReceivedAckTake;

// stany
enum StanKonstruktora
{
    REST_PROJECT,
    WAIT_TAKE,
    INSECTION_TAKE,
    REST_BUILDING,
    WAIT_FIGHT,
    INSECTION_FIGHT,
    WAIT_RETURN,
    REST_REPAIR,
    WAIT_RETURN_REMAINING,
    FINISH
};
extern StanKonstruktora stan;

// wiadomosci
enum Wiadomosc
{
    REQ_TAKE,
    ACK_TAKE,
    REQ_FIGHT,
    ACK_FIGHT,
    REQ_OPPONENT_FOUND,
    ACK_OPPONENT_FOUND,
    REQ_RETURN,
    ACK_RETURN
};

// struktura pakietu
struct Packet
{
    int nadawca;
    int lamport;
    int liczbaCzesci;
    int idPrzeciwnika;
};
extern MPI_Datatype MPI_PAKIET_T;

// semafory
extern pthread_mutex_t mutexLamport;
extern pthread_mutex_t mutexLamportyWyslania;
extern pthread_mutex_t mutexStan;
extern pthread_mutex_t mutexWanted;
extern pthread_mutex_t mutexOwned;
extern pthread_mutex_t mutexTaken;
extern pthread_mutex_t mutexAckCounterTake;
extern pthread_mutex_t mutexAckCounterOpponent;
extern pthread_mutex_t mutexAckCounterReturn;

void *startWatekKom(void *ptr);
void mainLoop();

int wyslijPakiet(int odbiorca, int tag, int liczbaCzesci, int idPrzeciwnika);
void wyslijWszystkim(int tag, int liczbaCzesci, int idPrzeciwnika);

int powiekszLamport();
int powiekszMaxLamport(int packetClock);
void zmienStan(StanKonstruktora nowyStan);

#endif