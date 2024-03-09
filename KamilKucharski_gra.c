#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <stdbool.h>
// Od razu inicjuje process id oraz pamieci wspoldzielone, zeby handler ktory aktywuje sie podczas CTRL+C w terminalu mogl zabic proces potomny oraz zamknac pamieci
int shmid;
int *dane;
int shmid2;
char *nickPrzeciwnika;
pid_t              pid;
// Struktura ktora bedzie przesylana przez nas uzywajac UDP
struct message
{
    // Typ wiadomosci pozwoli mi latwiej rozrozniac kiedy nowa osoba dolacza, rozlacza lub wysyla normalna wiadomosc
    int typWiadomosci;
    /*
    1 oznacza dolaczenie nowej osoby
    2 oznacza rozlaczenie osoby
    3 oznacza zwykla wiadomosc
    */
    char          nick[32];
    char          text[256];
    bool czyNick;
};
// Handler zamykajacy program i zabijajacy proces potomny oraz sprzatajacy pamieci wspoldzielone
void handler(int signum)
{
    printf("Sprzatam...\n");
    shmdt(dane);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(nickPrzeciwnika);
    shmctl(shmid2, IPC_RMID, NULL);
    kill(pid, SIGTERM);
    exit(0);
}
// Funkcja losujaca poczatkowa liczbe
int losowa()
{
    srand(time(0));
    int r = rand() % 10 + 1;
    return r;
}
int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 4)
    {
        printf("Podano zla ilosc argumentow! Wylaczam program!\n");
        return 0;
    }

    char                ipPrzeciwnika[INET_ADDRSTRLEN];
    int                 port = atoi(argv[2]);
    struct message      msg;

    struct addrinfo     hints;
    struct addrinfo     *result,*rp;
    struct sockaddr_in  addr, from, to;
    socklen_t           from_len = sizeof(from);
    int                 sockfd;

    // Przypisuje uzycie CTRL+C pod handlera ktory kasuje proces potomny i pamieci wspoldzielone
    signal(SIGINT,handler);


    /* INICJUJE PAMIEC WSPOLDZIELONA.
    Pierwsza czesc to cztero-elementowa tablica intow o nazwie dane[] gdzie
    dane[0] to liczba naszych punktow
    dane[1] to liczba punktow przeciwnika
    dane[2] to aktualna tura (gdzie 0 oznacza nasz ruch, 1 oznacza ruch przeciwnika)
    dane[3] oznacza aktualna liczbe przekazywana przez programy (tzn. ta ktora dodajemy do 50)
    */
    shmid = shmget(IPC_PRIVATE, sizeof(int) * 4, IPC_CREAT | 0666);
    dane = (int *)shmat(shmid, NULL, 0);
    /* INICJUJE DRUGA PAMIEC WSPOLDZIELONA
        jest to tablica zawierajaca nick przeciwnika, potrzebna do wypisywania jego nicku podczas <wynik>
    */
    shmid2 = shmget(IPC_PRIVATE,sizeof(char[255]), IPC_CREAT | 0666);
    nickPrzeciwnika = (char *)shmat(shmid2, NULL, 0);
    if(shmid==-1 || shmid2==-1)
    {
        printf("Problem z pamiecia! Wychodze!\n");
        shmdt(dane);
        shmctl(shmid, IPC_RMID, NULL);
        shmdt(nickPrzeciwnika);
        shmctl(shmid2, IPC_RMID, NULL);
        return 0;
    }
    // "Szablon" do okreslenia tego co chcemy uzyskac z getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int blad = getaddrinfo(argv[1], NULL, &hints, &result);
    if (blad != 0)
    {
        printf("Blad przy getaddrinfo! Wylaczam program!\n");
        // Jesli blad to sprzatam pamiec wspoldzielona i wylaczam program
        shmdt(dane);
        shmctl(shmid, IPC_RMID, NULL);
        shmdt(nickPrzeciwnika);
        shmctl(shmid2, IPC_RMID, NULL);
        return 0;
    }
    // Listujemy po zwroconych wartosciach
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        if(rp==NULL)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    if(rp == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        shmdt(dane);
        shmctl(shmid, IPC_RMID, NULL);
        shmdt(nickPrzeciwnika);
        shmctl(shmid2, IPC_RMID, NULL);
        return 0;
    }
    freeaddrinfo(result);

    //Przypisujemy do wartosci ipPrzeciwnika jego adres w formacie XXX.XXX.XXX.XXX
    inet_ntop(AF_INET, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), ipPrzeciwnika, INET_ADDRSTRLEN);

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("Blad przy socket! Wylaczam program!\n");
        // Jesli blad to sprzatam pamiec wspoldzielona i wylaczam program
        shmdt(dane);
        shmctl(shmid, IPC_RMID, NULL);
        shmdt(nickPrzeciwnika);
        shmctl(shmid2, IPC_RMID, NULL);
        return 0;
    }

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);
    memset(&(addr.sin_zero), '\0', 8);

    to.sin_family      = AF_INET;
    to.sin_addr        = (((struct sockaddr_in *)rp->ai_addr)->sin_addr);
    to.sin_port        = htons(port);
    memset(&(to.sin_zero), '\0', 8);

    // Ustawiamy swoj nick o ile zostal podany. W przeciwnym wypadku zmienna czyNick bedzie potrzebna do wypisywania IP
    if(argc==4)
    {
        strcpy(msg.nick,argv[3]);
        msg.czyNick=true;
    }
    else
    {
        msg.czyNick=false;
    }
    // Bindujemy socketa
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(addr)) == -1)
    {
        printf("[Blad przy bindowaniu socketa! Wylaczam program!]\n");
        // Jesli blad to sprzatam pamiec wspoldzielona i wylaczam program
        shmdt(dane);
        shmctl(shmid, IPC_RMID, NULL);
        shmdt(nickPrzeciwnika);
        shmctl(shmid2, IPC_RMID, NULL);
        return 0;
    }
    // Wszystko jest ok, zaczynamy gre
    printf("[Wariant A]\n[Zaczynam gre z %s].\n[Napisz <koniec> by zakonczyc gre lub <wynik> by wyswietlic wynik.]\n",ipPrzeciwnika);

    printf("[Propozycja gry wyslana]\n");
    // Tworzymy program potomny ktory bedzie odpowiedzialny za odbieranie wiadomosci
    if((pid = fork()) == 0)
    {
        for(;;)
        {
            // Odbieramy wiadomosc
            recvfrom(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)&from,&from_len);
            //Jesli przeciwnik ma nick to dodajmy go, w przeciwnym wypadku uzywajmy jego adresu IP
            if(msg.czyNick==true)
            {
                strcpy(nickPrzeciwnika,msg.nick);
            }
            else
            {
                strcpy(nickPrzeciwnika,ipPrzeciwnika);
            }
            /*
            Teraz zaleznie od typu wiadomosci
            1: ktos dolaczyl
            2: osoba z ktora gralismy wyszla z gry
            3: osoba wyslala wiadomosc
            */
            //Otrzymalismy wiadomosc o nowej osobie
            if(msg.typWiadomosci==1)
            {
                /*
                    Zerujemy nasze punkty, punkty przeciwnika, ture ustawiamy nasza i losujemy liczbe, bo zaczynamy gre
                */
                dane[0]=0;
                dane[1]=0;
                dane[2]=0;
                int el=losowa();
                dane[3]=el;
                // Wklejam liczbe do naszej wiadomosci
                sprintf(msg.text,"%d",el);
                // Informujemy o dolaczeniu przeciwnika
                printf("\n[%s dolaczyl do gry, wartosc %d]\n",nickPrzeciwnika,el);
                printf("[Podaj kolejna wartosc]>\n");
            }
            // Otrzymalismy wiadomosc o wyjsciu z rogrywski przez druga osobe
            if(msg.typWiadomosci==2)
            {
                /*
                    Zerujemy nasze punkty, punkty przeciwnika, ture ustawiamy nasza i zerujemy liczbe, bo czekamy na nowa gre
                */
                dane[0]=0;
                dane[1]=0;
                dane[2]=0;
                dane[3]=0;
                // Informujemy o wyjsciu z gry przeciwnika
                printf("\n[%s zakonczyl gre, mozesz poczekac na nastepnego gracza]\n", nickPrzeciwnika);
            }
            // Otrzymalismy normalna wiadomosc
            if(msg.typWiadomosci==3)
            {
                //Ustawiamy dana ture na nasza
                dane[2]=0;
                // Przenosimy nasza liczbe ktora jest tablica char na inta
                int liczba=atoi(msg.text);
                // Zmieniamy aktualne punkty na podane przez przeicwnika
                dane[3]=liczba;
                // Wypisujemy co podal przeciwnik
                printf("[%s podal %d]\n", nickPrzeciwnika, liczba);
                // Jesli podana liczba to 50, przeciwnik wygral i dodajemy mu punkt a nasza liczbe zerujemy
                if(liczba==50)
                {
                    printf("[Przeciwnik wygral! Zaczynam kolejna rozgrywke!]\n");
                    dane[1]+=1;
                    dane[3]=0;
                }
                else
                {
                    printf("[Podaj kolejna liczbe:]\n");
                }
            }
        }
    }
    else
    {
        // TUTAJ JEST RODZIC, KTORY WYSYLA WIADOMOSCI

        // Od razu dolaczamy wiec zerujemy sobie wszystkie wartosci oraz wysylamy wiadomosc ktora oznajmiamy przeciwnikowi, ze dolaczylismy do gry
        msg.typWiadomosci = 1;
        dane[0]=0;
        dane[1]=0;
        dane[2]=1;
        dane[3]=0;
        sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)&to,sizeof(to));
        for(;;)
        {
            // Przenosimy buforowane dane do strumienia, wczytujemy nasza wiadomosc
            fflush(stdout);
            fgets(msg.text, 256, stdin);
            msg.text[strlen(msg.text)-1] = '\0';
            // Jesli nasza wiadomosc to <koniec>
            if(strcmp(msg.text, "<koniec>") == 0)
            {
                //Wysylamy wiadomosc o typie 2, po czym czyscimy pamieci, zabijamy proces potomny i wychodzimy z petli
                msg.typWiadomosci = 2;
                sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)&to,sizeof(to));
                shmdt(dane);
                shmctl(shmid, IPC_RMID, NULL);
                shmdt(nickPrzeciwnika);
                shmctl(shmid2, IPC_RMID, NULL);
                kill(pid, SIGTERM);
                break;
            }
            // Jesli nasza wiadomosc to <wynik> to wypisujemy nasz wynik oraz przeciwnika. Jesli przeciwnik nie podal nicku wypisujemy jego IP
            if(strcmp(msg.text, "<wynik>") == 0)
            {
                //(msg.czyNick==true)?printf("[Ty %d : %d %s]\n",dane[0],dane[1],nickPrzeciwnika):printf("[Ty %d : %d %s]\n",dane[0],dane[1],ipPrzeciwnika);
                printf("[Ty %d : %d %s]\n",dane[0],dane[1],nickPrzeciwnika);
            }
            // Jesli nie jest to <wynik> ani <koniec>
            else
            {
                // Jesli to nasza tura to dzialaj, w przeciwnym razie wypisz, ze nie mozesz wyslac swojej liczby
                if(dane[2]==0)
                {
                    // Zamieniamy wiadomosc na liczbe i sprawdzamy, czy spelnia ona nasze wymagania (czyli np. nie jest mniejsza niz aktualna liczba lub nie jest za duza (roznica wieksza niz 10)
                    int liczba=atoi(msg.text);
                    if((liczba-dane[3])>10 || (liczba-dane[3])<1 || liczba>50 || liczba<dane[3])
                    {
                        printf("[Podaj dobra liczbe!]\n");
                    }
                    else
                    {
                        // Ustawiamy nasza ture za zakonczona
                        dane[2]=1;
                        // Nowa liczba punktow to ta podana przez nas
                        dane[3]=liczba;
                        // Typ wiadomosci ustawiamy na zwykla wiadomosc
                        msg.typWiadomosci = 3;
                        // Wysylamy wiadomosc
                        sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)&to,sizeof(to));
                        // Jesli nasza liczba jest rowna 50 (czyli wygralismy) to wtedy...
                        if(liczba==50)
                        {
                            // Ustawiamy ture dla nas
                            dane[2]=0;
                            // Dodajemy sobie punkt
                            dane[0]=dane[0]+1;
                            // Losujemy nowa liczbe do nowej gry
                            int el=losowa();
                            dane[3]=el;
                            sprintf(msg.text,"%d",el);
                            printf("[Wygrales! Nowa rozgrywka, wylosowana liczba %d]\n",el);
                            fflush(stdout);
                            printf("[podaj kolejna wartosc]> ");
                            fflush(stdout);
                        }
                    }
                }
                else
                {
                    printf("[teraz tura drugiego gracza, poczekaj na swoja kolej]\n");
                }

            }
        }
    }
    // Konczymy program, zamykamy pamieci, socket i proces potomny
    shmdt(dane);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(nickPrzeciwnika);
    shmctl(shmid2, IPC_RMID, NULL);
    close(sockfd);
    kill(pid, SIGTERM);
    return 0;
}
//jak tak dalej pojdzie to za 20 lat napisze wlasne heroes 3
