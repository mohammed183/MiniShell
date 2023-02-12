#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h> 
#include <errno.h> 
#include <dirent.h>
#include <regex.h>
#include "command.h"

void writeLog(int pid);

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_out_new = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	// Check if there is space to enter new command
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void SimpleCommand::wildcardArg(char *arg){
 	if( strchr(arg, '*') == NULL && strchr(arg, '?') == NULL ){
		Command::_currentSimpleCommand->insertArgument(arg);
		return;
	}
	char *reg = (char *)malloc(2*strlen(arg)+10);
	char *a = arg;
	char *r = reg;
	*r = '^'; r++;
	while(*a){
		if(*a == '*'){
			*r = '.'; r++;
			*r = '*'; r++;
		}else if (*a == '?'){
			*r = '.'; r++;
		}else if (*a == '.'){
			*r = '\\'; r++;
			*r = '.'; r++;
		}else{
			*r = *a; r++;
		}
		a++;
	}
	*r='$'; r++; *r=0;
	regex_t re;	
	int res = regcomp( &re, reg,  REG_EXTENDED | REG_NOSUB);
	if (res!=0) {
		perror("cannot create regex");
		return;
	}
	DIR* dir = opendir("."); // "." is the current directory
	if (dir == NULL) {
		perror("opendir");
		return;
		}
	struct dirent* ent;
	while ( (ent = readdir(dir))!= NULL) {
		regmatch_t match;
		 if (regexec(&re , ent-> d_name,1, &match,0 ) ==0 ) {
			 Command::_currentSimpleCommand->insertArgument(strdup(ent-> d_name));
 		}
 	}
 	closedir(dir);
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}


	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_out_new = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------");
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("\n  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void bye(){
	printf("\n\nGOODBYE !!!\n\n");	
	exit ( 0 );
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	
	if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
		bye();
	}
	if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
		char path[300];
		getcwd(path, 300);
		if ( _simpleCommands[0]->_numberOfArguments > 1) {
			strcat(path,"/");
			strcat(path, _simpleCommands[0]->_arguments[1]);
			if ( chdir(path) != 0 ) {
				printf("FAILED TO CHANGE DIRECTORY: %s\n", path);
			}
		}
		else {
			char* home = getenv("HOME");
			char* rst = strchr(home,'\n');
			if ( rst != NULL ) *rst = '\0';
			if ( chdir( home ) != 0 ) {
				printf("FAILED TO CHANGE DIRECTORY /HOME");
			}
		}
		printf("cwd: %s\n", getcwd(path,300));
	}
	else {
		int defaultin = dup( 0 ); 	// Default file Descriptor for stdin
		int defaultout = dup( 1 ); 	// Default file Descriptor for stdout
		int defaulterr = dup( 2 ); 	// Default file Descriptor for stderr
		int inp;
		int out;
		int er;
		if (_inputFile != 0 ) {
			inp = open( _inputFile, O_RDONLY | O_CREAT, 0666);
		}
		else {
			inp = dup( defaultin );
		}
		
		if ( _errFile != 0 ) {
			if ( _out_new ) {
				er = open( _errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			}
			else {
				er = open( _errFile, O_APPEND | O_WRONLY | O_CREAT, 0666); 
			}
			
		}
		else {
			er = dup( defaulterr );
		}
		dup2(er, 2);
		
		int pid;
		for (int i = 0; i < _numberOfSimpleCommands; i++) {
			dup2( inp, 0 );		// Redirect input 
			close ( inp );
			if ( i == _numberOfSimpleCommands -1 ) {
			// if last command direct to output file if exists
				if (_outFile != 0 ) { 	// Redirect Output
					if ( _out_new ) 
						out = open( _outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
					else
						out = open( _outFile, O_APPEND | O_WRONLY | O_CREAT, 0666); 	
				}
				else {
					out = dup( defaultout );
				}
			}
			else { // Direct to next command in pipeline
				int fdpipe[2];
				pipe( fdpipe );
				inp = fdpipe[0];
				out = fdpipe[1];
			}
			dup2( out, 1 );
			close( out );
			
			pid = fork();
			if ( pid == 0 ) {
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("EXECVP DIDN'T RUN CORRECTLY\n");
				exit( 0 );
			}
			else {
				writeLog(pid);
			}
		}
		close( inp );
		close( out );
		close ( er );
		dup2( defaultin, 0 );
		dup2( defaultout, 1 );
		dup2( defaulterr, 2 );
		close( defaultin );
		close( defaultout );
		close( defaulterr );
		if ( !_background ) {
			waitpid(pid, NULL, 0);
		}
	}
		

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("$s3s3/shell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void handle_exit(int sig){
	printf("\n TO TERMINATE USE EXIT COMMAND!! (Press Enter to return) \n$s3s3/shell>");
}

void writeLog(int pid) {
	FILE *f = fopen("log.txt", "a");
	fprintf(f, "%d\n", pid);
	fclose(f);
}

void handle_chld(int sig) {
	FILE *f = fopen("log.txt", "a");
	int pid;
	while ( 1 ) { 
		pid = waitpid(-1, NULL, WNOHANG);
		if ( pid <= 0 ) break;
		fprintf(f, "%d\n", pid);
	}
	fclose(f);
}

void initLog() {
	FILE *f = fopen("log.txt", "w");
	fprintf(f, "CHILD PID\n");
	fclose(f);
}

int main()
{
	initLog();
	//signal(SIGCHLD, handle_chld);
	signal(SIGINT, handle_exit);
	Command::_currentCommand.prompt();
	if ( yyparse() )
		printf("Invalid Command Form\n");
	return 0;
}

