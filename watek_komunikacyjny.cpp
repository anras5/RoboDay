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

        pthread_mutex_lock(&mutexStan);
        int stanAktualny = stan;
        pthread_mutex_unlock(&mutexStan);

        switch (status.MPI_TAG)
        {
        case REQ_TAKE:
        {
            if (stanAktualny == WAIT_TAKE)
            {
                pthread_mutex_lock(&mutexLamportyWyslania);
                pthread_mutex_lock(&mutexAckCounterTake);
                debug("[WAIT_TAKE] Otrzymałem REQ_TAKE od %d z %d częściami, LAMPORT_%d: %d, LAMPORT_%d: %d", p.nadawca, p.liczbaCzesci, p.nadawca, p.lamport, rank, lamportyWyslania.at(p.nadawca));                
                if (p.lamport > lamportyWyslania.at(p.nadawca) || (p.lamport == lamportyWyslania.at(p.nadawca) && p.nadawca > rank)) {
                    // wysyłamy w ACK_TAKE naszą wartość wanted, ponieważ
                    // j jest za nami w kolejce i musi nas wziąc pod uwagę
                    pthread_mutex_lock(&mutexWanted);
                    wyslijPakiet(p.nadawca, ACK_TAKE, wanted, -1);
                    debug("[WAIT_TAKE] Wysłałem do %d ile zabiorę: %d", p.nadawca, wanted);
                    pthread_mutex_unlock(&mutexWanted);
                } else {
                    // wysyłamy w ACK_TAKE 0, ponieważ
                    // j jest przed nami w kolejce i nie interesuje go nasza wartość wanted
                    debug("[WAIT_TAKE] Wysłałem do %d że jestem za nim", p.nadawca);
                    wyslijPakiet(p.nadawca, ACK_TAKE, 0, -1);
                }
                pthread_mutex_unlock(&mutexAckCounterTake);
                pthread_mutex_unlock(&mutexLamportyWyslania);
            }
            else
            {
                // wysyłamy posiadane części
                pthread_mutex_lock(&mutexOwned);
                wyslijPakiet(p.nadawca, ACK_TAKE, owned, -1);
                pthread_mutex_unlock(&mutexOwned);
            }
            break;
        }
        case ACK_TAKE:
        {
            if (stanAktualny == WAIT_TAKE)
            {
                pthread_mutex_lock(&mutexAckCounterTake);
                pthread_mutex_lock(&mutexTaken);
                // zwieksza AckCounterTake
                AckCounterTake++;
                // aktualizuje taken
                taken = taken + p.liczbaCzesci;
                ReceivedAckTake.at(p.nadawca) = true;
                debug("[WAIT_TAKE] Otrzymałem ACK_TAKE od %d z %d częściami, taken: %d", p.nadawca, p.liczbaCzesci, taken);
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
            if (stanAktualny == WAIT_TAKE)
            {
                if (ReceivedAckTake.at(p.nadawca))
                {
                    debug("[WAIT_TAKE] Otrzymałem REQ_RETURN od %d z %d częściami", p.nadawca, p.liczbaCzesci);
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
            pthread_mutex_lock(&mutexAckCounterReturn);
            AckCounterReturn++;
            pthread_mutex_unlock(&mutexAckCounterReturn);
            break;
        }
        }
    }
    return 0;
}