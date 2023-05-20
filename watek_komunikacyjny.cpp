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
                // AckCounterTake++;
                debug("Otrzymałem REQ_TAKE od %d z %d częściami", p.nadawca, p.liczbaCzesci);
                // jeśli odebrany od j REQ_TAKE ma priorytet wyższy niż wysłany REQ_TAKE
                // to liczba części o które ubiega się j jest dodawana do zmiennej taken
                debug("p.lamport: %d, mój wysłany %d", p.lamport, lamportyWyslania.at(p.nadawca));
                if (p.lamport < lamportyWyslania.at(p.nadawca) || (p.lamport == lamport && p.nadawca < rank))
                {
                    pthread_mutex_lock(&mutexTaken);
                    taken = taken + p.liczbaCzesci;
                    pthread_mutex_unlock(&mutexTaken);
                    // wysyłamy posiadane części czyli 0 bo jesteśmy gorsi od pytającego
                    wyslijPakiet(p.nadawca, ACK_TAKE, 0, -1);
                }
                // przypadek kiedy czekamy jedynie na zwolnienie zasobów i nie pojawi się nikt przed nami
                // (oprócz czekających przed nami o których już wiemy)
                else if (AckCounterTake == size - 1)
                {
                    pthread_mutex_lock(&mutexWanted);
                    wyslijPakiet(p.nadawca, ACK_TAKE, wanted, -1);
                    debug("Wysłałem do %d ile zabiorę: %d", p.nadawca, wanted);
                    pthread_mutex_unlock(&mutexWanted);
                }
                // przypadek kiedy nasz lamport był mniejszy, ale nie mamy od wszystkich ACK_TAKE
                // oznacza to, że osoba pytająca dostała już nasze REQ_TAKE i wpisała sobie nas przed siebie
                // wysyłamy puste ACK_TAKE
                else
                {
                    wyslijPakiet(p.nadawca, ACK_TAKE, 0, -1);
                    debug("Wysłałem do %d puste ACK_TAKE", p.nadawca);
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
            if (stan == WAIT_TAKE)
            {
                pthread_mutex_lock(&mutexAckCounterTake);
                pthread_mutex_lock(&mutexTaken);
                // zwieksza AckCounterTake
                AckCounterTake++;
                // aktualizuje taken
                taken = taken + p.liczbaCzesci;
                ReceivedAckTake.at(p.nadawca) = true;
                debug("Otrzymałem ACK_TAKE od %d z %d częściami, taken: %d", p.nadawca, p.liczbaCzesci, taken);
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
                    debug("Otrzymałem REQ_RETURN od %d z %d częściami", p.nadawca, p.liczbaCzesci);
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