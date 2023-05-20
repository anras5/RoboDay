#include "main.hpp"

void mainLoop()
{
    srand(rank);

    while (stan != FINISH)
    {

        switch (stan)
        {
        case REST_PROJECT:
        { // odpoczynek
            usleep(rand() % 10 * 300000);

            // losowanie części
            pthread_mutex_lock(&mutexWanted);
            // wanted = rand() % (C / 2) + 1;
            wanted = 8;
            // wysyłanie wszystkim ile chcę części
            debug("[REST_PROJECT] Chcę %d części -> [WAIT_TAKE]", wanted);
            // ustawiamy wektor z informacjami od jakiego procesu dostaliśmy ACK_TAKE
            // aby poprawnie przyjmować od nich REQ_RETURN
            for (long unsigned int i = 0; i < ReceivedAckTake.size(); i++)
            {
                ReceivedAckTake.at(i) = false;
            }
            zmienStan(WAIT_TAKE);
            wyslijWszystkim(REQ_TAKE, wanted, -1);
            pthread_mutex_unlock(&mutexWanted);
            // zmiana stanu na WAIT_TAKE
            break;
        }
        case WAIT_TAKE:
        { // sprawdzamy czy AckCounterTake == n - 1 oraz C - taken >= wanted
            // jeżeli tak to wchodzimy do sekcji i zabieramy części
            bool czyZmienicStan = false;
            pthread_mutex_lock(&mutexAckCounterTake);
            pthread_mutex_lock(&mutexTaken);
            if ((AckCounterTake == size - 1) && (C - taken >= wanted))
            {
                debug("[WAIT_TAKE] C: %d, taken: %d, wanted: %d, AckCounterTake: %d", C, taken, wanted, AckCounterTake);
                czyZmienicStan = true;
                AckCounterTake = 0;
            }
            pthread_mutex_unlock(&mutexAckCounterTake);
            pthread_mutex_unlock(&mutexTaken);
            if (czyZmienicStan)
            {
                pthread_mutex_lock(&mutexOwned);
                pthread_mutex_lock(&mutexTaken);
                debug("[WAIT_TAKE] Wchodzę do sekcji krytycznej -> [INSECTION_TAKE]");
                pthread_mutex_unlock(&mutexTaken);
                zmienStan(INSECTION_TAKE);
            }
            break;
        }
        case INSECTION_TAKE:
        { // aktualizuje liczbę posiadanych części: ustawia owned na wanted
            owned = wanted;
            pthread_mutex_unlock(&mutexOwned);
            // ustawia wanted na 0
            pthread_mutex_lock(&mutexWanted);
            wanted = 0;
            pthread_mutex_unlock(&mutexWanted);
            debug("[INSECTION_TAKE] Pobrałem %d części", owned);
            // symuluje wybieranie części poprzez krótkiego sleepa
            usleep(rand() % 10 * 100000);
            // przechodzi do budowy robota
            debug("[INSECTION_TAKE] Zmieniam stan na [REST_BUILDING]");
            zmienStan(REST_BUILDING);
            break;
        }
        case REST_BUILDING:
        { // symuluje budowanie robota poprzez krótkiego sleepa
            usleep(rand() % 10 * 100000);
            debug("[REST_BUILDING] Zbudowałem robota");

            // ---------------------------------- TODO ---------------------------- //
            // na razie z REST_BUILDING przechodzi od razu do WAIT_RETURN
            // jest to wersja bez walki

            // losujemy ile chcemy oddać
            pthread_mutex_lock(&mutexOwned);
            int toReturn = rand() % owned;
            // wysyła wszystkim wiadomość o oddawaniu
            debug("[REST_BUILDING] Oddaję %d części", toReturn);
            wyslijWszystkim(REQ_RETURN, toReturn, -1);
            owned = owned - toReturn;
            pthread_mutex_unlock(&mutexOwned);

            debug("[REST_BUILDING] Zmieniam stan na [WAIT_RETURN]");
            zmienStan(WAIT_RETURN);

            break;
        }
        case WAIT_FIGHT:
            // TODO
            break;
        case INSECTION_FIGHT:
            // TODO
            break;
        case WAIT_RETURN:
        { // czekamy na AckCounterReturn == n - 1
            pthread_mutex_lock(&mutexAckCounterReturn);
            if (AckCounterReturn == size - 1)
            {
                debug("[WAIT_RETURN] Zmieniam stan na [REST_REPAIR]");
                zmienStan(REST_REPAIR);
                AckCounterReturn = 0;
            }
            pthread_mutex_unlock(&mutexAckCounterReturn);
            break;
        }
        case REST_REPAIR:
        { // symulacja naprawy poprzez krótkiego sleepa
            usleep(rand() % 10 * 100000);
            // ustawia locka na AckCounterReturn i zeruje wartość
            pthread_mutex_lock(&mutexAckCounterReturn);
            AckCounterReturn = 0;
            pthread_mutex_unlock(&mutexAckCounterReturn);
            // wysyła wszystkim wiadomość o oddawaniu
            debug("[REST_REPAIR] Zmieniam stan na [WAIT_RETURN_REMAINING]");
            zmienStan(WAIT_RETURN_REMAINING);
            debug("[REST_REPAIR] Oddaję %d części", owned);
            wyslijWszystkim(REQ_RETURN, owned, -1);
            pthread_mutex_lock(&mutexOwned);
            owned = 0;
            pthread_mutex_unlock(&mutexOwned);
            break;
        }
        case WAIT_RETURN_REMAINING:
        { // czekamy na AckCounterReturn == n - 1
            pthread_mutex_lock(&mutexAckCounterReturn);
            if (AckCounterReturn == size - 1)
            {
                zmienStan(REST_PROJECT);
                AckCounterReturn = 0;
            }
            pthread_mutex_unlock(&mutexAckCounterReturn);
            break;
        }
        default:
            break;
        }
    }
}