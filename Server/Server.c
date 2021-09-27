#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<ctype.h>
#include<stdbool.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<limits.h>

#define TRUE 1

//To show default system errors
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// Different functions declaration
void logged_in_customer(int newsockfd, char buffer[255], char line[200]);
void logged_in_admin(int newsockfd, char buffer[255]);
void logged_in_police(int newsockfd, char buffer[255]);
void get_balance(char username[50], char balance[50]);
int get_no_of_lines(char filename[60]);
void send_mini_statement(int newsockfd, char buffer[255], char username[50]);
int get_customer_list(char cus_arr[10][50]);

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Port not provided, Program terminated\n");		// Check if sufficient arguments provided or not
		exit(1);
	}

	// portno -- given in arguments ,  master_socket-- to handle multiple clients , sd-- socket descriptor
	int portno, master_socket, new_socket, client_socket[15], sd;
	int n, i, j, valread, activity, max_clients = 15, max_sd = 0;
	int opt = TRUE;
	char buffer[255];

	fd_set readfds;		// File descriptor set to handle multiple client

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	master_socket = socket(AF_INET, SOCK_STREAM, 0);	// Master socket creation

	if(master_socket < 0)
		error("Error opening Master Socket.");
	else
		printf("To close the server: press ctrl+ c. \n Master Socket Opened\n");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	// Set master socket to allow multiple connections
	n = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));
	if(n < 0)
	{
		perror("SetSockOpt Error");
		exit(EXIT_FAILURE);
	}

	// Defining parameters of sockaddr_in struct variable serv_addr
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// Binding of master socket with the server address
	n = bind(master_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(n < 0)
		error("Binding Failed.");
	else
		printf("Binded\n");


	// Listen for connection on master socket (max 15 clients allowed)
	listen(master_socket, 15);
	clilen = sizeof(cli_addr);

	char line[200], temp[200], creden[200];
	char username[50], balance[60], filename[60];
	char user_type[2];
	char cus_arr[10][50];
	int login = 0;
	int no_of_customers;

	// Intializing cliet sockets to 0 , i.e. no client available
	for(i = 0; i < max_clients; i++)
		client_socket[i] = 0;

	while(1)
	{
		FD_ZERO(&readfds);		// Vacate the fd set
		FD_SET(master_socket, &readfds);	// Add master socket to fd set
		max_sd = master_socket;		// Will be used for select function

		for(i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
			if(sd > 0)
				FD_SET( sd , &readfds);		// Adding available clients to fd set
			if(sd > max_sd)
				max_sd = sd;	// Updating max_sd
		}

		bzero(buffer, 255);
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);		// Waiting for any activity from client side either from new client or from existing one.

		if((activity < 0) && (errno != EINTR))
			printf("Select Error");

		if(FD_ISSET(master_socket, &readfds))		// If master_socket flag is set --> new connection request.
		{
			new_socket = accept(master_socket, (struct sockaddr *) &cli_addr, &clilen);		// Accepting new client to a new socket.
			if(new_socket < 0)
			{
				perror("Accept Error");
				exit(EXIT_FAILURE);
			}
			else
				printf("New Connection Accepted\n");

			// Adding new client to client array.
			for(i = 0; i < max_clients; i++)
			{
				if(client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets with index %d\n", i);
					break;
				}
			}
		}

		no_of_customers = get_customer_list(cus_arr);	// Total no of available customers with the bank.

		for(i = 0; i < max_clients; i++)	// To check for each client socket it has shown any activity to server.
		{
			sd = client_socket[i];

			if(FD_ISSET(sd, &readfds))
			{
				bzero(buffer, 255);
				valread = read(sd, buffer, 255);	// Reading request from the client.
				if(valread == 0)	// Exit request by client.
				{
					close(sd);
					client_socket[i] = 0;
				}

				if(strncmp(buffer, "LOGIN_REQUEST: <", 16) == 0)	// Login request from the client with credentials attached to message.
				{
					bzero(creden, 200);
					strncpy(creden, buffer + 16, strlen(buffer) - 17);

					FILE *file = fopen("Login_File.txt", "r");		// Lookup file for credentials
					login = 0;
					while(fgets(line, 200, file) != NULL)
					{
						bzero(temp, 200);
						strncpy(temp, line, strlen(line) - 2);
						if(strcmp(temp, creden) == 0)
						{
							login = 1;
							break;
						}
						bzero(line, 200);
					}
					fclose(file);

					if(login == 0)		// Login unsuccessful
					{
						bzero(buffer, 200);
						strcat(buffer, "RESPONSE: <Log_UnSucc>");	// Sending respose.

						n = write(sd, buffer, strlen(buffer));
						if(n < 0)
							error("Error on Writing.");
						bzero(buffer, 255);
					}
					else if(login == 1)		// Login successful
					{
						user_type[0] = line[strlen(line) - 2];
						user_type[1] = '\0';
						bzero(buffer, 255);
						strcat(buffer, "RESPONSE: <Log_Succ ");
						strcat(buffer, user_type);
						strcat(buffer, ">");

						n = write(sd, buffer, strlen(buffer));		// Sending response with user type attached.
						if(n < 0)
							error("Error on Writing.");
						bzero(buffer, 255);
					}
				}
				else if(strncmp(buffer, "BALANCE_REQUEST: C <", 20) == 0)		// Balance request by the customer.
				{
					bzero(username, 50);
					i = 20;
					j = 0;
					while(buffer[i] != '>')
					{
						username[j] = buffer[i];
						i++;
						j++;
					}
					username[j] = '\0';

					get_balance(username, balance);		// Getting balance for the corresponding user.

					bzero(buffer, 255);
					strcat(buffer, "RESPONSE: ");
					strcat(buffer, balance);

					n = write(sd, buffer, strlen(buffer));		// Sending response to the user.
					if(n < 0)
						error("Error on Writing.");
					bzero(buffer, 255);
				}
				else if(strncmp(buffer, "BALANCE_REQUEST: P", 18) == 0)		// Balance request by police.
				{
					n = write(sd, &no_of_customers, 1);
					if(n < 0)
						error("Error on Writing.");

					for(i = 0; i < no_of_customers; i++)
					{
						bzero(buffer, 255);
						strcat(buffer, cus_arr[i]);
						strcat(buffer, ",");

						get_balance(cus_arr[i], balance);

						strcat(buffer, balance);
						strcat(buffer, "\n");

						write(sd, buffer, strlen(buffer));

						bzero(buffer, 255);
						read(sd, buffer, 255);
						if(strcmp(buffer, "ACK") == 0)		// Sending data to client using STOP AND WAIT ARQ.
							continue;
						else
						{
							printf("Error Sending File\n");
							break;
						}
					}
				}
				else if(strncmp(buffer, "M_STATE_REQUEST: <", 18) == 0)		// Mini Statement request by the client.
				{
					bzero(username, 50);
					i = 18;
					j = 0;
					while(buffer[i] != '>')
					{
						username[j] = buffer[i];
						i++;
						j++;
					}
					username[j] = '\0';

					bool user_exist = false;
					for(i = 0; i < no_of_customers; i++)
						if(strcmp(username, cus_arr[i]) == 0)
							user_exist = true;

					if(user_exist)
						send_mini_statement(sd, buffer, username);
					else
					{
						int t = 20;
						n = write(sd, &t, 1);
						if(n < 0)
							error("Error on Writing.");
						bzero(buffer, 255);
					}
				}
				else if(strncmp(buffer, "TRANS_REQUEST: A <", 18) == 0)		// Transaction request by the cashier.
				{
					bzero(username, 50);
					i = 18;
					j = 0;
					while(buffer[i] != '>')
					{
						username[j] = buffer[i];
						i++;
						j++;
					}
					username[j] = '\0';

					bool user_exist = false;
					for(i = 0; i < no_of_customers; i++)	// Check if given user exists or not.
						if(strcmp(username, cus_arr[i]) == 0)
							user_exist = true;

					if(user_exist)		// Showing balance to the cashier.
					{
						get_balance(username, balance);
						bzero(buffer, 255);
						strcat(buffer, "RESPONSE: <Avail> | ");
						strcat(buffer, balance);

						n = write(sd, buffer, strlen(buffer));
						if(n < 0)
							error("Error on Writing.");
					}
					else	// If user doesn't exist.
					{
						bzero(buffer, 255);
						strcat(buffer, "RESPONSE: <INVU>");
						n = write(sd, buffer, strlen(buffer));
						if(n < 0)
							error("Error on Writing.");
					}
				}
				else if(strncmp(buffer, "TRANSACTION <", 13) == 0)		// Updating transaction to the corresponding user's transaction file.
				{
					bzero(username, 50);
					i = 13;
					j = 0;
					while(buffer[i] != '>')
					{
						username[j] = buffer[i];
						i++;
						j++;
					}
					username[j] = '\0';

					bzero(filename, 60);
					strcat(filename, username);
					strcat(filename, ".txt");

					i = 0;
					while(buffer[i] != '>')
						i++;
					i += 2;
					j = 0;
					bzero(temp, 200);
					while(buffer[i] != '\n')
					{
						temp[j] = buffer[i];
						i++;
						j++;
					}
					temp[j] = '\n';
					FILE *file = fopen(filename, "a");
					fputs(temp, file);
					fclose(file);
				}
			}
		}
	}

	return(0);
}

// Utility function to get the no line of given file.
int get_no_of_lines(char filename[60])
{
	FILE *file;
	int no_of_lines = 0;
	char c;

	file = fopen(filename, "r");
	for(c = getc(file); c != EOF; c = getc(file))
		if(c == '\n')
			no_of_lines++;

	fclose(file);

	return(no_of_lines);
}

// Function to get the balance of the user.
void get_balance(char username[50], char balance[50])
{
	int count, no_of_lines, i, j;
	char temp[200], filename[60];

	bzero(filename, 60);
	strcat(filename, username);
	strcat(filename, ".txt");
	FILE *file = fopen(filename, "r");

	no_of_lines = get_no_of_lines(filename);

	count = 1;
	bzero(balance, 50);
	while(fgets(temp, 200, file) != NULL)
	{
		if(count == no_of_lines)
		{
			i = 0;
			while(temp[i] != ',')
				i++;
			i++;
			while(temp[i] != ',')
				i++;
			i++;
			j = 0;
			while(temp[i] != '\n')
			{
				balance[j] = temp[i];
				i++;
				j++;
			}
			balance[j] = '\0';
			break;
		}
		count++;
		bzero(temp, 200);
	}
	fclose(file);
}

// Sending Mini Statement of user to the socket.
void send_mini_statement(int newsockfd, char buffer[255], char username[50])
{
	int no_of_lines, i, n;
	char filename[60];

	bzero(filename, 60);
	strcat(filename, username);
	strcat(filename, ".txt");

	no_of_lines = get_no_of_lines(filename);

	FILE *file = fopen(filename, "r");

	if(no_of_lines <= 10)
	{
		bzero(buffer, 255);
		n = write(newsockfd, &no_of_lines, 1);
		if(n < 0)
			error("Error on Writing.");

		i = 1;
		while(i <= no_of_lines)
		{
			bzero(buffer, 255);
			fgets(buffer, 255, file);
			write(newsockfd, buffer, strlen(buffer));
			bzero(buffer, 255);
			read(newsockfd, buffer, 255);
			i++;
			if(strcmp(buffer, "ACK") == 0)
				continue;
			else
				printf("Error Sending File\n");
		}
	}
	else
	{
		bzero(buffer, 255);
		int t = 10;
		n = write(newsockfd, &t, 1);
		if(n < 0)
			error("Error on Writing.");

		i = 1;
		bzero(buffer, 255);
		while(fgets(buffer, 255, file) != NULL)
		{
			if(i >= no_of_lines - 9)
			{
				write(newsockfd, buffer, strlen(buffer));
				bzero(buffer, 255);
				read(newsockfd, buffer, 255);
				if(strcmp(buffer, "ACK") == 0)
					continue;
				else
					printf("Error Sending File\n");
			}
			i++;
			bzero(buffer, 255);
		}
	}
	fclose(file);
}

// Get the customer list.
int get_customer_list(char cus_arr[10][50])
{
	int i, j;
	char line[200];

	FILE *file = fopen("Login_File.txt","r");
	i = 0;
	while(fgets(line, 200, file) != NULL)
	{
		j = 0;
		if(line[strlen(line) - 2] == 'C')
		{
			while(line[j] != ',')
			{
				cus_arr[i][j] = line[j];
				j++;
			}
			cus_arr[i][j] = '\0';
			i++;
		}
	}
	fclose(file);

	return(i);
}
