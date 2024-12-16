// Copyright 2024 Lieson Mwale

#define _CRT_SECURE_NO_WARNINGS  
#include <stdio.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS  
#include <winsock2.h>
#include <string.h>  
#pragma comment(lib, "ws2_32.lib")  

#define SERVER_IP "172.18.182.131"  // IP-адрес сервера
#define CLIENT_PORT 27015  // Заданная порта клиента
#define BUFFER_SIZE 512  // Размер буфера для отправки и получения данных

int main(int argc, char *argv[]) {
    // Проверяем, передан ли порт сервера как аргумент командной строки
    if (argc != 2) {
        printf("Usage: client.exe <SERVER_PORT>\n");  
        return 1;
    }

    // Парсим порт сервера из аргументов командной строки
    int serverPort = atoi(argv[1]);  // Преобразуем строку в число
    if (serverPort <= 0 || serverPort > 65535) {  // Проверяем допустимость порта
        printf("Invalid port number. Must be within (1-65535).\n");
        return 1;
    }

    // Инициализация Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);  
    if (iResult != NO_ERROR) {  // Проверяем успешность инициализации
        printf("Error at WSAStartup()\n");
        return 1;
    }

    // Создание UDP-сокета
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
    if (udpSocket == INVALID_SOCKET) {  // Проверка на ошибку создания сокета
        printf("Error creating socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Привязываем сокет к определённому порту клиента
    struct sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;  // Используем IPv4
    clientAddr.sin_addr.s_addr = INADDR_ANY;  
    clientAddr.sin_port = htons(CLIENT_PORT);  // Задаём порт клиента

    if (bind(udpSocket, (struct sockaddr*)&clientAddr, 
    sizeof(clientAddr)) == SOCKET_ERROR) {
        // Проверяем успешность привязки сокета
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }

    // Настраиваем адрес сервера, используя порт, переданный в аргументах
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);  
    serverAddr.sin_port = htons(serverPort);  

    // Получаем данные от пользователя
    char word[256], filename[256];  // Буферы для ввода слова и имени файла
    printf("Enter the word to search for: ");  // Ввести слово
    scanf("%255s", word);  
    printf("Enter the text file name on the server: ");  //Ввести имя файла
    scanf("%255s", filename);

    // Формируем сообщение для отправки на сервер
    char sendbuf[BUFFER_SIZE];
    snprintf(sendbuf, sizeof(sendbuf), "%s %s", word, filename);  

    // Отправляем данные на сервер
    int bytesSent = sendto(
        udpSocket,
        sendbuf,
        (int)strlen(sendbuf),
        0,
        (struct sockaddr*)&serverAddr,
        sizeof(serverAddr)
    );

    if (bytesSent == SOCKET_ERROR) {  // Проверка на успешность отправки
        printf("Send failed with error: %d\n", WSAGetLastError());
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }
    printf("Bytes Sent: %d\n", bytesSent);  

    // Получаем данные от сервера (количество вхождений слова)
    char recvbuf[BUFFER_SIZE] = { 0 };  // Буфер для полученных данных
    int serverAddrLen = sizeof(serverAddr);
    int bytesRecv = recvfrom(
        udpSocket,
        recvbuf,
        BUFFER_SIZE - 1,
        0,
        (struct sockaddr*)&serverAddr,
        &serverAddrLen
    );

    if (bytesRecv == SOCKET_ERROR) {  
        printf("Recv failed with error: %d\n", WSAGetLastError());
    } else {
        recvbuf[bytesRecv] = '\0';  
        printf("Bytes Received: %d\n", bytesRecv);  
        printf("Word Repetitions: %s\n", recvbuf);  
    }

    // Завершаем работу
    closesocket(udpSocket);  // Закрываем сокет
    WSACleanup();  // Очищаем ресурсы Winsock
    return 0;
}
