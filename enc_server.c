#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Error function used for reporting issues */
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/* Set up the address struct for the server socket */
void setupAddressStruct(struct sockaddr_in *address,
						int portNumber)
{

	/* Clear out the address struct */
	memset((char *)address, '\0', sizeof(*address));

	/* The address should be network capable */
	address->sin_family = AF_INET;
	/* Store the port number */
	address->sin_port = htons(portNumber);
	/* Allow a client at any address to connect to this server */
	address->sin_addr.s_addr = INADDR_ANY;
}

/* Assumes toEncrypt is okay to be overwritten (expected empty) */
void encrypt(char *toEncrypt, int toEncryptSize, char *raw, char *key)
{
	if (toEncryptSize < strlen(raw))
	{
		printf("CLIENT: ERROR: Encryption buffer size smaller than raw data.\n");
	}
	memset(toEncrypt, '\0', toEncryptSize);
	int i;
	for (i = 0; i < strlen(raw); i++)
	{
		char encryptedCharCode = ((raw[i] + key[i]) % 26) + 'A';
		toEncrypt[i] = encryptedCharCode;
	}
	printf("\nCLIENT: Encrypted message: %s\n", toEncrypt);
}

void sendACK(int connectionSocket)
{
	char ACKBuffer[20];
	memset(ACKBuffer, '\0', 20);
	strcat(ACKBuffer, "ACK");
	printf("SERVER: Sending ACK:%s to client\n", ACKBuffer);
	send(connectionSocket, ACKBuffer, strlen(ACKBuffer), 0);
}

void receiveData(char *payload, int connectionSocket)
{
	int i;
	char pBuff[6];
	memset(pBuff, '\0', 6);
	/* First recv() should be payload size */
	printf("SERVER: Attempting to receive data...\n");
	int charsRead = recv(connectionSocket, pBuff, 5, 0);
	if (charsRead < 0)
	{
		perror("enc_server");
		printf("ERROR with read\n");
	}
	else
	{
		/* Sending ACK to client */
		printf("SERVER: Received: %s\n", pBuff);
		printf("SERVER: CHARS READ: %d\n", charsRead);
		sendACK(connectionSocket);
	}
	int currByte, payloadSize;
	payloadSize = atoi(pBuff);
	printf("SERVER: Received Size of Payload: %d\n", payloadSize);
	currByte = 0;

	/* retrieve payload */
	char buffer[1024];
	memset(buffer, '\0', 1024);
	memset(payload, '\0', 71000);
	while (currByte < payloadSize)
	{
		memset(buffer, '\0', 1024);
		int chars = recv(connectionSocket, buffer, 1024, 0);
		if (chars < 0)
		{
			printf("SERVER: ERROR retrieving payload\n");
		}
		else
		{
			currByte += chars;
			sendACK(connectionSocket);
		}
		strcat(payload, buffer);
		printf("SERVER: Received: %s\n", buffer);
	}
}

int main(int argc, char *argv[])
{
	int i;

	int connectionSocket, charsRead;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);

	/* Check usage & args */
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	/* Create the socket that will listen for connections */
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0)
	{
		error("ERROR opening socket");
	}

	/* Set up the address struct for the server socket */
	setupAddressStruct(&serverAddress, atoi(argv[1]));

	/* Associate the socket to the port */
	if (bind(listenSocket,
			 (struct sockaddr *)&serverAddress,
			 sizeof(serverAddress)) < 0)
	{
		error("ERROR on binding");
	}

	/* Start listening for connetions. Allow up to 5 connections to queue up */
	listen(listenSocket, 5);

	/* Accept a connection, blocking if one is not available until one connects */
	while (1)
	{
		/* Accept the connection request which creates a connection socket */
		connectionSocket = accept(listenSocket,
								  (struct sockaddr *)&clientAddress,
								  &sizeOfClientInfo);
		if (connectionSocket < 0)
		{
			error("ERROR on accept");
		}
		int pid = fork();
		switch (pid)
		{
		case -1:
			error("ERROR Creating Fork");
			break;
		case 0:
			printf("SERVER: Connected to client running at host %d port %d\n",
				   ntohs(clientAddress.sin_addr.s_addr),
				   ntohs(clientAddress.sin_port));
			/* Receive key from client */
			char key[71000];
			memset(key, '\0', 71000);
			receiveData(key, connectionSocket);
			printf("SERVER: Confirming key: %s\n", key);

			/* Receive message from client */
			char message[71000];
			memset(message, '\0', 71000);
			receiveData(message, connectionSocket);
			printf("SERVER: Confirming message: %s\n", message);

			/* Encrypt message from client and store it in new buffer */
			char encryptedMessage[71000];
			memset(encryptedMessage, '\0', 71000);
			encrypt(encryptedMessage, strlen(message), message, key);

			/* Send a Success message back to the client */
			/* TODO: SEND BACK ENCRYPTED TEXT (Do it in a more elegant way, 1024 buffers) */
			charsRead = send(connectionSocket,
							 encryptedMessage, strlen(encryptedMessage), 0);
			if (charsRead < 0)
			{
				error("ERROR writing to socket");
			}
			/* Close the connection socket for this client */
			close(connectionSocket);
			exit(0);
		default:
			break;
		}
	}
	/* Close the listening socket */
	close(listenSocket);
	return 0;
}
