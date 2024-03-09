# Gra Dodawanie do 50

## Opis Projektu
Projekt składa się z pojedynczego programu w języku C, umożliwiającego rozgrywkę w grę "dodawanie do 50" pomiędzy dwoma graczami na różnych maszynach. Komunikacja odbywa się za pomocą protokołu UDP IPv4. Gra jest w pełni asynchroniczna, co pozwala na odbieranie i wysyłanie wiadomości w dowolnym momencie gry.

## Zasady Gry
Gra "dodawanie do 50" polega na tym, że gracze na przemian dodają liczbę naturalną od 1 do 10 do wspólnej sumy. Gra rozpoczyna się od liczby losowej z tego samego zakresu. Celem gry jest osiągnięcie sumy równającej się dokładnie 50. Gracz, który poda liczbę 50, wygrywa.

## Wymagania Systemowe
- Linux lub system uniksopodobny
- Kompilator języka C (np. gcc)
- Biblioteki: stdio.h, stdlib.h, arpa/inet.h, netdb.h, sys/socket.h, netinet/in.h, unistd.h, string.h, signal.h, sys/shm.h, time.h, stdbool.h

## Instrukcja Uruchomienia
1. Skompiluj program używając kompilatora języka C, np. `gcc -o gra KamilKucharski_gra.c`.
2. Uruchom program na dwóch różnych maszynach z następującymi argumentami:
   - Adres IP lub domenowy hosta, z którym chcesz się połączyć.
   - Numer portu, używany do komunikacji.
   - Opcjonalnie, nick użytkownika. Jeśli nie podany, używane jest IP.
   
   Przykład: `./gra 192.168.0.2 8080 Janek`

## Możliwe Komendy Podczas Gry
- `<koniec>` - informuje przeciwnika o zakończeniu gry i zamyka aplikację.
- `<wynik>` - wyświetla aktualny wynik rozgrywki między graczami.

## Specyfikacja Techniczna
- Program obsługuje błędy i stosuje dobre praktyki programistyczne.
- Interfejs użytkownika jest estetyczny i zgodny ze specyfikacją.
- Komunikacja odbywa się za pomocą UDP, co wymaga otwartego portu.
- Program wykorzystuje pamięć współdzieloną do zarządzania stanem gry.

## Uwagi
- Program nie pozostawia procesów sierot ani zombie po zakończeniu działania.

## Autor
Kamil Kucharski

