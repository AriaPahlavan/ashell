//Name: Aria Pahlavan
//Professor: Ramesh Yerraballi
//Date: 09/12/2016
//Course: Operating Systems EE641S
//EID: ap44342

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define TOKEN_DELIMITER " \t\r\n\a"
#define ARG_CAPACITY 10
char GREAT[] = ">";
char LESS[] = "<";
char TWOGREAT[] = "2>";
char AMPERSAND[] = "&";
char PIPE[] = "|";

enum FileType {
	INPUT,
	OUTPUT,
	ERROR
};


static void sig_hup(int signo) {
	kill(getpid(), SIGTERM);
}


static void sig_int(int signo) {
	printf("SIGINIT");
//	kill(getpid(), SIGCONT);
}

static void sig_tstp(int signo) {
	kill(getpid(), SIGTSTP);
}


/**
 * The most basic form of a command (i.e. cmd [arg1]*)
 */
struct BC {
	char **args;    //array of arguments
	int capArg;     //argument capacity
	int numArg;     //number of arguments

};
typedef struct BC BasicCommand;


/**
 * default constructor for Complex commands
 * @return
 */
BasicCommand *BasicCmdConst() {

	BasicCommand *newCmd = malloc(sizeof(BasicCommand));
	newCmd->capArg = ARG_CAPACITY;
	newCmd->args = malloc(sizeof(char *) * ARG_CAPACITY);
	newCmd->numArg = 0;
	return newCmd;
}


/**
 * default destructor for basic commands
 * @param bc
 */
void basicCmdDestruct(BasicCommand *bc) {
	free(bc->args);
	free(bc);
}


/** Adds new argument to the argument list*/
void addArg(BasicCommand *bc, char *newArg) {

	int num = bc->numArg;

	//if the capacity is exceeded, then reallocate
	if (num >= bc->capArg) {
		bc->args = realloc(bc->args, bc->capArg + sizeof(char *) * ARG_CAPACITY);
	}

	//add the new argument to the argument list
	bc->args[num] = newArg;

	//increment next arg index
	bc->numArg = num + 1;
}


/**
 * Complex commands containing basic commands
 */
struct CC {
	BasicCommand **basicCommandsList;
	int cap;
	int numBasicCommand;
	bool isBackground;         //background processes
	char *outputFile;     //file to contain outputFile
	char *inputFile;      //file to read inputFile from
	char *errorFile;      //file to contain errors

};
typedef struct CC CmplxCommand;


/**
 * default constructor for Complex commands
 * @return
 */
CmplxCommand *CmplxCmdConst() {

	CmplxCommand *newCmd = malloc(sizeof(CmplxCommand));
	newCmd->cap = ARG_CAPACITY;
	newCmd->numBasicCommand = 0;
	newCmd->basicCommandsList = malloc(sizeof(BasicCommand) * ARG_CAPACITY);
	newCmd->inputFile = NULL;
	newCmd->outputFile = NULL;
	newCmd->errorFile = NULL;
	newCmd->isBackground = false;
	return newCmd;
}


/**
 * default destructor for Complex commands
 * @param cc
 */
void cmplxCmdDestruct(CmplxCommand *cc) {
	free(cc->basicCommandsList);
	free(cc->inputFile);
	free(cc->outputFile);
	free(cc->errorFile);
	free(cc);
}


/**
 * Adds new simple cmd to the argument list
 * @return newly allocated basic command ptr
 */
BasicCommand *addnCreateBasicCmd(CmplxCommand *cc, BasicCommand *newCmd) {

	int num = cc->numBasicCommand;

	//if the capacity is exceeded, then reallocate
	if (num >= cc->cap) {
		cc->basicCommandsList = realloc(cc->basicCommandsList, cc->cap + sizeof(BasicCommand) * ARG_CAPACITY);
	}

	//add the new command to the command list
	cc->basicCommandsList[num] = newCmd;

	//increment next arg index
	cc->numBasicCommand = num + 1;


	return BasicCmdConst();
}


/**
 * Adds new simple cmd to the argument list
 */
void addBasicCmd(CmplxCommand *cc, BasicCommand *newCmd) {

	int num = cc->numBasicCommand;

	//if the capacity is exceeded, then reallocate
	if (num >= cc->cap) {
		cc->basicCommandsList = realloc(cc->basicCommandsList, cc->cap + sizeof(BasicCommand) * ARG_CAPACITY);
	}

	//add the new command to the command list
	cc->basicCommandsList[num] = newCmd;

	//increment next arg index
	cc->numBasicCommand = num + 1;
}


/**
 * Add file name (according to its type) into the complex command
 * @param cmplxCmd
 * @param token
 */
void addFile(CmplxCommand *cmplxCmd, char *token, enum FileType type) {

	switch (type) {
		case INPUT:
			if (cmplxCmd->inputFile == NULL) {//if a file name already exists, ignore
				cmplxCmd->inputFile = malloc(sizeof(char) * 100);
				strcpy(cmplxCmd->inputFile, token);
			}
			break;

		case OUTPUT:
			if (cmplxCmd->outputFile == NULL) {//if a file name already exists, ignore
				cmplxCmd->outputFile = malloc(sizeof(char) * 100);
				strcpy(cmplxCmd->outputFile, token);
			}
			break;

		case ERROR:
			if (cmplxCmd->errorFile == NULL) {//if a file name already exists, ignore
				cmplxCmd->errorFile = malloc(sizeof(char) * 100);
				strcpy(cmplxCmd->errorFile, token);
			}
			break;

		default:
			return;
	}


}



/**
 * @param token
 * @return true if token is one of '>', '<', '2>' or '|'
 */
bool isRedirectionToken(char *token) {
	bool isRedirection = false;

	if (strcmp((const char *) token, GREAT) == 0
		|| strcmp((const char *) token, LESS) == 0
		|| strcmp((const char *) token, TWOGREAT) == 0
		|| strcmp((const char *) token, PIPE) == 0
		|| strcmp(&token[strlen(token) - 1], AMPERSAND) == 0
			) {

		isRedirection = true;

	}

	return isRedirection;
}




/**
 * process redirections based on type
 * @param cmplxCmd
 * @param token
 * @param nextToken
 */
char * processRedirection
		(CmplxCommand *cmplxCmd, BasicCommand* basicCommand, char *token, char *nextToken) {


	if (strcmp((const char *) token, GREAT) == 0) {

		addFile(cmplxCmd, nextToken, OUTPUT);
		return  strtok(NULL, TOKEN_DELIMITER);

	}
	else if (strcmp((const char *) token, LESS) == 0) {

		addFile(cmplxCmd, nextToken, INPUT);
		return strtok(NULL, TOKEN_DELIMITER);


	}
	else if (strcmp((const char *) token, TWOGREAT) == 0) {

		addFile(cmplxCmd, nextToken, ERROR);
		return strtok(NULL, TOKEN_DELIMITER);

	}
	else if (strcmp((const char *) token, PIPE) == 0) {
		return nextToken;
	}
	else if (strcmp((const char *) token, AMPERSAND) == 0) {

		//this process will be in background
		cmplxCmd->isBackground = true;
		return NULL;
	}
	else if (strcmp(&token[strlen(token) - 1], AMPERSAND) == 0) {

		//this process will be in background
		cmplxCmd->isBackground = true;
		token[strlen(token) - 1] = '\0';
		addArg(basicCommand, token);
		return NULL;


	}
}



/**
 * Parses the command into CmplexCommand tree
 */
CmplxCommand *parseCmd(char *line) {

	int num;
	CmplxCommand *cmplxCmd = CmplxCmdConst();
	BasicCommand *basicCmd = BasicCmdConst();

	// get the first token
	char *token = strtok(line, TOKEN_DELIMITER);


	// process tokens
	while (token != NULL) {

		char* nextToken = strtok(NULL, TOKEN_DELIMITER);

		if (isRedirectionToken(token)){// '>', '<', "2>", or '&'
			nextToken = processRedirection(cmplxCmd, basicCmd, token, nextToken);
		}
		else { //simple command

			addArg(basicCmd, token);


			if (nextToken != NULL && isRedirectionToken(nextToken))
				basicCmd = addnCreateBasicCmd(cmplxCmd, basicCmd);

		}

		token = nextToken;
	}

	if (basicCmd->numArg != 0) {
		addBasicCmd(cmplxCmd, basicCmd);
	}

	return cmplxCmd;

}



void initAllNegative(int *array, int size) {

	for (int i = 0; i < size; ++i) {
		array[i] = -2;
	}
}



/**
 * Method to execute the command line
 * which is a complex command, consisting
 * of one or more basic commands
 */
void exec_line(CmplxCommand *command) {
	int pipefd[2];
	int status = -2;
//	pid_t pid = -2;


	//int status;
	int num_commands = command->numBasicCommand;

	int *cpids = (int *) malloc(sizeof(int) * num_commands);

	initAllNegative(cpids, num_commands);

	//save std in and out in case we need them later
	int save_stdin = dup(0);
	int save_stdout = dup(1);

	int fd_in, fd_out;

	if (command->errorFile != NULL) {
		close(STDERR_FILENO);
		open(command->errorFile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
	}

	//check if custom inputFile file is requested
	if (command->inputFile == NULL) {
		//read from terminal
		//dup2(save_stdin, fd_in);
		fd_in = dup(save_stdin);
	} else {
		//read from file
		fd_in = open(command->inputFile, O_RDONLY);
	}


//	printf("%d\n", command->numBasicCommand);
	for (int i = 0; i < command->numBasicCommand; i += 1) { //only allow 2 basic commands


		dup2(fd_in, 0); //make stdin cpnnect to what fd_in is connected to
		close(fd_in);

		// if we're processing the last basic command
		// then we need to figure out where the outputFile needs to be redirected
		if (i == command->numBasicCommand - 1) {
			//check if custom outputFile file is requested
			if (command->outputFile != NULL) {
				fd_out = open(command->outputFile, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU); //open/creat outputFile file
			} else {
				//if not just use the terminal to outputFile data
				fd_out = dup(save_stdout);
			}
		}

			//Otherwise let's keep processing them small commands
		else {

			pipe(pipefd);

			//piping
			fd_out = pipefd[1];
			fd_in = pipefd[0];
		}

		dup2(fd_out, STDOUT_FILENO);
		close(fd_out);      //close unused outputFile

		//creating new process
		cpids[i] = fork();
		if (cpids[i] < 0) {        //fork failed
			fprintf(stderr, "Fork Failed");
			exit(EXIT_FAILURE);
		}
		if (cpids[i] == 0) {  //child

			//for first command
//			if (i == 1) {
//				setsid(); // child 1 creates a new session and a new group and becomes leader -
			//   group id is same as his pid: cpid1
//			} else {
			setpgid(0, cpids[0]); //next children join the group whose group id is same as first child's pid
//			}
//
			//execute one basic command
			execvp(command->basicCommandsList[i]->args[0], command->basicCommandsList[i]->args);

			//NOT REACHED

			fprintf(stderr, "Invalid command");

			for (int j = 0; j < command->basicCommandsList[i]->numArg; ++j) {
				fprintf(stderr, ": %s", command->basicCommandsList[i]->args[j]);
			}
			fprintf(stderr, "\n");
			exit(EXIT_FAILURE);
		}

	} //end loop



	dup2(save_stdin, 0);
	dup2(save_stdout, 1);
	close(save_stdin);
	close(save_stdout);

//	if (command->isBackground == false) {

//		int status;
	pid_t pid;

//		while (num_commands > 0) {
//			pid = wait(&status);
//			printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
//			--num_commands;  // TODO(pts): Remove pid from the pids array.
//		}
	while (num_commands > 0) {
		pid = waitpid(cpids[num_commands - 1], &status, WUNTRACED | WCONTINUED);

		//errorFile with wait
		if (pid == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}

		//child exited
		if (WIFEXITED(status)) {
			num_commands--;
		}
			//child killed by signal
		else if (WIFSIGNALED(status)) {
			num_commands--;
		}
			//when should they continue
		else if (WIFSTOPPED(status)) {
			for (int i = 0; i < 10000; ++i) {
				//TODO: what would go gere? :(
			}
			num_commands--;
			kill(pid, SIGCONT);
		}


	}
//	}


}

/**
 * reads the command line
 */
char *read_line() {
	char *line_entered = NULL;      //to contain the command line
	ssize_t size_line = 0;
	getline(&line_entered, &size_line, stdin);

	return line_entered;
}

/**
 * - Input the commands
 * - Parse
 * - Run
 */
int main(int argc, char *argv[]) {

	if (signal(SIGINT, sig_int) == SIG_ERR)
		printf("signal(SIGINT) errorFile");
	if (signal(SIGTSTP, sig_tstp) == SIG_ERR)
		printf("signal(SIGTSTP) errorFile");

	char *command_line;
	int pid[2];
	char buf[10];

	CmplxCommand *cmplxCmd;

	while (!feof(stdin)) {
		printf("# ");

		command_line = read_line();


		cmplxCmd = parseCmd(command_line);

		exec_line(cmplxCmd);

		cmplxCmdDestruct(cmplxCmd);
	}
	return 0;
}

