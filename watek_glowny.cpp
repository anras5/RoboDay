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
            pthread_mutex_lock(&mutexTaken);
            // wanted = rand() % (C / 2) + 1;
            // wanted = rand() % C + 1;
            wanted = 3;
            taken = 0;

            // wysyłanie wszystkim ile chcę części
            debugln("[REST_PROJECT] Chcę %d części -> [WAIT_TAKE], AckCounterTake: %d", wanted, AckCounterTake);
            // ustawiamy wektor z informacjami od jakiego procesu dostaliśmy ACK_TAKE
            // aby poprawnie przyjmować od nich REQ_RETURN
            for (long unsigned int i = 0; i < ReceivedAckTake.size(); i++)
            {
                ReceivedAckTake.at(i) = false;
            }
            zmienStan(WAIT_TAKE);
            wyslijWszystkim(REQ_TAKE, wanted, -1);
            pthread_mutex_unlock(&mutexTaken);
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
                debugln("[WAIT_TAKE] C: %d, taken: %d, wanted: %d, AckCounterTake: %d", C, taken, wanted, AckCounterTake);
                czyZmienicStan = true;
                AckCounterTake = 0;
            }
            pthread_mutex_unlock(&mutexAckCounterTake);
            pthread_mutex_unlock(&mutexTaken);
            if (czyZmienicStan)
            {
                pthread_mutex_lock(&mutexOwned);
                pthread_mutex_lock(&mutexTaken);
                debugln("[WAIT_TAKE] Wchodzę do sekcji krytycznej -> [INSECTION_TAKE]");
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
            usleep(rand() % 10 * 100000);
            debugln("[INSECTION_TAKE] Pobrałem %d części, zmieniam stan na [REST_BUILDING]", owned);
            // symuluje wybieranie części poprzez krótkiego sleepa
            // przechodzi do budowy robota
            zmienStan(REST_BUILDING);
            break;
        }
        case REST_BUILDING:
        {
            // symuluje budowanie robota poprzez krótkiego sleepa
            usleep(rand() % 10 * 100000);
            debugln("[REST_BUILDING] Zbudowałem robota");
            powiekszLamport();

            // po zbudowaniu robota zmienia stan na WAIT_FIGHT i wysyła wszystkim REQ_FIGHT z takim samym lamportem
            pthread_mutex_lock(&mutexAckCounterFight);
            AckCounterFight = 0;
            pthread_mutex_unlock(&mutexAckCounterFight);
            zmienStan(WAIT_FIGHT);
            int lamportWyslania = wyslijWszystkimTakiSam(REQ_FIGHT, 0, -1);
            debugln("Wysyłam wszystkim REQ_FIGHT");
            // wpisuje siebie na listę FightQueue
            QueuePlace myPlace = {rank, lamportWyslania};
            wpiszNaFightQueue(myPlace);

            break;
        }
        case WAIT_FIGHT:
        {
            pthread_mutex_lock(&mutexAckCounterFight);
            // czekamy na AckCounterFight == n - 1
            if (AckCounterFight == size - 1)
            {
                // jeśli znajduję się na pozycji parzystej, wysyłam REQ_OPPONENT_FOUND do wszystkich
                // i czekam na AckCounterOpponent == n - 1
                pthread_mutex_lock(&mutexFightQueue);
                for (long unsigned int i = 0; i < FightQueue.size(); i++)
                {
                    if (FightQueue.at(i).idProcesu == rank && i % 2 == 1)
                    {
                        int idPrzeciwnika = FightQueue.at(i - 1).idProcesu;
                        wyslijWszystkim(REQ_OPPONENT_FOUND, 0, idPrzeciwnika);
                        usunZFightQueue(rank);
                        usunZFightQueue(idPrzeciwnika);
                        debugln("[WAIT_FIGHT] Znalazłem przeciwnika: %d", idPrzeciwnika);
                        break;
                    }
                }
                AckCounterFight = 0;
                pthread_mutex_unlock(&mutexFightQueue);
            }
            // czekamy na AckCounterOpponent == n - 1
            pthread_mutex_lock(&mutexAckCounterOpponent);
            if (AckCounterOpponent == size - 1)
            {
                AckCounterOpponent = 0;
                debugln("[WAIT_FIGHT] Zmieniam stan na [INSECTION_FIGHT]");
                zmienStan(INSECTION_FIGHT);
            }
            pthread_mutex_unlock(&mutexAckCounterOpponent);
            // wątek komunikacyjny odebrał od kogoś REQ_OPPONENT_FIGHT z naszym ID, przechodzimy do walki
            if (callToArms)
            {
                callToArms = false;
                debugln("[WAIT_FIGHT] Zostałem wybrany do walki, zmieniam stan na [INSECTION_FIGHT]");
                zmienStan(INSECTION_FIGHT);
            }
            pthread_mutex_unlock(&mutexAckCounterFight);
            break;
        }
        case INSECTION_FIGHT:
        {
            // symuluje walkę z przeciwnikiem poprzez krótkiego sleepa
            debugln("[INSECTION_FIGHT] Walczę");
            usleep(rand() % 10 * 800000);

            // losujemy ile chcemy oddać
            pthread_mutex_lock(&mutexOwned);
            int toReturn = rand() % owned;
            // wysyła wszystkim wiadomość o oddawaniu
            debugln("[REST_BUILDING] Oddaję %d części", toReturn);
            wyslijWszystkim(REQ_RETURN, toReturn, -1);
            owned = owned - toReturn;
            pthread_mutex_unlock(&mutexOwned);

            debugln("[REST_BUILDING] Zmieniam stan na [WAIT_RETURN]");
            zmienStan(WAIT_RETURN);
            break;
        }
        case WAIT_RETURN:
        {
            // czekamy na AckCounterReturn == n - 1
            pthread_mutex_lock(&mutexAckCounterReturn);
            if (AckCounterReturn == size - 1)
            {
                debugln("[WAIT_RETURN] Zmieniam stan na [REST_REPAIR]");
                zmienStan(REST_REPAIR);
                AckCounterReturn = 0;
            }
            pthread_mutex_unlock(&mutexAckCounterReturn);
            break;
        }
        case REST_REPAIR:
        { // symulacja naprawy poprzez krótkiego sleepa
            usleep(rand() % 10 * 100000);
            // wysyła wszystkim wiadomość o oddawaniu
            debugln("[REST_REPAIR] Oddaję %d części, zmieniam stan na [WAIT_RETURN_REMAINING]", owned);
            zmienStan(WAIT_RETURN_REMAINING);
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