#include<iostream>
#include<stdio.h>
#include <memory.h>
using namespace std;
int main(){
for(int i = 0; i<1024*30; i++)
{

    char *p = new char[1024*1024];
    memset(p, 0, 1024*1024);
}
 getchar();
}
