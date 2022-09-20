#include <iostream>
#include <string.h>	//strtok, strcmp
#include <unistd.h>	//execvp, fork
#include <sys/wait.h>	//wait
#include <fcntl.h>	//open + flags
using namespace std;

//this function separates subcommand and stores it inside subtoken, it updates iterator of main token array, and it updates read and write file descriptors
bool create_subtokens(char** tokens, char** subtokens, int& iterator, int& readfd, int& writefd, int* pipefd);

int main() {
	char array[100],
	     **tokens = new char* [20],
	     **subtokens = new char* [5];

	int i, exec_status; //i for tokenization, exec_status for execvp command status

	pid_t ret_val;
	int iterator, readfd, writefd, pipefd, fd1[2], fd2[2]; //iterator for **tokens, readfd & writefd for redirection, additionally 2 pipes are created
	int backup_readfd, backup_writefd; //for exec failure

	iterator = 0; //iterator for **tokens starts reading from 0th index
	bool redirect; //it ensures if 1st pipe was opened earlier for reading, the next time, it wont be opened again for reading. 2nd pipe will be used
	
	while(true) {
		if(iterator == 0) { //executes for every new command (explicitly asked by user), otherwise, previous command has remaining subcommands which aren't executed. 
			redirect=false;
			close(fd1[0]);
			close(fd1[1]);
			close(fd2[0]);
			close(fd2[1]);

			if(pipefd = pipe(fd1) == -1) {
				cout<<"error: pipe failure!\n";
				break;
			}
			if(pipefd = pipe(fd2) == -1) {
				cout<<"error: pipe failure!\n";
				break;
			}

			for(int j=0;j<i;j++) tokens[j] = NULL;

			cout<<"Command: ";
			cin.getline(array, 100, '\n');
		
			if(strcmp(array, "exit") == 0) break;

   			i = 0;
   			tokens[i] = strtok(array, " ");
   			while(tokens[i] != NULL )
      				tokens[++i] = strtok(NULL, " ");
      		}
      		//cout<<iterator<<endl;
			
      		readfd = 0;
      		writefd = 1;

      		if(!create_subtokens(tokens, subtokens, iterator, readfd, writefd, fd1)) {
      			//subtoken failure detected
      			iterator = 0;
      			for(int j=0;j<i;j++) tokens[j]=NULL;
      			for(int j=0;j<5;j++) subtokens[j]=NULL;
      			continue;
      		}

      		//redirection conditions
      		if(readfd == fd1[0] && writefd == fd1[1]) {
      			if(!redirect) {
      				close(fd2[0]);
      				close(fd2[1]);
      				pipe(fd2);

      				writefd = fd2[1];
      				redirect = true;
      			}
      			else {
      				close(fd1[0]);
      				close(fd1[1]);
      				pipe(fd1);

      				readfd = fd2[0];
      				writefd = fd1[1];
      				redirect = false;
      			}
      		}
      		else if(redirect && readfd==fd1[0]) {
      			readfd=fd2[0];
      		}

      		/*for(int j=0;subtokens[j]!=NULL;j++) 
      			cout<<subtokens[j]<<' ';
      		cout<<endl;*/

      		ret_val = fork();
      		if(ret_val == 0) {
      			backup_readfd=dup(0);
      			backup_writefd=dup(1);
      			
      			//cout<<readfd<<' '<<writefd<<endl;
      			if(readfd!=0) {
      				dup2(readfd, 0);
      				close(readfd);
      			}
      			else close(backup_readfd);
				
      			if(writefd!=1) {
      				dup2(writefd, 1);
      				close(writefd);
      			}
      			else close(backup_writefd);

      			close(fd1[0]);
      			close(fd1[1]);
      			close(fd2[0]);
      			close(fd2[1]);
      			exec_status = execvp(subtokens[0], subtokens);
      			
      			if(exec_status == -1) { //exec command failure
      				dup2(0, backup_readfd);
      				dup2(1, backup_writefd);
      				close(backup_readfd);
      				close(backup_writefd);
      				cout<<"error: invalid command!\n";
      				break;
      			}
      		}
      		else if(ret_val > 0) {
      			wait(NULL); //waiting for child to finish execution

      			if(readfd!=0) {
      				if(readfd == fd1[0]) fd1[0]=-1;
      				if(readfd == fd2[0]) fd2[0]=-1;
      				close(readfd);
      			}

      			if(writefd!=1) {
      				if(writefd == fd1[1]) fd1[1]=-1;
      				if(writefd == fd2[1]) fd2[1]=-1;
      				close(writefd);
      			}
      		}
      		else {
      			cout<<"error: fork failure!\n";
      		}

      		//reset subtokens after each execution
      		for(int j=0;j<5;j++) subtokens[j] = NULL;

   	}//while loop ends here
   	
   	delete[] tokens;
   	tokens = NULL;
   	close(fd1[0]);
   	close(fd1[1]);
   	close(fd2[0]);
   	close(fd2[1]);

	return 0;
}

bool create_subtokens(char** tokens, char** subtokens, int& iterator, int& readfd, int& writefd, int* pipefd) {

	int i=0;		//index for subtoken
	bool resume = true,	//keeps adding tokens into subtokens when true
	     success = true;	//true till any failure occurs

	while(resume) {
		if(tokens[iterator] == NULL) { //main token array is fully traversed
			iterator = 0;
			subtokens[i] = NULL;
			resume = false;
		}
		else if(strcmp(tokens[iterator], "<")==0) { //input redirection met
			iterator++;
			readfd = open(tokens[iterator], O_RDONLY); //opening file

			if(readfd == -1) {
				success = false;
				resume = false;
			}
			else {
				iterator++;
				if(tokens[iterator] != NULL && strcmp(tokens[iterator], "|") == 0) { //for scenarios where user prefers output to be written in pipe after reading from file
					writefd = pipefd[1];
					resume = false;
				}
			}
		}
		else if(strcmp(tokens[iterator], ">")==0) { //output redirection met
			iterator++;
			writefd = open(tokens[iterator], O_WRONLY | O_CREAT | O_TRUNC, 0666); //opening file (creating if it doesnt exist)
			
			if(writefd == -1) {
				success = false;
				resume = false;
			}
			else iterator++;
			resume = false;
		}
		else if(strcmp(tokens[iterator], "|")==0) { //pipe redirection met
			if(i == 0) { //first token is pipe which denotes that data should be read from pipe
				readfd = pipefd[0];
				iterator++;
			}
			else { //for writing in pipe
				writefd = pipefd[1];
				resume = false;
			}
		}
		else subtokens[i++] = tokens[iterator++]; //all above cases are false, that means just store token in subtoken and increment iterators..

	}
	return success;
}
