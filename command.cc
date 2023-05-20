
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "command.h"
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1)
	{
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **)realloc(_arguments,
									  _numberOfAvailableArguments * sizeof(char *));
	}

	_arguments[_numberOfArguments] = argument;

	// Add NULL argument at the end
	_arguments[_numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_numberOfSimpleCommands = 0;
	_simpleCommands = (SimpleCommand **)
		malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_great = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands + 1)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
													_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}

	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}
	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}
	if (_errFile != _outFile)
	{

		if (_errFile)
		{
			free(_errFile);
		}
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}
void handler(int sig)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	pid_t chpid = wait(NULL);
	FILE *fp;
	fp = fopen("data.log", "a");
	fprintf(fp, "%d is terminated at %s \n", chpid, asctime(timeinfo));
	fclose(fp);
}
void Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i + 1);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
	}

	printf("\n\n");
	printf("  Output       Input        Error        Background\n");
	printf("  ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
		   _background ? "YES" : "NO");
	printf("\n\n");
}

void Command::execute()
{
	// Don't do anything if there are no simple commands
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}
	if (strcmp("exit", _simpleCommands[0]->_arguments[0]) == 0)
	{
		printf("Good bye!! \n");
		exit(2);
	}
	else
	{
		// Print contents of Command data structure
		print();
	}

	int defaultin = dup(0);	 // Default file Descriptor for stdin
	int defaultout = dup(1); // Default file Descriptor for stdout
	int defaulterr = dup(2); // Default file Descriptor for stderr
	int fdpipe[2 * (_numberOfSimpleCommands - 1)];
	int j = 0;
	int k = 1;
	int pid;
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{

		if (strcmp("cd", _simpleCommands[i]->_arguments[0]) == 0)
		{
			if (_simpleCommands[i]->_arguments[1] == NULL)
			{
				chdir("/home");
			}
			else
			{
				chdir(_simpleCommands[i]->_arguments[1]);
			}
		}
		if (strcmp("echo", _simpleCommands[i]->_arguments[0]) == 0 && (strstr(_simpleCommands[i]->_arguments[1], "*") != NULL || strstr(_simpleCommands[i]->_arguments[1], "?") != NULL))
		{

			char **found;
			glob_t gstruct;
			int r;

			r = glob(_simpleCommands[i]->_arguments[1], GLOB_ERR, NULL, &gstruct);
			/* check for errors */
			if (r != 0)
			{
				if (r == GLOB_NOMATCH)
					fprintf(stderr, "No matches\n");
				else{
					fprintf(stderr, "Some kinda glob error\n");
				    exit(1);}
			}
            if (gstruct.gl_pathc != 0){
			/* success, output found filenames */
			printf("Found %zu filename matches\n", gstruct.gl_pathc);
			found = gstruct.gl_pathv;
			while (*found)
			{
				printf("%s\n", *found);
				found++;
			}
			}
		}
		if (pipe(fdpipe + i * 2) == -1)
		{
			perror("pipe failed");
			exit(2);
		}
		if (i == 0)
		{
			if (_inputFile != 0)
			{
				int infd = open(_inputFile, O_RDONLY, 0666);
				dup2(infd, 0);
				close(infd);
				_inputFile = 0;
			}
			else
			{
				dup2(defaultin, 0);
			}
		}
		else
		{
			dup2(fdpipe[j], 0);
			j += 2;
		}
		if (i + 1 != _numberOfSimpleCommands)
		{
			dup2(fdpipe[k], 1);
			k += 2;

			// Redirect err (use stderr)
			dup2(defaulterr, 2);

			// Create new process for "cat"
			signal(SIGCHLD, handler);
			pid = fork();

			if (pid == -1)
			{
				perror("can't creat child\n");
				exit(2);
			}
			if (pid == 0)
			{
				// Child
				for (int x = 0; x < 2 * (_numberOfSimpleCommands - 1); x++)
				{
					close(fdpipe[x]);
				} // close file descriptors that are not needed

				close(defaultin);
				close(defaultout);
				close(defaulterr);

				// You can use execvp() instead if the arguments are stored in an array
				int exe = execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				if (exe == -1)
				{
					printf("command not found\n");
					exit(2);
				}
				// exec() is not suppose to return, something went wrong
				perror("cat_grep: exec cat");
				exit(2);
			}
		}
		else
		{

			if (_outFile != 0)
			{
                
				if (_great == 0)
				{
                    
					int outfd = creat(_outFile, 0666);
					dup2(outfd, 1);
					
					// Redirect err (use stderr)
					dup2(defaulterr, 2);
				}
				else if (_great == 1)
				{
                   
					int outfd = open(_outFile, O_APPEND | O_WRONLY | O_CREAT, 0666);
					dup2(outfd, 1);
					
					// Redirect err (use stderr)
					dup2(defaulterr, 2);
				}
				else if (_great == 2)
				{
                    
					int outfd = open(_outFile, O_APPEND | O_WRONLY | O_CREAT, 0666);
					dup2(outfd, 1);
					dup2(outfd, 2);
					
					close(outfd);
				}
			}
			else
			{
				dup2(defaultout, 1);
			}

			// Add execution here

			if (_inputFile != 0)
			{
				int infd = open(_inputFile, O_RDONLY, 0666);
				dup2(infd, 0);
				close(infd);
				// Redirect err
				dup2(defaulterr, 2);
			}

			signal(SIGCHLD, handler);
			pid = fork();
			if (pid == -1)
			{
				perror("ls: fork\n");
				exit(2);
			}

			if (pid == 0)
			{
				for (int x = 0; x < 2 * (_numberOfSimpleCommands - 1); x++)
				{
					close(fdpipe[x]);
				}
				close(defaultin);
				close(defaultout);
				close(defaulterr);

				int exe = execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				if (exe == -1)
				{
					printf("command not found\n");
					exit(2);
				}
			}

			dup2(defaultin, 0);
			dup2(defaultout, 1);
			dup2(defaulterr, 2);
			for (int x = 0; x < 2 * (_numberOfSimpleCommands - 1); x++)
			{
				close(fdpipe[x]);
			}
			close(defaultin);
			close(defaultout);
			close(defaulterr);

			if (_background == 0)
			{

				waitpid(pid, 0, 0);
			}
		}
	}

	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse(void);

void sigintHandler(int sig_num)
{
	signal(SIGINT, sigintHandler);
	fflush(stdout);
}

// void setup_term(void) {
//   struct termios t;
// tcgetattr(0, &t);
// t.c_lflag &= ~ECHOCTL;
// tcsetattr(0, TCSANOW, &t);
//}

void send_signal(int pid)
{
	int ret;
	ret = kill(pid, SIGHUP);
	printf("ret : %d", ret);
}

int main()
{

	Command::_currentCommand.prompt();

	// signal(SIGCHLD, handler);
	signal(SIGINT, sigintHandler);
	// setup_term();
	// getchar();

	yyparse();

	return 0;
}
