/**********************************************************************
 * Copyright (c) 2020-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

int runEx(int nr_tokens, char *tokens[])
{
	pid_t pid =fork();
	if(pid==-1) //fail
    {    
        return -1;
    }else if(pid)
    {
        wait(NULL);
		return 1;
    }
    else //child
    {
		execvp(tokens[0],tokens);
		exit(EXIT_FAILURE);
    }
}
int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0;
	//Execute external commands 
	if(strcmp(tokens[0],"ls")==0||strcmp(tokens[0],"pwd")==0||strcmp(tokens[0],"cp")==0)
	{
		return runEx(nr_tokens,tokens);
	}
	

	return -1;
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char * const argv[])
{
	if(strcmp(argv[0],"cp")==0&&argc!=2)
		return -1;
	
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{
}
