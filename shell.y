%token	<string_val> WORD

%token 	NOTOKEN OUTPUT NEWLINE NEW INPUT AOUT  BACK AOUTBACK OUTPUTBACK

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;

simple_command:	
	commands_list iomodifier_opt_list backp NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

iomodifier_opt_list:
	iomodifier_opt_list iomodifier_opt
	| /* can be empty */
	;
	
commands_list:
	commands_list NEW command_and_args
	|  command_and_args
	;
	

command_and_args:
	command_word arg_list{
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->wildcardArg( $1 );\
	}
	;

command_word:
	WORD{
		printf("   Yacc: insert command \"%s\"\n", $1);
		Command::_currentSimpleCommand = new SimpleCommand();
	        Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

iomodifier_opt:
	OUTPUT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._out_new = 1;
	}
	| OUTPUTBACK WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._out_new = 1;
	}
	| INPUT WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	| AOUT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._out_new = 0;
	}
	| AOUTBACK WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._out_new = 0;
	}
	;

backp :
	BACK {
		printf("   Yacc: initiate back processing \n");
		Command::_currentCommand._background = 1;
	}
	| /*can be empty*/
	;
	
%%

void yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
