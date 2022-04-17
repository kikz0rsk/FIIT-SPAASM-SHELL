/**
* Part 1 assignment
* Napíšte v jazyku C jednoduchý interaktívny program, "shell", ktorý bude opakovane čakať na
* zadanie príkazu a potom ho spracuje. Na základe princípov klient-server architektúry tak musí
* s pomocou argumentov umožňovať funkciu servera aj klienta. Program musí umožňovať
* spúšťať zadané príkazy a bude tiež interpretovať aspoň nasledujúce špeciálne znaky: # ; < >
* | \ . Príkazy musí byť možné zadať zo štandardného vstupu a tiež zo spojení
* reprezentovaných soketmi. Na príkazovom riadku musí byť možné špecifikovať
* prepínačom -p port číslo portu a/alebo prepínačom -u cesta názov lokálneho soketu na
* ktorých bude program čakať na prichádzajúce spojenia. Po spustení s prepínačom -h sa musia
* vypísať informácie o autorovi, účele a použití programu, zoznam príkazov. "Shell" musí
* poskytovať aspoň nasledujúce interné príkazy: help - výpis informácií ako pri -h, quit -
* ukončenie spojenia z ktorého príkaz prišiel, halt - ukončenie celého programu.
* Prednastavený prompt musí pozostávať z mena používateľa, názvu stroja, aktuálneho času a
* zvoleného ukončovacieho znaku, e.g. '16:34 user17@student#'. Na zistenie týchto informácií
* použite vhodné systémové volania s použitím knižničných funkcií. Na formátovanie výstupu,
* zistenie mena používateľa z UID a pod. môžte v programe využiť bežné knižničné funkcie.
* Spúšťanie príkazov a presmerovanie súborov musia byť implementované pomocou príslušných
* systémových volaní. Tie nemusia byť urobené priamo (cez assembler), avšak knižničná
* funkcia popen(), prípadne podobná, nesmie byť použitá. Pri spustení programu bez
* argumentov, alebo s argumentom "-s" sa program bude správať vyššie uvedeným spôsobom,
* teda ako server. S prepínačom "-c" sa bude správať ako klient, teda program nadviaže
* spojenie so serverom cez socket, do ktorého bude posielať svoj štandardný vstup a čítať dáta
* pre výstup. Chybové stavy ošetrite bežným spôsobom. Počas vytvárania programu (najmä
* kompilácie) sa nesmú zobrazovať žiadne varovania a to ani pri zadanom prepínači
* prekladača -Wall.
* Vo voliteľných častiach zadania sa očakáva, že tie úlohy budú mať vaše vlastné riešenia, nie
* jednoduché volania OS.
* 
* Nasledujúce časti predstavujú povinné minimum pre akceptovanie funkčného zadania a
* po splnení budú hodnotené 6 bodmi.
* spracovanie argumentov, spracovanie zadaného vstupného riadku, interné
* príkazy help, halt, quit.
* * overenie činnosti a spustenie zadaných príkazov, presmerovanie
* (volania fork, exec, wait, pipe, dup).
* * sokety, spojenia
* (volania socket, listen, accept, bind, connect, select, read, write); systémové
* volania pre prompt.
*
* Part 2 features
* * Switch -i for specifying IP addresses (2 points)
* * Switch -c with -i and -u (3 points)
* * Built-in command stat for displaying connected clients (3 points)
* * -d to daemonize the server (4 points)
*/

#include "kshell.h"
#include "common.h"
#include "arguments.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "client.h"

int main(int argc, char** argv)
{
	struct arguments args = { argc, argv };

	if (args.count == 1 || is_flag_set(&args, "-h")) {
		char* help = get_help();
		puts(help);
		safe_free((void**)&help);
		return 0;
	}

	int port = 25566;
	char* customPort;
	if ((customPort = get_flag_value(&args, "-p")) != NULL) {
		port = atoi(customPort);
	}

	bool clientFlag = is_flag_set(&args, "-c");
	bool serverFlag = is_flag_set(&args, "-s");

	if (!clientFlag && !serverFlag) {	// Default mode is a server mode
		printf("No mode specified, running in server mode\n");
		serverFlag = true;
	}

	bool useLocalSocket = false;
	char* target = get_flag_value(&args, "-i");
	if (target == NULL) {
		if (is_flag_set(&args, "-p")) {
			target = NULL;
		}
		else {
			target = get_flag_value(&args, "-u");
			useLocalSocket = true;
			if (target == NULL) {
				target = KSHELL_SOCK_PATH;
			}
		}
	}

	bool daemon = false;
	if (is_flag_set(&args, "-d") && serverFlag) {
		if (fork() != 0) {
			return 0;
		}
		daemon = true;
		close(STDIN_FILENO);
	}

	if (serverFlag) {
		server(&args, target, port, useLocalSocket, daemon);
		if (!daemon) {
			client(&args, target, port, useLocalSocket);
		}
	}
	else {
		client(&args, target, port, useLocalSocket);
	}

	return 0;
}

char* get_help() {
	char helpString[] = { "kShell - interactive remote shell\n"
		"Author: Christian Danížek\n"
		"Usage: kshell <flags> <-c | -s>\n"
		"Flags:\n"
		"  -h  - display this help\n"
		"  -p <port>  - specify port number (default 25566)\n"
		"  -u <path>  - set path to local socket (overrides -i and -p)\n"
		"  -i <ip>  - set IP address\n"
		"  -c  - client mode\n"
		"  -s  - server mode\n"
		"Built-in commands:\n"
		"  help  - display this help\n"
		"  quit  - close remote session\n"
		"  halt  - close shell server\n"
		"  stat  - display connected clients and their addresses\n"
	};
	char* help = malloc(sizeof(helpString) * sizeof(char));
	memcpy(help, helpString, sizeof(helpString));
	return help;
}