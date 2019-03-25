/*
 * main.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define BUFF_SIZE 128
#define WHITE_SIGNS " \t\r\n"

void mainLoop(int, FILE *);
char *readLine(void);
char **splitLine(char *);
int processCreate(char **);
FILE *fileOpen(char *);
char *readFileLine(FILE *);
void saveHistory(char *);
void printHistory(int);

int amp = 0;									//zmienna globalna do ustalenia czy na koncu linii byl &

int main(int argc, char **argv) {

	struct sigaction sa;

	sa.sa_handler = printHistory;				//funkcja obslugujaca sygnal
	sigfillset(&(sa.sa_mask));					//blokowanie sygnalow na czas obslugi
	sa.sa_flags = 0;							//zerowanie flag
	sigaction(SIGQUIT, &sa, 0);					//obsluga SIGQUIT

	if (argc == 2) {							//jezeli shell uruchomiony ze skryptu
		mainLoop(1, fileOpen(argv[1]));			//czytaj skrypt
	}
	mainLoop(0, NULL);							//zwykle uzycie shella
	return 0;
}

void mainLoop(int version, FILE *fp) {
	char *line, **args;
	int status;

	do {
		if (version == 0) {						//jezeli zwykle uzycie shella
			printf("MyShell>");					//wyswietl prompt
			line = readLine();					//czytaj linie z stdin
		} else
			line = readFileLine(fp);			//jezeli uruchomiony ze skryptu czytaj linie z pliku

		args = splitLine(line);					//podziel linie na wyrazy
		status = processCreate(args);			//stworz procesy i obsluz polecenie

		free(line);
		free(args);
	} while (status);
}

char *readLine(void) {
	int bufferSize = BUFF_SIZE, i = 0, c;
	char *buffer = malloc(sizeof(char) * bufferSize);		//alokacja pamięci na bufor

	if (!buffer) {
		printf("can't allocate memory for buffer\n");
	}

	while (1) {
		c = getchar();										//czytaj pojedynczy znak

		if (c == '\n' || c == EOF) {						//jezeli koniec linii lub pliku
			if (buffer[i - 1] == '&')						//jezeli na koncu linii byl &
				amp = 1;									//ustaw zmienna globalna
			else
				amp = 0;
			buffer[i] = '\0';								//dodaj znak konca wyrazu na koncu bufora
			saveHistory(buffer);							//zapisz historie polecenia
			return buffer;
		} else {
			buffer[i] = c;									//zapisuj kolejne znaki do bufora
		}
		i++;
	}

	if (i >= bufferSize) {									//realokoacja bufora jezeli za maly
		bufferSize += BUFF_SIZE;
		buffer = realloc(buffer, bufferSize);
		if (!buffer) {
			printf("can't reallocate memory for buffer\n");
		}
	}
}

char **splitLine(char *line) {
	int bufferSize = BUFF_SIZE, i = 0;
	char **words = malloc(bufferSize * sizeof(char*));		//alokacja pamieci na wyrazy
	char *word;

	if (!words) {
		printf("can't allocate memory for words\n");
	}

	word = strtok(line, WHITE_SIGNS);						//wydzielenie wyrazow dzieki bialym znakom
	while (word != NULL) {
		words[i] = word;									//zapis wyrazow do tablicy
		i++;
		if (strcmp(word, "exit") == 0)						//jezeli ktorys z wyrazow to exit wyjdz z programu
			exit(0);

		if (i >= bufferSize) {								//realokacji bufora jezeli za maly
			bufferSize += BUFF_SIZE;
			words = realloc(words, bufferSize * sizeof(char*));
			if (!words) {
				printf("can't reallocate memory for words\n");
			}
		}

		word = strtok(NULL, WHITE_SIGNS);
	}

	words[i] = NULL;										//dopisanie NULLA do konca
	return words;
}

int processCreate(char **args) {
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {											//proces potomny
		if (execvp(args[0], args) == -1) {					//jezeli blad execvp
			printf("nie znaleziono polecenia\n");			//wypisz blad
		}
		exit(0);											//zamknij proces potomny
	} else if (pid < 0) {									//jezeli blad forka
		printf("fork error\n");
	} else {												//proces macierzysty
		if (amp == 0) {										//jezeli nie bylo &
			do {
				waitpid(pid, &status, WUNTRACED);			//czekaj na koniec potomka
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));  //dopoki potomek zakonczony normalne lub przez sygnal
		}

	}

	return 1;
}

FILE *fileOpen(char *argv) {
	char *c = malloc(sizeof(char) * BUFF_SIZE);				//alokacja pamieci
	strcpy(c, argv + 2);									//odrzucenie ./ od nazwy pliku
	FILE *fp;
	if ((fp = fopen(c, "r")) == NULL) {						//otwarcie pliku do czytania
		printf("can't open script\n");
		return NULL;
	} else
		return fp;
}

char *readFileLine(FILE *fp) {
	int bufferSize = BUFF_SIZE, i = 0, c;
	char *buffer = malloc(sizeof(char) * bufferSize);		//alokacja pamieci

	if (!buffer) {
		printf("can't allocate memory for buffer\n");
	}

	while (1) {
		c = fgetc(fp);										//czytanie pojedynczego znaku

		if (c == EOF)										//jezeli koniec pliku wyjdz z programu
			exit(0);

		if (c == '#') {										//jezeli linia jest komentarzem
			while (c != '\n')								//czytaj znaki az do konca linii
				c = fgetc(fp);
			c = fgetc(fp);
		}

		if (c == '\n') {									//jezeli koniec linii
			buffer[i] = '\0';								//dodaj znak konca slowa
			return buffer;
		} else if (c == EOF)								//jezeli koniec pliku
			exit(0);										//wyjdz z programu
		else {
			buffer[i] = c;									//dodawanie kolejnych znakow do bufora
		}
		i++;
	}

	if (i >= bufferSize) {									//realokacja bufora jezeli za maly
		bufferSize += BUFF_SIZE;
		buffer = realloc(buffer, bufferSize);
		if (!buffer) {
			printf("can't reallocate memory for buffer\n");
		}
	}
}

void saveHistory(char *line) {
	FILE *fp;
	if ((fp = fopen("MyShellHistory.txt", "a")) == NULL) {	//otworz plik do dopisywania historii
		printf("can't open history file\n");
		return;
	}
	fprintf(fp, "%s", line);								//zapisz linie do pliku
	fprintf(fp, "\n");										//dopisz koniec wiersza
	fclose(fp);												//zamknij plik
}

void printHistory(int signum) {
	FILE *fp;
	int i = 0;
	char c;

	if ((fp = fopen("MyShellHistory.txt", "r")) == NULL) {	//otworz plik do czytania
		printf("can't open history file\n");
		exit(1);
	}

	printf("\nHistoria operacji:\n");

	c = fgetc(fp);											//czytanie znakow z pliku
	while (i < 21) {										//wypisanie max 20 polecen
		printf("%c", c);									//pisanie znakow z pliku na stdout
		c = fgetc(fp);
		if (c == '\n')
			i++;
		if (c == EOF)
			exit(0);
	}
	exit(0);
}
