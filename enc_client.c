#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* { close } */
#include <string.h>
#include <sys/types.h>	/* { ssize_t } */
#include <sys/socket.h> /* { send,recv } */
#include <netdb.h>		/* { gethostbyname } */
#include <fcntl.h>		/* { open } */

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

int isupper(char c)
{
	if (65 <= c && c <= 90)
	{
		return 1;
	}
	return 0;
}

int numdigits(int n)
{
	int count = 0;
	while (n > 0)
	{
		count++;
		n = n / 10;
	}
	return count;
}

int getCharCount(char *fn)
{
	int c;
	int count = 0;
	FILE *fp = fopen(fn, "r");

	while (1)
	{
		c = fgetc(fp);

		if (c == EOF || c == '\n')
			break;
		if (!isupper(c) && c != ' ')
		{
			error("File contains bad characters!\n");
		}
		count++;
	}

	fclose(fp);
	return count;
}

void readFile(char *fileContents, char *pTextFile)
{
	FILE *fp;
	/* argv[1] is the plaintext filename */
	fp = fopen(pTextFile, "r");
	if (fp == NULL)
	{
		printf("ERROR\n");
		perror(pTextFile), exit(1);
	}
	fseek(fp, 0, SEEK_END);
	int fSize = ftell(fp);
	rewind(fp);
	fread(fileContents, 1, fSize, fp);
	fclose(fp);
}

/* http://www.strudel.org.uk/itoa/ */
char *myitoa(int value, char *result, int base)
{
	// check that the base if valid
	if (base < 2 || base > 36)
	{
		*result = '\0';
		return result;
	}

	char *ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

/* Error function used for reporting issues */
void error(char *msg)
{
	perror(msg);
	exit(0);
}

/* Set up the address struct */
void setupAddressStruct(struct sockaddr_in *address,
						int portNumber,
						char *hostname)
{

	/* Clear out the address struct */
	memset((char *)address, '\0', sizeof(*address));

	/* The address should be network capable */
	address->sin_family = AF_INET;
	/* Store the port number */
	address->sin_port = htons(portNumber);

	/* Get the DNS entry for this host name */
	struct hostent *hostInfo;
	hostInfo = gethostbyname(hostname);
	if (hostInfo == NULL)
	{
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	/* Copy the first IP address from the DNS entry to sin_addr.s_addr */
	memcpy((char *)&address->sin_addr.s_addr,
		   hostInfo->h_addr_list[0],
		   hostInfo->h_length);
}

void sendallFromFile(char *fn, int socketFD)
{
	int i;

	/* Prepare fileContents buffer and fill it */
	char fileContents[71000];
	int fileLen;
	memset(fileContents, '\0', 71000);
	readFile(fileContents, fn);
	fileLen = strlen(fileContents);

	/* Send TOTAL payload size */
	char buffer[1024];
	char ACKBuffer[20];
	memset(ACKBuffer, '\0', 20);
	memset(buffer, '\0', 1024);
	int currByte = 0;
	char payloadSize[1024];
	memset(payloadSize, '\0', 1024);
	myitoa(fileLen, payloadSize, 10);
	printf("CLIENT: Sending Size of Payload: %s\n", payloadSize);

	/* recv ACK (stops program and waits for data) */
	int bytesSent = send(socketFD, payloadSize, atoi(payloadSize), 0);
	printf("CLIENT: Bytes sent: %d\n", bytesSent);
	recv(socketFD, ACKBuffer, sizeof(ACKBuffer) - 1, 0);
	if (strcmp(ACKBuffer, "ACK") == 0)
	{
		printf("CLIENT: Received ACK of: \'%s\' from the server\n", ACKBuffer);
	}

	/* Start sending data and receiving ACKs while there's still data to send */
	while (currByte < fileLen)
	{
		/* Transfer data from fileContents buffer (large) to payload buffer (small) */
		memset(buffer, '\0', 1024);
		for (i = 0; i < 1023; i++)
		{
			if (fileContents[currByte + i] == '\0')
			{
				break;
			}
			buffer[i] = fileContents[currByte + i];
		}
		buffer[i] = '\0';

		/* Send payload buffer now that it's ready */
		printf("CLIENT: Sending Payload: %s\n", buffer);
		send(socketFD, buffer, sizeof(buffer) - 1, 0);

		/* Await ACK from server */
		memset(ACKBuffer, '\0', 20);
		recv(socketFD, ACKBuffer, 20, 0);
		if (strcmp(ACKBuffer, "ACK") == 0)
		{
			printf("CLIENT: Received ACK:%s from client\n", ACKBuffer);
			currByte += i;
		}
	}
}

int main(int argc, char *argv[])
{
	int i;

	int socketFD, charsRead;
	struct sockaddr_in serverAddress;
	/* Check usage & args */
	if (argc < 4)
	{
		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
		exit(0);
	}
	char *plainTextFile = argv[1];
	char *keyTextFile = argv[2];

	/* Create a socket (internet socket, TCP connection, auto-protocol) */
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0)
	{
		error("CLIENT: ERROR opening socket");
	}
	int y = 1;
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)); //make socket reusable

	/* Set up the server address struct (struct, port, hostname) */
	setupAddressStruct(&serverAddress, atoi(argv[3]), "127.0.0.1");

	/* Connect to server */
	if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	{
		error("CLIENT: ERROR connecting");
	}

	/* Gather "raw"/unread file lengths for both text and key files */
	long rFileLen = getCharCount(plainTextFile);
	long rKeyLen = getCharCount(keyTextFile);
	rFileLen > rKeyLen ? printf("Key is too short!\n") : 1;

	/* Send all of the key to the server */
	sendallFromFile(keyTextFile, socketFD);
	printf("\nCLIENT: Done sending key data to server\n");

	/* Send all of the plain text data to user */
	sendallFromFile(plainTextFile, socketFD);
	printf("\nCLIENT: Done sending text data to server\n");

	/* Receive data from the server, leaving \0 at end */
	/* TODO: Make a more elegant receiving system that limits buffer to 1024*/
	char buffer[71000];
	memset(buffer, '\0', 71000);
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsRead < 0)
	{
		error("CLIENT: ERROR reading from socket");
	}
	printf("CLIENT: Encrypted version of message sent from server: \"%s\"\n", buffer);

	sleep(10);

	/* Close the socket */
	close(socketFD);
	return 0;
}