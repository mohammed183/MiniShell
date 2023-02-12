%{

#include <string.h>
#include "y.tab.h"

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return OUTPUT;
	}

"<"	{
		return INPUT;
	}
	
"|"	{
		return NEW;
	}
	
"&"	{
		return BACK;
	}
	
">>&"	{
		return AOUTBACK;
	}
	
">&"	{
		return OUTPUTBACK;
	}
	
"&>>"	{
		return AOUTBACK;
	}
	
"&>"	{
		return OUTPUTBACK;
	}
	
">>"	{
		return AOUT;
	}


[^ ><|\t\n][^ ><|\t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

.	{
		/* Invalid character in input */
		return NOTOKEN;
	}

%%
int yywrap(void) {
	return 1;
}
