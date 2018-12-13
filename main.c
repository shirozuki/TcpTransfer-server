#include "server.h"

void awaitCommand(int socket)
{
    char command[1];
    int endConnection = 0;

    while(endConnection == 0)
    {
        memset(command, 0, 1);
        sleep(1);

        recv(socket, &command, sizeof(command), 0);

        switch(command[0])
        {
        case '1':
            printf("Klient wysłał żądanie pobrania pliku\n");
            sendFile(socket);
            break;
        case '2':
            printf("Klient wysłał żądanie udostępnienia listy plików i katalogów\n");
            listFiles(socket);
            break;
        case '3':
            printf("Klient zakończył połączenie\n");
            endConnection = 1;
            close(socket);
            break;
        default:
            continue;

        }
    }
}

void sendFile(int socket)
{
    printf("Serwer w trybie przesyłania pliku do klienta.\n");
    send(socket, "1", sizeof("1"), 0);

    sleep(1);

    FILE *fp;
    long long fileSize, dataRead, sessionSentData, allSentData;
    char c_fileSize[32] = { 0 };
    char delivered[1] = { 0 };
    char fileName[128];
    char fullPath[256];
    char buffer[BUFFER_SIZE];
    struct stat fileinfo;

    memset(delivered, 0, 1);
    memset(fileName, 0, 128);
    memset(fullPath, 0, 256);

    recv(socket, &fileName, sizeof(fileName), 0);

    sleep(1);

    snprintf(fullPath, sizeof(fullPath), "%s%s", "./share/", fileName);

    printf("Żądanie klienta: %s\n", fileName);
    printf("Pod ścieżką: %s\n", fullPath);

    if(access(fullPath, F_OK) != -1)
    {
        printf("Plik znaleziony!\n");
        send(socket, "1", sizeof("1"), 0);
    }

    else
    {
        printf("Klient zażądał nieistniejącego pliku!\n");
        send(socket, "0", sizeof("0"), 0);
        return;
    }

    sleep(1);

    if(stat(fullPath, &fileinfo) < 0)
    {
        printf("Błąd w odczycie parametrów pliku!\nPlik uszkodzony lub odmowa dostępu!\n");
    }

    fp = fopen(fullPath, "rb");

    fileSize = fileinfo.st_size;
    printf("Rozmiar pliku: %lld bajtów.\n", fileSize);

    dataRead = sessionSentData = allSentData = 0;

    sprintf(c_fileSize, "%lld", fileSize);
    send(socket, &c_fileSize, strlen(c_fileSize), 0);

    memset(delivered, 0, 1);
    recv(socket, &delivered, sizeof(delivered), 0);

    if(delivered[0] == 'K')
    {
        printf("Klient prawidłowo odebrał wielkość pliku.\n");
    }

    else
    {
        printf("Klient opuścił tryb pobierania pliku.\n");
        close(socket);
        exit(EXIT_FAILURE);
    }

    sleep(3);

    printf("Wysyłam plik: %s...\n", fileName);

    while(allSentData < fileSize)
    {
        memset(buffer, 0, BUFFER_SIZE);
        memset(delivered, 0, 1);
        dataRead = fread(buffer, 1, BUFFER_SIZE, fp);
        sessionSentData = send(socket, buffer, dataRead, 0);
        recv(socket, &delivered, sizeof(delivered), 0);

        if(delivered[0] != 'K')
        {
            printf("Błąd przesyłania pliku!\n");
            exit(EXIT_FAILURE);
            break;
        }

        else
        {

        }

        if(dataRead != sessionSentData)
        {
            printf("Błąd odczytu pliku!\n");
            break;
        }

        allSentData += sessionSentData;
    }

    printf("\nŁącznie przesłano %lld z %lld bajtów.\n", allSentData, fileSize);

    if(allSentData == fileSize)
    {
        printf("Plik przesłany poprawnie!\n");
    }

    else
    {
        printf("Błąd przy przesyłaniu pliku!\n");
    }

    fclose(fp);

    printf("Czekam na poprawne rozłączenie klienta...\n");
    sleep(3);

    printf("Zamykam instancję.\n");
    close(socket);
    exit(EXIT_SUCCESS);
}

void listFiles(int socket)
{
    printf("Serwer w trybie listowania katalogów dla klienta.\n");
    send(socket, "1", sizeof("1"), 0);

    char fil[256];
    struct dirent *de;
    DIR *dr = opendir("./share/");

    if(dr == NULL)
    {
        perror("");
    }

    while((de = readdir(dr)) != NULL)
    {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
        {

        }

        else
        {
            memset(fil, 0, 256);
            strncpy(fil, de->d_name, sizeof(de->d_name));
            memcpy(fil+strlen(de->d_name), "\n", sizeof("\n"));
            memcpy(fil+(strlen(de->d_name)+1), "\0", sizeof("\0"));
            send(socket, fil, strlen(fil), 0);
        }

    }

    closedir(dr);

    printf("Zamykam instancję.\n");
    close(socket);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int srvSock, cliSock;
    struct sockaddr_in addr;
    socklen_t addr_length;

    srvSock = socket(PF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = PORT;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr_length = sizeof(struct sockaddr_in);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    pid_t child;
    int chunkSize = CHUNK_SIZE;
    int bindSig = 0, listenSig = 0;
    int sock1Sig = 0, sock2Sig = 0, sock3Sig = 0, sock4Sig = 0;

    bindSig = bind(srvSock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if(bindSig < 0)
    {
        perror("Błąd");
        return 1;
    }

    listenSig = listen(srvSock, 5);
    if(listenSig < 0)
    {
        perror("Błąd");
        return 1;
    }

    printf("Serwer uruchomiony!\n"
           "Nasłuchuję na porcie: %u\n", htons(PORT));

    while(true)
    {
        cliSock = accept(srvSock, (struct sockaddr *) &addr, &addr_length);

        if(cliSock <= 0)
        {
            perror("Problem z nawiązaniem połączenia");
            continue;
        }

        printf("Ustanowiono połączenie z: %s:%u\n", inet_ntoa(addr.sin_addr), htons(PORT));

        sock1Sig = setsockopt(cliSock, SOL_SOCKET, SO_RCVBUF, (const void *) &chunkSize, sizeof(chunkSize));
        sock2Sig = setsockopt(cliSock, SOL_SOCKET, SO_SNDBUF, (const void *) &chunkSize, sizeof(chunkSize));
        sock3Sig = setsockopt(srvSock, SOL_SOCKET, SO_RCVBUF, (const void *) &chunkSize, sizeof(chunkSize));
        sock4Sig = setsockopt(srvSock, SOL_SOCKET, SO_SNDBUF, (const void *) &chunkSize, sizeof(chunkSize));

        if((sock1Sig != 0) || (sock2Sig != 0) ||
           (sock3Sig != 0) || (sock4Sig != 0))
        {
            printf("Nie można ustawić parametrów gniazda!\n");
            close(cliSock);
            close(srvSock);
            exit(EXIT_FAILURE);
        }

        printf("Ustawianie parametrów gniazda zakończone pomyślnie!\n");

        child = fork();

        if(child == 0)
        {
            printf("Przechodzę do obsługi klienta...\n");

            awaitCommand(cliSock);
            close(cliSock);
            exit(EXIT_SUCCESS);
        }

        else
        {
            continue;
        }
    }

    return 0;
}
