#pragma once
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define SERVERPORT 9000
#define BUFSIZE    4096
#define MAX_CLIENT 3

enum GameState {
	Accept = 1,
	Robby = 2,
	InGame = 3
};