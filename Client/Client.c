#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<time.h>
#include<limits.h>

// To show default system errors
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// Utility function declaration
void user_customer(int sockfd, char buffer[255], char username[50]);
void user_admin(int sockfd, char buffer[255]);
void user_police(int sockfd, char buffer[255]);
void generate_query(char buffer[255], char username[50], int avail_bal, int amount, int type);

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	// Check for sufficient arguments while connecting
	char buffer[255];

	if(argc < 3)
	{
		fprintf(stderr, "Give server IP and Port no.\n");
		exit(1);
	}

	// Server port number
	portno = atoi(argv[2]);

	// Attempt to intiliaze client socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
		error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if(server == NULL)
		fprintf(stderr, "Error, no such host");

	// Detailing server specifications to the socket
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	// Connect client socket to server socket 
	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("Connection Failed");
	else
		printf("Connection established successfully with the Bank server\n");

	char username[50];
	char password[50];
	char i;

	while(1)
	{
		// Basic menu
		printf("Enter `1` to Login\n");
		printf("Enter `2` to Exit\n>>>> ");
		scanf("%c", &i);

		while((getchar()) != '\n');
		if(i != '1' && i != '2')
		{
			printf("Invalid Response\n");
			continue;
		}
		else if(i == '2')
			break;
		else
		{
			// User login
			bzero(buffer, 255);
			bzero(username, 50);
			bzero(password, 50);

			printf("Username: ");
			scanf("%[^\n]", username);
			while((getchar()) != '\n');
			printf("Password: ");
			scanf("%[^\n]", password);
			while((getchar()) != '\n');

			// Requesting login of form LOGIN_REQUEST: <USERNAME,PASSWORD,> TO  server
			strcat(buffer, "LOGIN_REQUEST: <");
			strcat(buffer, username);
			strcat(buffer, ",");
			strcat(buffer, password);
			strcat(buffer, ",>");

			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			bzero(buffer, 255);

			// Waiting for server response
			n = read(sockfd, buffer, 255);
			if(n < 0)
				error("Error on reading.");

			// Server Response of form RESPONSE <log_Succ/log_UnSucc 'user type 'C/A/P> 
			if(strncmp(buffer, "RESPONSE: <Log_Succ ", 20) == 0)
			{
				printf("Login Successfull\n");
				if(buffer[strlen(buffer) - 2] == 'C')
					user_customer(sockfd, buffer, username);
				else if(buffer[strlen(buffer) - 2] == 'A')
					user_admin(sockfd, buffer);
				else if(buffer[strlen(buffer) - 2] == 'P')
					user_police(sockfd, buffer);
			}
			else if(strcmp(buffer, "RESPONSE: <Log_UnSucc>") == 0)
			{
				printf("Login Failed!!!\n");
				continue;
			}
		}
	}
	close(sockfd);

	return(0);
}

void user_customer(int sockfd, char buffer[255], char username[50])
{
	int n, i, j, no_of_lines;
	char choice;

	while(1)
	{	
		// Menu for customer
		printf("Enter `1` for Available Balance\n");
		printf("Enter `2` for MINI STATEMENT\n");
		printf("Enter `3` to LogOut\n>>>> ");
		scanf("%c", &choice);
		while((getchar()) != '\n');

		if(choice != '1' && choice != '2' && choice != '3')
		{
			printf("Invalid Response\n");
			continue;
		}
		else if(choice == '3')
			return;
		else if(choice == '1')
		{
			// Request balance to server
			bzero(buffer, 255);
			strcat(buffer, "BALANCE_REQUEST: C <");
			strcat(buffer, username);
			strcat(buffer, ">");
			
			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			bzero(buffer, 255);
			n = read(sockfd, buffer, 255);
			if(n < 0)
				error("Error on reading.");

			printf("Available Balance in your account is - ");
			i = 10;
			while(buffer[i] != '\0')
			{
				printf("%c",buffer[i]);
				i++;
			}
			printf("\n");
		}
		else if(choice == '2')
		{
			// Request mini statement file to server
			bzero(buffer, 255);
			strcat(buffer, "M_STATE_REQUEST: <");
			strcat(buffer, username);
			strcat(buffer, ">");
			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			// Remove existing file
			remove("MiniStatement.txt");

			// Open the file and append the information of ministatement from the server
			FILE *file;
			file = fopen("MiniStatement.txt","a");
			fprintf(file, "%s\n", username);
			char temp[] = "Transaction_Date,Transaction_Type,Available_Balance";
			fprintf(file, "%s\n", temp);

			bzero(buffer, 255);
			no_of_lines = 0;
			n = read(sockfd, &no_of_lines, 1);
			if(n < 0)
				error("Error on reading.");

			bzero(buffer, 255);

			j = 1;
			while(j <= no_of_lines)
			{
				bzero(buffer, 255);
				read(sockfd, buffer, 255);
				fputs(buffer, file);
				bzero(buffer, 255);
				strcat(buffer, "ACK");
				write(sockfd, buffer, strlen(buffer));
				j++;
			}
			printf("File Received\n");
			fclose(file);
		}
	}
}

// For Admin
void user_admin(int sockfd, char buffer[255])
{
	int i, j, n;
	char username[50], choice;

	while(1)
	{
		// Basic menu
		printf("Enter `1` for Transaction\n");
		printf("Enter `2` to LogOut\n>>>> ");
		scanf("%c", &choice);
		while((getchar()) != '\n');

		if(choice != '1' && choice != '2')
		{
			printf("Invalid Response\n");
			continue;
		}
		else if(choice == '2')
			return;
		else if(choice == '1')
		{
			printf("Enter the username - ");
			bzero(username, 50);
			scanf("%[^\n]", username);
			while((getchar()) != '\n');

			// Request transaction to server
			bzero(buffer, 255);
			strcat(buffer, "TRANS_REQUEST: A <");
			strcat(buffer, username);
			strcat(buffer, ">");

			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			bzero(buffer, 255);
			n = read(sockfd, buffer, 255);
			if(n < 0)
				error("Error on reading.");

			if(strcmp(buffer, "RESPONSE: <INVU>") == 0)
				printf("Invalid Username\n");
			else if(strncmp(buffer, "RESPONSE: <Avail>", 17) == 0)
			{
				int avail_bal, amount;
				char avail_bal_s[50];
				i = 20;
				j = 0;
				bzero(avail_bal_s, 50);
				while(*(buffer + i) != '\0')
				{
					avail_bal_s[j] = *(buffer + i);
					j++;
					i++;
				}
				sscanf(avail_bal_s, "%d", &avail_bal);
				printf("Available Balance - %d\n", avail_bal);

				// Request for credit or debit to server
				printf("Enter `1` for Credit\n");
				printf("Enter `2` for Debit\n>>>> ");
				scanf("%c", &choice);
				while((getchar()) != '\n');

				// Credit
				// Entering the amount and generate query of the form Date, credit amountToCredit, Balanace 
				if(choice != '1' && choice != '2')
					printf("Invalid Response\n");
				else if(choice == '1')
				{
					printf("Enter the Amount - ");
					int c = scanf("%d", &amount);

					while((getchar()) != '\n');
					if(c == 1 && amount >= 0)
					{
						generate_query(buffer, username, avail_bal, amount, 1);
						n = write(sockfd, buffer, strlen(buffer));
						if(n < 0)
							error("Error on writing.");
						printf("Transaction Successfull\n");
						printf("Available Balance for %s is %d\n", username, avail_bal + amount); 
					}
					else
						printf("Enter a valid Amount\n");
				}

				// Debit
				// Entering the amount and generate query of the form Date,credit amountToCredit,Balanace
				else if(choice == '2')
				{
					printf("Enter the Amount - ");
					scanf("%d", &amount);
					while((getchar()) != '\n');

					if(amount > avail_bal)
					{
						printf("Insufficient Balance\n");
						bzero(buffer, 255);
					}
					else
					{
						generate_query(buffer, username, avail_bal, amount, 2);
						n = write(sockfd, buffer, strlen(buffer));
						if(n < 0)
							error("Error on writing.");
						printf("Transaction Successfull\n");
						printf("Available Balance for %s is %d\n", username, avail_bal - amount);
					}
				}
			}
		}
	}
}

// For Police
void user_police(int sockfd, char buffer[255])
{
	int n, no_of_customers, j;
	char username[50], filename[70], choice;
	while(1)
	{
		// Basic menu
		printf("Enter `1` to get the Available Balance of all customers\n");
		printf("Enter `2` to get Mini Statement of a user\n");
		printf("Enter `3` to LogOut\n>>>> ");
		scanf("%c", &choice);
		while((getchar()) != '\n');

		if(choice != '1' && choice != '2' && choice != '3')
		{
			printf("Invalid Response\n");
			continue;
		}
		else if(choice == '3')
			return;
		else if(choice == '1')
		{
			// Requesting file -> available balance of all customers 
			bzero(buffer, 255);
			strcat(buffer, "BALANCE_REQUEST: P");
			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			remove("Customer_Balance_Sheet.txt");

			// Apend information in given file from the server
			FILE *file;
			file = fopen("Customer_Balance_Sheet.txt","a");
			char a[] = "Username,Balance";
			fprintf(file, "%s\n", a);

			bzero(buffer, 255);
			no_of_customers = 0;
			n = read(sockfd, &no_of_customers, 1);
			if(n < 0)
				error("Error on reading.");

			bzero(buffer, 255);
			j = 1;
			while(j <= no_of_customers)
			{
				bzero(buffer, 255);
				read(sockfd, buffer, 255);
				fputs(buffer, file);
				bzero(buffer, 255);
				strcat(buffer, "ACK");
				write(sockfd, buffer, strlen(buffer));
				j++;
			}
			printf("File Received\n");
			fclose(file);
		}
		else if(choice == '2')
		{
			// Requsting mini statement file of a particular user
			printf("Enter the username - ");
			bzero(username, 50);
			scanf("%[^\n]", username);
			while((getchar()) != '\n');

			bzero(buffer, 255);
			strcat(buffer, "M_STATE_REQUEST: <");
			strcat(buffer, username);
			strcat(buffer, ">");

			n = write(sockfd, buffer, strlen(buffer));
			if(n < 0)
				error("Error on writing.");

			bzero(buffer, 255);
			int no_of_lines = 0;
			n = read(sockfd, &no_of_lines, 1);
			if(n < 0)
				error("Error on reading.");
			
			if(no_of_lines == 20)
				printf("Invalid Username\n");
			else
			{
				char c[] = "Transaction_Date, Transaction_Type, Available_Balance";
				bzero(filename, 70);
				strcat(filename, "MiniStatement_");
				strcat(filename, username);
				strcat(filename, ".txt");

				remove(filename);

				FILE *file2;
				file2 = fopen(filename,"a");
				fprintf(file2, "%s\n", c);

				bzero(buffer, 255);
				int j = 1;
				while(j <= no_of_lines)
				{
					bzero(buffer, 255);
					read(sockfd, buffer, 255);
					fputs(buffer, file2);
					bzero(buffer, 255);
					strcat(buffer, "ACK");
					write(sockfd, buffer, strlen(buffer));
					j++;
				}
				printf("File Received\n");
				fclose(file2);
			}
		}
	}
}

// Utility function for arranging string in a specific format used by user_admin function
void generate_query(char buffer[255], char username[50], int avail_bal, int amount, int type)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int year = tm.tm_year + 1900;
	int month = tm.tm_mon + 1;
	int day = tm.tm_mday;
	char year_s[10], mon_s[10], day_s[10];

	bzero(year_s, 10);
	sprintf(year_s, "%d", year);
	bzero(mon_s, 10);
	sprintf(mon_s, "%d", month);
	bzero(day_s, 10);
	sprintf(day_s, "%d", day);

	bzero(buffer, 255);
	strcat(buffer, "TRANSACTION <");
	strcat(buffer, username);
	strcat(buffer, "> ");
	strcat(buffer, year_s);
	strcat(buffer, "-");
	strcat(buffer, mon_s);
	strcat(buffer, "-");
	strcat(buffer, day_s);
	strcat(buffer, ",");

	if(type == 1)
		strcat(buffer, "Credit ");
	else
		strcat(buffer, "Debit ");

	char amount_s[20];
	bzero(amount_s, 20);
	sprintf(amount_s, "%d", amount);
	strcat(buffer, amount_s);
	strcat(buffer, ",");

	int total;
	if(type == 1)
		total = avail_bal + amount;
	else
		total = avail_bal - amount;
	char total_s[20];
	bzero(total_s, 20);
	sprintf(total_s, "%d", total);
	strcat(buffer, total_s);
	strcat(buffer, "\n");
}
