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
typedef unsigned char bool;
#include"list_head.h"
#include"parser.h"
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
LIST_HEAD(alias);
struct entry {
	struct list_head list;
	char *key;
	char *value[MAX_NR_TOKENS];
};
void push_alias(char * tokens[])
{
	struct entry *node =(struct entry *)malloc(sizeof(struct entry));
	node->key=strdup(tokens[1]);
	int idx=2;
	int i=0;
	while(tokens[idx]!=NULL)
	{
		node->value[i++]=strdup(tokens[idx++]);
	}
	node->value[i]=NULL;
	list_add_tail(&node->list,&alias);
}

int run_command(int nr_tokens, char *tokens[])
{
	if (strcmp(tokens[0], "exit") == 0) return 0;
	
	int idx=0;
	while(!list_empty(&alias)&&tokens[idx]!=NULL) //alias change
	{
		struct entry *node;
		list_for_each_entry(node,&alias,list){
			if(strcmp(tokens[idx],node->key)==0) //find in alias
				{
					char *newtokens[MAX_NR_TOKENS*2]={NULL};
					for(int i=0;i<idx;i++)
					{
						newtokens[i]=strdup(tokens[i]);
					}
					int nr_tokens2=0;
					int i=idx;
					while(node->value[i-idx]!=NULL)
					{
						newtokens[i]=strdup(node->value[i-idx]);
						nr_tokens2++;
						i++;
					}
					i=idx;
					nr_tokens=0;
					while(tokens[nr_tokens]!=NULL)
						nr_tokens++;
					for(int i=idx;i<nr_tokens-1;i++)
					{
						if(tokens[i+1]!=NULL)
							newtokens[i+nr_tokens2]=strdup(tokens[i+1]);
					}
					tokens=(char**)malloc(sizeof(newtokens));
					for(int i=0;newtokens[i]!=NULL;i++)
						tokens[i]=newtokens[i];
					idx+=nr_tokens2-1;
					break;
				}
		}
		idx++;
	}

	int is_pipe=0;
	int pipeidx=0;
	for(int i=0;tokens[i]!=NULL;i++)
	{
		if(strcmp(tokens[i],"|")==0)
		{	
			is_pipe=1;
			pipeidx=i;
		}
	}
	if(is_pipe==1)
	{
		char*left_tokens[MAX_NR_TOKENS]={NULL};
		char *right_tokens[MAX_NR_TOKENS]={NULL};
		for(int i=0;i<pipeidx;i++)
			left_tokens[i]=strdup(tokens[i]);
		for(int i=pipeidx+1;tokens[i]!=NULL;i++)
			right_tokens[i-pipeidx-1]=strdup(tokens[i]);
		
		int pipefd[2];
		pid_t cpid1,cpid2;
		cpid1=fork();
		if (cpid1== 0)
		{
			if (pipe(pipefd) == -1)
			{
				perror("pipe");
				return -1;
			}
			cpid2=fork();
			if(cpid2==0)
			{
				close(pipefd[0]);
				dup2(pipefd[1],1);
				execvp(left_tokens[0], left_tokens);
				fprintf(stderr, "Unable to execute %s\n", left_tokens[0]);
				exit(EXIT_FAILURE);
			}
			else{
				usleep(500);
				close(pipefd[1]);
				dup2(pipefd[0],0);
				execvp(right_tokens[0],right_tokens);
				fprintf(stderr, "Unable to execute %s\n", right_tokens[0]);
				exit(EXIT_FAILURE);
			}
		}
		else{
			waitpid(cpid1,NULL,0);
			return 1;
		}
	}



	else if(strcmp(tokens[0],"cd")==0)
	{
		char*path;
		if(tokens[1]==NULL||strcmp(tokens[1],"~")==0)
		{
			path=getenv("HOME");
		}
		else
			path=tokens[1];
		if(path==NULL)
			{
				return -1;
			}
		chdir(path);
		return 1;
	}
	else if(strcmp(tokens[0],"alias")==0)
	{
		if(tokens[1]==NULL)
		{
			struct entry *node;
			list_for_each_entry(node,&alias,list){
				fprintf(stderr,"%s: ",node->key);
				int i=0;
				while(node->value[i]!=NULL)
					fprintf(stderr,"%s ",node->value[i++]);
				fprintf(stderr,"\n");
			}		
		}
		else
			push_alias(tokens);
		return 1;
	}
	else//extrenal program
	{
		pid_t pid = fork();
		if (pid == -1) // fail
		{
			return -1;
		}
		else if (pid)
		{
			wait(NULL);
			return 1;
		}
		else // child
		{
			execvp(tokens[0], tokens);
			return -1;
		}
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
