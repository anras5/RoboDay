#include "main.hpp"

void *startWatekKom(void *ptr)
{

    MPI_Status status;
    Packet p;

    srand(rank);
    while (stan != FINISH)
    {
        MPI_Recv(&p, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        powiekszMaxLamport(p.lamport);

        switch (status.MPI_TAG)
        {
        case REQ_TAKE:
        {
            if (stan == WAIT_TAKE)
            {
                pthread_mutex_lock(&mutexLamportyWyslania);
                pthread_mutex_lock(&mutexAckCounterTake);
                AckCounterTake++;
                // jeśli odebrany od j REQ_TAKE ma priorytet wyższy niż wysłany REQ_TAKE
                // to liczba części o które ubiega się j jest dodawana do zmiennej taken
                if (p.lamport < lamportyWyslania.at(p.nadawca) || (p.lamport == lamport && p.nadawca < rank))
                {
                    pthread_mutex_lock(&mutexTaken);
                    taken = taken + p.liczbaCzesci;
                    pthread_mutex_unlock(&mutexTaken);
                }
                pthread_mutex_unlock(&mutexAckCounterTake);
                pthread_mutex_unlock(&mutexLamportyWyslania);
            }
            // wysyłamy posiadane części
            pthread_mutex_lock(&mutexOwned);
            wyslijPakiet(p.nadawca, ACK_TAKE, owned, -1);
            pthread_mutex_unlock(&mutexOwned);
            break;
        }
        case ACK_TAKE:
        {
            if (stan == WAIT_TAKE)
            {
                pthread_mutex_lock(&mutexAckCounterTake);
                pthread_mutex_lock(&mutexTaken);
                // zwieksza AckCounterTake
                AckCounterTake++;
                // aktualizuje taken
                taken = taken + p.liczbaCzesci;
                ReceivedAckTake.at(p.nadawca) = true;
                pthread_mutex_unlock(&mutexAckCounterTake);
                pthread_mutex_unlock(&mutexTaken);
            }
            break;
        }
        case REQ_FIGHT:
            break;
        case ACK_FIGHT:
            break;
        case REQ_OPPONENT_FOUND:
            break;
        case ACK_OPPONENT_FOUND:
            break;
        case REQ_RETURN:
        {
            if (stan == WAIT_TAKE)
            {
                if (ReceivedAckTake.at(p.nadawca))
                {
                    debug("Otrzymałem REQ_RETURN z %d częściami", p.liczbaCzesci);
                    pthread_mutex_lock(&mutexTaken);
                    taken = taken - p.liczbaCzesci;
                    pthread_mutex_unlock(&mutexTaken);
                }
            }
            wyslijPakiet(p.nadawca, ACK_RETURN, -1, -1);
            break;
        }
        case ACK_RETURN:
        {
            if (stan == WAIT_RETURN || stan == WAIT_RETURN_REMAINING)
            {
                pthread_mutex_lock(&mutexAckCounterReturn);
                AckCounterReturn++;
                pthread_mutex_unlock(&mutexAckCounterReturn);
            }
            break;
        }
        }
    }
    return 0;
}