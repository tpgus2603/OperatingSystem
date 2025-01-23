#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>



void error_handling(char* message) { //에러처리 코드 
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


void *input(void *serverSock) {  //함수포인터 
	int s_input = (int)((uint64_t)serverSock);  // 인자를 받아서 인트형으로 저장
	while(1)  //infite loop
	{	
		int ch = getchar(); 
		write(s_input, &ch, 1);// write to serversocket one byte 
	}
}
void *output(void *serverSock) {
	int s_output = (int)((uint64_t)serverSock); //  인자를 받아서 인트형으로 저장
	while(1)
	{
		char ch;
		read(s_output, &ch, 1); // read one byte . EOF라면 프로그램을 종료함
		if (ch == EOF)
			return 0;
		else
			putchar(ch); //else  output
	}
}

int main(int argc, char * argv[])
{
    printf("input two number: ");
	int sock = socket(PF_INET, SOCK_STREAM, 0);//클라이언트 소켓생성
	int code;
	int len;
	char buf[1024];
	struct sockaddr_in server_addr; //서버 주소 구조체 생성
	if (argc != 4)
	{
		printf("Usage : %s <IP> <Port> <source code>\n", argv[0]);  //에러메세지 
		return -1;
	}
	
	if (sock == -1) 
		error_handling("sock error");
   

	//소켓 설정  
	memset(&server_addr, 0, sizeof(server_addr));  //메모리 블록 0으로 초기화 
    server_addr.sin_family = AF_INET; //주소 체계를 나타내는 상수
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);  // argv1로 전달된 주소를 변환 후 서버 ip주소 설정
    server_addr.sin_port = htons(atoi(argv[2]));  // argv2로 전잘된 포트번호를 변환후 서버포트 번호로 설정 

	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) //  
		error_handling("connect() error");

	code = open(argv[3], O_RDONLY);//argv3으로 전달된 코드파일을 open 
	do
	{
	len = read(code, buf, 1024);  //code read 
	}while(write(sock, buf, len) >= 1024); // 코드를 server에 전달

	fsync(sock); // 소켓 동기화 출력버퍼에 대기중인 모든데이터가 전달되도록  

	pthread_t input_worker, output_worker; //쓰레드 변수 
	pthread_create(&input_worker, NULL, input, (void*)((uint64_t)sock)); // input 역할을 하는 쓰레드 생성 input함수를 실행 sock을 input함수에 전달 
	pthread_create(&output_worker, NULL, output, (void*)((uint64_t)sock)); // output thread의 생성 output함수 실행 sock을 output함수에 전달 
 	pthread_join(output_worker, NULL); // output thread 종료될때까지 기다림 
	
    printf("[*] session closed\n");
	exit(0); 
}
