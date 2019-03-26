#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

//create arg array for all characters 
static char *args[200];
//this pipe will process our pipe as our second pipe
static int command_pipe[2];
//this will track/help the process
static pid_t p;
//will track our line for our command
static char line[1024];
//tracks calls made to the command
static int callNum = 0;
//tracks pipe totals
static int pipeFlag = 0;

//this is the wait process used by the pipes/fork processes
static void waitProcess (int callNum);

//wait function
static void
waitProcess (int callNum)
{
  int i;
  //waits as long as needed
  for (i = 0; i < callNum; ++i)
    wait (NULL);
}

//this will grab the command based on input, using fork and pipe
static int
inputCommandAndPipe (int command, int inputStart, int inputEnd)
{
//initialize pipe/fork info
  p = fork ();
  int fd[2];
  pipe (fd);

  //sentinel for process
  int sentinel = 0;

//below is pipe and fork processes
  if (p == 0)
    {
      for (int i = 0; args[i] != 0; i++)
	{
	  if (args[i][0] == '>')
	    {
	      //these have funny names but they are part of the imports
	      //these control pipes
	      fd[1] = open (args[i + 1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
	      sentinel = 1;
	    }
	  //checks for redirection in linux terminal
	  if (args[i] == ">>")
	    {
	      fd[1] = open (args[i + 1], O_APPEND | O_WRONLY, 0644);
	      sentinel = 1;
	    }
	}
      if (inputStart == 1 && inputEnd == 0 && command == 0)
	{
	  dup2 (fd[1], 1);
	}
      else if (inputStart == 0 && inputEnd == 0 && command != 0)
	{
	  dup2 (command, 0);
	  dup2 (fd[1], 1);
	}
      else
	{
	  if (sentinel == 1)
	    {
	      dup2 (fd[1], 1);
	    }
	  dup2 (command, 0);
	}
      if (sentinel == 1)
	{
	  //processes redirection using execlp based on case
	  if (strcmp (args[1], ">>") == 0)
	    execlp (args[0], args[0], 0, (char *) 0);
	  else if (strcmp (args[1], ">") == 0)
	    execlp (args[0], args[0], 0, (char *) 0);
	  else
	    execlp (args[0], args[0], args[1], (char *) 0);
	}
      else if (execvp (args[0], args) == -1)
	exit (1);
    }

//closes all pipe/fork variables
  if (command != 0)
    close (command);
  close (fd[1]);
  if (inputEnd == 1)
    close (fd[0]);
  return fd[0];
}

//runs command to begin
static int runCommand (char *commandChar, int command, int inputStart,
		       int inputEnd);

//main function
int
main ()
{
  printf ("Simple shell runner in C: Professor Nhut's Assignment \n");
  printf ("At any time, press 0 to exit.\n\n");

  //runs infinite true loop until broken by user
  while (1)
    {
      //simple trackers used for processes
      pipeFlag = 0;
      int command = 0;
      int inputStart = 1;

      //how I have my linux terminal set up
      printf ("squinto@tinyshell:~$ ");
      fflush (NULL);
//exits if not proper
      if (!fgets (line, 1024, stdin))
	return 0;

      //initlalizes line as input saver
      char *commandChar = line;
      //checks delimiter |, ;, or 0
      char *next = strchr (commandChar, '|');
      char *semi = strchr (commandChar, ';');
      char *end = strchr (commandChar, '0');
      //if it is zero we end the code
      while (end != NULL)
	{
	  return 0;
	}

//char is grabbed, goes into this loop if the char is a pipe "|"
      while (next != NULL)
	{
	  //pointer, checks to find the pipe |
	  *next = '\0';
	  //finds pipe, and chops string to only contain command between the pipe or new command
	  //just the one.
	  command = runCommand (commandChar, command, inputStart, 0);

	  commandChar = next + 1;
	  //used to find the next pipe, if any
	  next = strchr (commandChar, '|');
	  inputStart = 0;
	}

      //same as the above while loop, will check char to see if it is a semicolon
      while (semi != NULL)
	{
	  //pipeflag acts as a sentinel
	  pipeFlag++;
	  //these variables set up a popen command
	  char commandGrabber[1024];
	  int len;
	  FILE *fp;
	  int status;
	  char path[400];
	  //special formatting here since we want the string to be an already
	  //initialized variable
	  len = snprintf (commandGrabber, 1024, line);
	  //as long as it fits size requirements we run the command
	  if (len <= 1024)
	    fp = popen (commandGrabber, "r");

//here we pring the command
	  while (fgets (commandGrabber, 1024, fp) != NULL)
	    printf ("%s", commandGrabber);

	  //pointer, checks to find the pipe |
	  *semi = '\0';
	  //finds pipe, and chops string to only contain command between the pipe or new command
	  //just the one.
	  commandChar = semi + 1;
	  //used to find the next pipe, if any
	  semi = strchr (commandChar, ';');
	  inputStart = 0;
	  //breaks loop
	  break;
	  exit;

	}
      //we only run the command if popen was not used, as it has not been taken care of
      if (pipeFlag == 0)
	command = runCommand (commandChar, command, inputStart, 1);
      //waits for process to be completed
      waitProcess (callNum);
      callNum = 0;
    }
  return 0;
}

//this handles tokenizing the commands needed for the program
static void charTokenizer (char *commandChar);

//made to run the command, checks if it is an exit case
int
runCommand (char *commandChar, int command, int inputStart, int inputEnd)
{
  charTokenizer (commandChar);
  if (args[0] != NULL)
    {
      //exits when exit is typed, as a linux terminal would
      if (strcmp (args[0], "exit") == 0)
	exit (0);
      callNum += 1;
      //runs command after checking for exit case into the command pipe
      return inputCommandAndPipe (command, inputStart, inputEnd);
    }
  return 0;
}

//checks for space, and counts them
static char *
spaceDelimeter (char *s)
{
  while (isspace (*s))
    ++s;
  return s;
}

//used to tokenize a string to it can be piped and processed
static void
charTokenizer (char *commandChar)
{
//checks for spacing
  commandChar = spaceDelimeter (commandChar);
  char *next = strchr (commandChar, ' ');
  int i = 0;

  while (next != NULL)
    {
      //cuts string based on it being non empty
      next[0] = '\0';
      args[i] = commandChar;
      ++i;
      commandChar = spaceDelimeter (next + 1);
      next = strchr (commandChar, ' ');
    }
//keeps command going as long as non empty
  if (commandChar[0] != '\0')
    {
      args[i] = commandChar;
      next = strchr (commandChar, '\n');
      next[0] = '\0';
      ++i;
    }
//returns final case as empty to restart
  args[i] = NULL;
}
