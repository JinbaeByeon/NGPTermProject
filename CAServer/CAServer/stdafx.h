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
#define BUFSIZE    128
#define MAX_CLIENT 3