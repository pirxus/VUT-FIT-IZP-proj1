//------------------------------------//
// NAME: projekt 1 - práce s textem
// VUT FIT BIT 2018/19 - IZP
// AUTHOR: Šimon Sedláček, xsedla1h@stud.fit.vutbr.cz
//
// Jednoduchý textový editor
// -----------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

//Důležité konstanty
#define LINE_LIMIT 1002 //konstanta pro nastavení limitu délky řádku vstupního textu
#define LINE_LIMIT_COMMANDS 1003 //konstanta pro nastavení limitu délky řádku příkazu
#define COMMAND_LIMIT 101 //konstanta pro nastavení limitu počtu příkazů

//Návratové kódy
#define NO_LINES_LEFT -2 //indikuje konec streamu vstupního souboru
#define QUIT_COMMAND -1 //indikuje spuštění příkazu quit
#define SUCCESS 0 //indikuje v pořádku proběhlou operaci
#define	ERR_COMMAND_SYNTAX 1
#define ERR_BUFFER_OVERFLOW 2
#define ERR_INVALID_FILE 3
#define ERR_LINE_LIMIT_EXCEEDED 4

//Prototyp funkce, která řídí průběh programu
int determineCommand(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndex, bool *modified);


//Prototypy funkcí, které implementují příkazy editoru
int insertContent(char *commandLine);
int beforeContent(char *commandLine, char *inputLine, bool *modified);
int appendContent(char *commandLine, char *inputLine, bool *modified);
int deleteCurrentLine(char *commandLine, char *inputLine, bool *modified);
int removeEndOfLineCharacter(char *commandLine, char *inputLine, bool *modified);
int patternReplace(char *commandLine, char *inputLine, bool *modified);
int patternReplaceAll(char *commandLine, char *inputLine, bool *modified);
int nextLine(char *commandLine, char *inputLine, bool *modified);
int quit(char *commandLine, char *inputLine, bool *modified);
int goToCommand(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndexPointer);


//Prototypy nepovinných funkcí
int appendEol(char *commandLine, char *inputLine, bool *modified);
int findPattern(char *commandLine, char *inputLine, bool *modified);
int conditionedGoTo(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndexPointer);


//Prototypy pomocných funkcí
int loadAllCommands(FILE *commandFile, char commands[][LINE_LIMIT_COMMANDS]);
int printAndReadInputLine(char *inputLine, bool *modified, bool printCondition);
int extractNumberFromCommandLine(char *commandLine, char *extractedDigits, int relevantDigitCount);
void clearLine(char line[]);
void printOutRemainingText(char *inputLine, bool *modified);
bool checkForCmdSequence(char *commandLine, int *commandArrayIndexPointer);
void ifMissingPrintEOL(char *commandLine, int *commandArrayIndexPointer);
int getPatternReplacement(char *commandLine, char *pattern, char *replacement);
void readMe();


/***********************************************************/
/***********************************************************/

 
int main(int argc, char *argv[]) {
 	//kontrola počtu vstupních argumentů
	if(argc != 2) {
		readMe();
		return SUCCESS;
	}

 	//soubor s příkazy
	FILE *commandFile;
	if((commandFile = fopen(argv[1], "r")) == NULL) {
		perror(strerror(errno));
		return ERR_INVALID_FILE;
	}

	//2d pole pro uložení příkazů z příkazového soubrou
	char commands[COMMAND_LIMIT][LINE_LIMIT_COMMANDS] = {{0}}; 
	//pole pro aktuálně zpracovávaný řádek
	char inputLine[LINE_LIMIT] = {0};
	//pole, díky jehož obsahu funkce GoTo detekuje zacyklení 
	char inputLineHistory[LINE_LIMIT] = {0};
	//indikuje, zda byl řádek modifikován
	bool modified = false;
 	//index pro pohyb v příkazovém poli
	int commandArrayIndex = 0;

 	//načte příkazy ze souboru
	if(loadAllCommands(commandFile, commands) == ERR_LINE_LIMIT_EXCEEDED) {
		fprintf(stderr, "Err: Command was too long.\n");
		return ERR_LINE_LIMIT_EXCEEDED;
	}
	fclose(commandFile);

	//načte první řádek ze stdin
	printAndReadInputLine(inputLine, &modified, false);

	while(commands[commandArrayIndex][0] != 0) { //probíhá dokud jsou v poli příkazy
 		//na základě vrácené hodnoty určí další postup programu
		switch(determineCommand(commands[commandArrayIndex], inputLine, inputLineHistory, &commandArrayIndex, &modified)) {
			case SUCCESS:
				break;
			case QUIT_COMMAND:
				ifMissingPrintEOL(commands[commandArrayIndex], &commandArrayIndex);
				return SUCCESS;
			case NO_LINES_LEFT:
				ifMissingPrintEOL(commands[commandArrayIndex], &commandArrayIndex);
				return SUCCESS;
			case ERR_COMMAND_SYNTAX:
				ifMissingPrintEOL(commands[commandArrayIndex], &commandArrayIndex);
				return ERR_COMMAND_SYNTAX;
			case ERR_BUFFER_OVERFLOW:
				ifMissingPrintEOL(commands[commandArrayIndex], &commandArrayIndex);
				return ERR_BUFFER_OVERFLOW;
			case ERR_LINE_LIMIT_EXCEEDED:
				ifMissingPrintEOL(commands[commandArrayIndex], &commandArrayIndex);
				return ERR_LINE_LIMIT_EXCEEDED;
			default:
				perror(strerror(errno));		
				return SUCCESS;
		}
		commandArrayIndex++;
	}

	//pokud došly příkazy, zkontroluje, zda je na vstupu ještě nějaký text a vypíše ho
	printOutRemainingText(inputLine, &modified);
	return SUCCESS;
}

//Funkce řídící průběh programu

int determineCommand(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndexPointer, bool *modified) {
	//rozhoduje, jaký příkaz bude proveden a vrací příslušné hodnoty
	switch(commandLine[0]) {
	case 'i':
		return insertContent(commandLine);
	case 'b':
		return beforeContent(commandLine, inputLine, modified);
	case 'a':
		return appendContent(commandLine, inputLine, modified);
	case 'd':
		return deleteCurrentLine(commandLine, inputLine, modified);
	case 'r':
		return removeEndOfLineCharacter(commandLine, inputLine, modified);
	case 's':
		return patternReplace(commandLine, inputLine, modified);
	case 'S':
		return patternReplaceAll(commandLine, inputLine, modified);
	case 'n':
		return nextLine(commandLine, inputLine, modified);
	case 'q':
		return quit(commandLine, inputLine, modified);
	case 'g':
		return goToCommand(commandLine, inputLine, inputLineHistory, commandArrayIndexPointer);
	case 'e':
		return appendEol(commandLine, inputLine, modified);
	case 'f':
		return findPattern(commandLine, inputLine, modified);
	case 'c':
		return conditionedGoTo(commandLine, inputLine, inputLineHistory, commandArrayIndexPointer); 
	default:
		fprintf(stderr, "Err: Unknown command.\n");
		return ERR_COMMAND_SYNTAX;
	}
}


//Funkce implementující příkazy editoru

//na nový řádek vytiskne řetězec content
int insertContent(char *commandLine) {
	fprintf(stdout,"%s", &commandLine[1]);
	return SUCCESS;
}

//vloží řetězec content na začátek aktuální řádky
int beforeContent(char *commandLine, char *inputLine, bool *modified) {
	//dočasné pole pro manipulaci při kopírování obsahu
	char lineCopy[LINE_LIMIT] = {0};
	int contentLength = strlen(commandLine) - 1;

	if(LINE_LIMIT < (strlen(inputLine) + contentLength)) {
		fprintf(stderr, "Err: Line exceeded the line length limit.\n");
		return ERR_BUFFER_OVERFLOW;
	}

	strcpy(lineCopy, &commandLine[1]);
	strcpy(&lineCopy[contentLength - 1], inputLine);
	strcpy(inputLine, lineCopy);
	*modified = true;
	return SUCCESS;
}

//připojí řetězec content na konec aktuální řádky
int appendContent(char *commandLine, char *inputLine, bool *modified) {
	int inputLength = strlen(inputLine);
	
	if(LINE_LIMIT <= (inputLength + strlen(commandLine) - 1)) {
		fprintf(stderr, "Err: Line exceeded the line length limit.\n");
		return ERR_BUFFER_OVERFLOW;
	}

	//zaručuje, že se obsah CONTENT zkopíruje vždy přes newline nebo na první nulový byte
	int offset = -1;
	if(inputLine[inputLength + offset] != '\n') {
		offset = 0;
	}
	strcpy(&inputLine[inputLength + offset], &commandLine[1]);
	*modified = true;
	return SUCCESS;
}
 
//smaže 1 až n řádků
int deleteCurrentLine(char *commandLine, char *inputLine, bool *modified) {
	//pole pro uložení případného čísla z příkazu
	char extractedDigits[9] = {0};
	//tato funkce extrahuje z příkazu číslo pro případné použití
	int iterations = extractNumberFromCommandLine(commandLine, extractedDigits, 8);

	switch(iterations) {
		case -1:
			fprintf(stderr, "Err: Invalid 'd' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		case -2:
			//v příkazu žádné číslo nebylo
			return printAndReadInputLine(inputLine, modified, false);
		case -3:
			fprintf(stderr, "Err: Invalid 'd' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		default:
			if(iterations == 0) {
				return printAndReadInputLine(inputLine, modified, false);
			} else {
				//opakování načtení nových řádků podle hodnoty iterations
				for(int i = 0; i < iterations; i++) {
					switch(printAndReadInputLine(inputLine, modified, false)) {
						case NO_LINES_LEFT:
							return NO_LINES_LEFT;
						case ERR_LINE_LIMIT_EXCEEDED:
							return ERR_LINE_LIMIT_EXCEEDED;
						default:
							break;
					}
				}
			}
			break;
	}
	return SUCCESS;

}

//na aktuálním řádku smaže EOL
int removeEndOfLineCharacter(char *commandLine, char *inputLine, bool *modified) {
	if(commandLine[1] != '\n') {
		fprintf(stderr, "Err: Invalid 'r' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}

	int lineLength = strlen(inputLine);
	//projde celý řádek od konce a pokud narazí na newline, přepíše ho na nulový byte
	for(int i = lineLength - 1; i >= 0; i++) {
		if(inputLine[i] == '\n') {
			*modified = true;
			inputLine[i] = 0;
			break;
		}
	}
	return SUCCESS;
}

//nahradí první výskyt řetězce pattern za řetězec replacement
int patternReplace(char *commandLine, char *inputLine, bool *modified) {
	char pattern[LINE_LIMIT] = {0};
	char replacement[LINE_LIMIT] = {0};

	//načte pattern a replacement řetězce
	if(getPatternReplacement(commandLine, pattern, replacement) == ERR_COMMAND_SYNTAX) {
		fprintf(stderr, "Err: Invalid 's' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}
	
	//pomocný řetězec pro kopírování
	char tempLine[LINE_LIMIT] = {0};
	//pointer pro uložení adresy výskytu řetězce pattern na vstupní řádce
	char *substringPtr;

	if((substringPtr = strstr(inputLine, pattern)) == NULL) {
		return SUCCESS;
	}

	if(LINE_LIMIT <= strlen(inputLine) - strlen(pattern) + strlen(replacement)) {
		fprintf(stderr, "Err: Line exceeded the line length limit.\n");
		return ERR_BUFFER_OVERFLOW;
	}

	strcpy(tempLine, substringPtr + strlen(pattern));
	strcpy(substringPtr, replacement);
	strcpy(substringPtr + strlen(replacement), tempLine);
	*modified = true;
	return SUCCESS;
}

//nahradí všechny výskyty řetězce pattern za řetězec replacement
int patternReplaceAll(char *commandLine, char *inputLine, bool *modified) {
	char pattern[LINE_LIMIT] = {0};
	char replacement[LINE_LIMIT] = {0};
	
	if(getPatternReplacement(commandLine, pattern, replacement) == ERR_COMMAND_SYNTAX) {
		fprintf(stderr, "Err: Invalid 'S' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}

	char tempLine[LINE_LIMIT] = {0};
	char *substringPtr;

	//postupně nahradí všechny výskyty pattern na řádku
	while((substringPtr = strstr(inputLine, pattern)) != NULL) {
		if(LINE_LIMIT <= strlen(inputLine) - strlen(pattern) + strlen(replacement)) {
			fprintf(stderr, "Err: Line exceeded the line length limit.\n");
			return ERR_BUFFER_OVERFLOW;
		}
		strcpy(tempLine, substringPtr + strlen(pattern));
		strcpy(substringPtr, replacement);
		strcpy(substringPtr + strlen(replacement), tempLine);
	}
	*modified = true;
	return SUCCESS;
}

/*vytiskne 1 až n aktuálních řádků, vždy se pokusí načíst další.
 *Pokud žádný text nezbývá, ukončí program.*/
int nextLine(char *commandLine, char *inputLine, bool *modified) {
	char extractedDigits[9] = {0};
	//určí, zda newLine proběhne jednou nebo s více iteracemi
	int iterations = extractNumberFromCommandLine(commandLine, extractedDigits, 8);

	switch(iterations) {
		case -1:
			fprintf(stderr, "Err: Invalid 'n' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		case -2:
			return printAndReadInputLine(inputLine, modified, true);
		case -3:
			fprintf(stderr, "Err: Invalid 'n' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		default:
			if(iterations == 0) {
				return printAndReadInputLine(inputLine, modified, true);
			} else {
				for(int i = 0; i < iterations; i++) {
					switch(printAndReadInputLine(inputLine, modified, true)) {
						case NO_LINES_LEFT:
							return NO_LINES_LEFT;
						case ERR_LINE_LIMIT_EXCEEDED:
							return ERR_LINE_LIMIT_EXCEEDED;
						default:
							break;
					}
				}
			}
			break;
	}
	return SUCCESS;
}

//vrací hodnotu, která indikuje ukončení programu
int quit(char *commandLine, char *inputLine, bool *modified) {
	if(commandLine[1] != '\n') {
		fprintf(stderr, "Err: Invalid 'q' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}
	if(*modified == true) {
		fprintf(stdout, "%s", inputLine);
		/*přepsání příkazu na 'n' je důležité pro funkci, která kontroluje,
		zda je za posledním řádkem na výstupu znak EOL.*/
		commandLine[0] = 'n';
	}
	return QUIT_COMMAND;
}

//přeskočí na n-tý příkaz
int goToCommand(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndexPointer) {
	char extractedDigits[3] = {0};
	int jumpIndex = extractNumberFromCommandLine(commandLine, extractedDigits, 2);

	switch(jumpIndex) {
		case -1:
			fprintf(stderr, "Err: Invalid 'g' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		case -2:
			fprintf(stderr, "Err: Invalid 'g' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		case -3:
			fprintf(stderr, "Err: Invalid 'g' command syntax.\n");
			return ERR_COMMAND_SYNTAX;
		default:
			if(jumpIndex > 100) {
				fprintf(stderr, "Err: GoTo cannot jump beyond the command limit.\n");
				return ERR_COMMAND_SYNTAX;
			} else if(jumpIndex - 1 == *commandArrayIndexPointer) {
				fprintf(stderr, "Err: GoTo command cannot jump on itself.\n");
				return ERR_COMMAND_SYNTAX;
			}
				
			/*detekce zacyklení GoTo - zahrnuje případy, kdy program nikdy neskončí,
			 * tj. nevyskytne se ani chyba jako line overflow*/
			if(jumpIndex < *commandArrayIndexPointer + 1) {
				if(strcmp(inputLine, inputLineHistory) == 0) {
					fprintf(stderr, "Err: Detected cycle caused by GoTo commmand.\n");
					return ERR_COMMAND_SYNTAX;
				}
			}
			strcpy(inputLineHistory, inputLine);

			*commandArrayIndexPointer = jumpIndex - 2;
			break;
	}
	return SUCCESS;
}


//Nepovinné funkce

//přidá na konec řádku EOL, pokud chybí
int appendEol(char *commandLine, char *inputLine, bool *modified) {
	if(commandLine[1] != '\n') {
		fprintf(stderr, "Err: Invalid 'e' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}

	if(strlen(inputLine) < LINE_LIMIT - 1) {
		inputLine[strlen(inputLine)] = '\n';
	} else {
		fprintf(stderr, "Err: Line exceeded the line length limit.\n");
		return ERR_BUFFER_OVERFLOW;
	}
	*modified = true;
	return SUCCESS;
}

//posune aktuální řádek na řádek obsahující řetězec pattern
int findPattern(char *commandLine, char *inputLine, bool *modified) {
	char pattern[LINE_LIMIT] = {0};
	int returnValue = 0;

	//načte pattern bez znaku EOL
	strncpy(pattern, &commandLine[1], strlen(commandLine) - 2);

	while(strstr(inputLine, pattern) == NULL && returnValue != NO_LINES_LEFT && returnValue != ERR_LINE_LIMIT_EXCEEDED) {
		returnValue = printAndReadInputLine(inputLine, modified, true);
	}
	return returnValue;
}

//pokud aktuální řádek obsahuje řetězec pattern, přeskočí na x-tý příkaz
int conditionedGoTo(char *commandLine, char *inputLine, char *inputLineHistory, int *commandArrayIndexPointer) {
	char extractedDigits[3] = {0};
	char pattern[LINE_LIMIT] = {0};
	int jumpIndex = 0;

	//ověří formát příkazu, načte pattern a čísla 
	if(commandLine[2] == ' ' && isdigit(commandLine[1])) {
		extractedDigits[0] = commandLine[1];
		strncpy(pattern, &commandLine[3], strlen(commandLine) - 4);	
	} else if(commandLine[3] == ' ' && isdigit(commandLine[1]) && isdigit(commandLine[2])) {
		extractedDigits[0] = commandLine[1];
		extractedDigits[1] = commandLine[2];
		strncpy(pattern, &commandLine[4], strlen(commandLine) - 5);
	} else {
		printf("suh\n");
		fprintf(stderr, "Err: Invalid 'c' command syntax.\n");
		return ERR_COMMAND_SYNTAX;
	}
	
	sscanf(extractedDigits, "%d", &jumpIndex);

	if(strstr(inputLine, pattern) != NULL) {
		if(jumpIndex == 0) {
			fprintf(stderr, "Err: cGoTo - no command with index 0.\n");
			return ERR_COMMAND_SYNTAX;
		}
		if(jumpIndex - 1 == *commandArrayIndexPointer) {
			fprintf(stderr, "Err: cGoTo cannot jump on itself.\n");
			return ERR_COMMAND_SYNTAX;
		}
		if(jumpIndex > 100) {
			fprintf(stderr, "Err: cGoTo cannot jump beyond the command limit.\n");
			return ERR_COMMAND_SYNTAX;
		}

		/*detekce zacyklení cGoTo - zahrnuje případy, kdy program nikdy neskončí,
		 * tj. nevyskytne se ani chyba jako line overflow*/
		if(jumpIndex < *commandArrayIndexPointer + 1) {
			if(strcmp(inputLine, inputLineHistory) == 0) {
				fprintf(stderr, "Err: Detected cycle caused by cGoTo commmand.\n");
				return ERR_COMMAND_SYNTAX;
			}
		}
		strcpy(inputLineHistory, inputLine);

		*commandArrayIndexPointer = jumpIndex - 2;
	}
	return SUCCESS;
}


//Další pomocné funkce

//načte všechny příkazy z příkazového souboru do 2d pole
int loadAllCommands(FILE *commandFile, char commands[][LINE_LIMIT_COMMANDS]) {
	for(int i = 0; fgets(commands[i], LINE_LIMIT_COMMANDS - 1, commandFile) != NULL && i < COMMAND_LIMIT - 1; i++) {
		if(commands[i][LINE_LIMIT_COMMANDS - 2] != '\n' && commands[i][LINE_LIMIT_COMMANDS - 1] != 0) {
			return ERR_LINE_LIMIT_EXCEEDED;
		}
	}
	return SUCCESS;
}

//podle hodnoty printCondition vytiskne aktuální řádek a pokusí se načíst další
int printAndReadInputLine(char *inputLine, bool *modified, bool printCondition) {
	if(printCondition == true)
		fprintf(stdout, "%s", inputLine);

	clearLine(inputLine);
	fgets(inputLine, LINE_LIMIT, stdin);

	if(inputLine[0] == 0)
		return NO_LINES_LEFT;

	if(inputLine[LINE_LIMIT - 2] != 0 && inputLine[LINE_LIMIT - 2] != '\n') {
		fprintf(stderr, "Err: An input line was longer than 1000 characters.\n");
		return ERR_LINE_LIMIT_EXCEEDED;
	}
	*modified = false;
	return SUCCESS;
}

//extrahuje z řádky příkazu číslo indikující pozici nebo počet iterací dané funkce
int extractNumberFromCommandLine(char *commandLine, char *extractedDigits, int relevantDigitCount) {
	int extractedNumber = 0;
	int commandLineLength = strlen(commandLine);

	//relevantDigitCount indikuje, kolikaciferné číslo daná funkce očekává
	if(commandLineLength - 2  > relevantDigitCount) {
		//zajistí chybové hlášení při větším počtu cifer, než je relevantDigitCount
		return -1;
	}

	if(commandLine[1] == '\n' || commandLine[1] == 0) {
		//indikuje absenci čísla v příkazu
		return -2;
	}

	int i = 1;
	while(i < commandLineLength - 1) {
		if(isdigit(commandLine[i]) != 0) {
			extractedDigits[i - 1] = commandLine[i];
			i++;
		} else {
			//indikuje neplatný znak
			return -3;
		}
	}

	sscanf(extractedDigits, "%d", &extractedNumber);
	return extractedNumber;
}

//smaže obsah daného pole 
void clearLine(char *line) {
	int lineLength = strlen(line);
	
	for(int i = 0; i<=lineLength; i++) {
		line[i] = 0;
	}
}

//zajišťuje vytisknutí zbývajícího textu ze stdin
void printOutRemainingText(char *inputLine, bool* modified) {
	while(printAndReadInputLine(inputLine, modified, true) != NO_LINES_LEFT) {
	}
}

//hledá v příkazech nebezpečnou posloupnost, která způsobí opomenutí EOL na posledním řádku
bool checkForCmdSequence(char *commandLine, int *commandArrayIndexPointer) {
	for(int i = 0; i <= *commandArrayIndexPointer; i++) {
		//prochází první sloupce v daném řádku
		switch(*(commandLine - (LINE_LIMIT_COMMANDS * i))) {
			case 'q':
				break;
			case 'i':
				return false;
			case 'e':
				return false;
			case 'n':		
				if(isdigit(*(commandLine - (LINE_LIMIT * i))) != 0) {
					return false;
				}
				i++;
				for(i = i; i <= *commandArrayIndexPointer; i++) {
					switch(*(commandLine -(LINE_LIMIT_COMMANDS * i))) {
						case 'r':
							return true;
						case 'a':
							return false;
						case 'e':
							return false;
						case 'n':
							return false;
						case 'd':
							return false;
						default:
							break;
					}
				}
			default:
				break;
		}
	}
	return false;
}

//zajistí newline character za za posledním řádkem na stdout
void ifMissingPrintEOL(char *commandLine, int *commandArrayIndexPointer) {
	if(checkForCmdSequence(commandLine, commandArrayIndexPointer) == true)
		printf("\n");
}

//nahraje z commandLine řetězce pattern a replacement
int getPatternReplacement(char *commandLine, char *pattern, char *replacement) {
	if(commandLine[1] == '\n' || commandLine[1] == commandLine[2]) {
		return ERR_COMMAND_SYNTAX;
	}

	//oddělovací znak
	char separator = commandLine[1];
	int separatorCount = 1;
	int commandLength = strlen(commandLine) - 1;
	//sekundární index pro cyklus
	int j = 0;
	
	//zkontroluje počet oddělovacích znaků a načte pattern a replacement
	for(int i = 2; i < commandLength; i++) {
		if(separator == commandLine[i]) {
			separatorCount++;
			j = 0;
			continue;
		}

		if(separatorCount < 2)
			pattern[j] = commandLine[i];
		else
			replacement[j] = commandLine[i];
		j++;
	}

	if(separatorCount != 2) {
		return ERR_COMMAND_SYNTAX;
	}
	return SUCCESS;
}

//nápověda
void readMe() {
	printf("##	**********************************NÁPOVĚDA**********************************  ##\n");
	printf("##	Tento program slouží k editaci textu na standardním vstupu. Příkazy pro       ##\n");
	printf("##	editor a text k editaci uložte do dvou textových souborů. Syntaxe spuštění    ##\n");
	printf("##	je pak následující:                                                           ##\n");
	printf("##                                                                                ##\n");
	printf("##	./proj1 cmd.txt <input.txt                                                    ##\n");
	printf("##                                                                                ##\n");
	printf("##	Každý příkaz musí mít svůj vlastní řádek. Maximální délka jedné řádky textu   ##\n");
	printf("##	je 1000 znaků, maximální počet příkazů je 100.                                ##\n");
	printf("##	****************************************************************************  ##\n");
}
