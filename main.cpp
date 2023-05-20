#include "main.hpp"

int rank;
int size;
int lamport = 0;

int C = 10;
int wanted = 0;
int owned = 0;
int taken = 0;
int AckCounterTake = 0;
std::vector<int> FightQueue;
std::vector<bool> ReceivedAckTake;
std::vector<int> lamportyWyslania;
int AckCounterOpponent = 0;
int AckCounterReturn = 0;

// watek komunikacyjny
pthread_t watekKom;

// stan poczatkowy
StanKonstruktora stan = REST_PROJECT;

// Zmienna opisująca przesyłane dane
MPI_Datatype MPI_PAKIET_T;

// semafory
pthread_mutex_t mutexLamport = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexStan = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexWanted = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexOwned = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTaken = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexAckCounterTake = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexAckCounterOpponent = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexAckCounterReturn = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexLamportyWyslania = PTHREAD_MUTEX_INITIALIZER;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}

int wyslijPakiet(int odbiorca, int tag, int liczbaCzesci, int idPrzeciwnika)
{
    Packet *p = new Packet;

    int lamportWyslania = powiekszLamport();
    p->nadawca = rank;
    p->lamport = lamport;
    p->idPrzeciwnika = idPrzeciwnika;
    p->liczbaCzesci = liczbaCzesci;

    MPI_Send(p, 1, MPI_PAKIET_T, odbiorca, tag, MPI_COMM_WORLD);

    delete p;

    return lamportWyslania;
}

void wyslijWszystkim(int tag, int liczbaCzesci, int idPrzeciwnika)
{
    for (int i = 0; i < size; i++)
    {
        if (i != rank)
        {
            pthread_mutex_lock(&mutexLamportyWyslania);
            int lamportWyslania = wyslijPakiet(i, tag, liczbaCzesci, idPrzeciwnika);
            lamportyWyslania.at(i) = lamportWyslania;
            pthread_mutex_unlock(&mutexLamportyWyslania);
        }
    }
}

int powiekszLamport()
{
    pthread_mutex_lock(&mutexLamport);
    lamport++;
    int lamportToReturn = lamport;
    pthread_mutex_unlock(&mutexLamport);
    return lamportToReturn;
}

int powiekszMaxLamport(int packetClock)
{
    pthread_mutex_lock(&mutexLamport);
    lamport = std::max(packetClock, lamport) + 1;
    int lamportToReturn = lamport;
    pthread_mutex_unlock(&mutexLamport);
    return lamportToReturn;
}

void zmienStan(StanKonstruktora nowyStan)
{
    pthread_mutex_lock(&mutexStan);
    stan = nowyStan;
    pthread_mutex_unlock(&mutexStan);
}

void inicjuj(int *argc, char ***argv)
{

    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    // check_thread_support(provided);

    // wczytujemy rank i size
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int nitems = 4;
    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype typy[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[4];
    offsets[0] = offsetof(Packet, nadawca);
    offsets[1] = offsetof(Packet, lamport);
    offsets[2] = offsetof(Packet, liczbaCzesci);
    offsets[3] = offsetof(Packet, idPrzeciwnika);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    srand(rank);
    srandom(rank);

    for (int i = 0; i < size; i++)
    {
        lamportyWyslania.push_back(0);
        ReceivedAckTake.push_back(0);
    }

    pthread_create(&watekKom, NULL, startWatekKom, 0);
}

void finalizuj()
{
    pthread_mutex_destroy(&mutexLamport);
    pthread_mutex_destroy(&mutexStan);

    pthread_join(watekKom, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

int main(int argc, char **argv)
{

    inicjuj(&argc, &argv);

    mainLoop();

    finalizuj();
    return 0;
}