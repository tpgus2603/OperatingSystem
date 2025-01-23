#include<stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>


#define PORT 24210 // 포트 번호

void error_handling(char* message) { //에러처리 코드 
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
//파일디스크립터(fd): 정수로표현되는 i/o리소스를 식벽하기 위한 고유정수값,프로세스가 파일또는 i/o에 접근할떄 사용용
//리눅스에서 stdin은 0 stdout는 1로 나타남 
int main(void){
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr; // 소켓 주소를 위한 구조체 생성 
	socklen_t client_addr_size;
	server_sock = socket(PF_INET, SOCK_STREAM, 0);// 서버 소켓 생성
	char buf[1024];
 	if (server_sock == -1) {
		error_handling("socket() error");
	}

    //서버 소켓 주소 설정 
    memset(&server_addr, 0, sizeof(server_addr)); //메모리 블록을 0으로 초기화 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//서버구조체에 네트워크 ip주소 할당 
    server_addr.sin_port = htons(PORT); //서버의 포트번호 할당 

		//서 버 소켓과 서버 주소를 바인딩
    if(bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
		error_handling("bind() error");
    	// 클라이언트 요청 대기
    if (listen(server_sock, 5) == -1) 
	    error_handling("listen() error\n");
        
    while (1) //서버가 계속해서 클라이언트의 요청을 받도록 무한루프를 돔 
	{
		int len;
		int writefd = open("temp.c", O_WRONLY | O_CREAT | O_TRUNC , 0644); // 코드를 저장할 임시 파일 생성 후 fd 저장
		pid_t pid; // 프로세스아이디 저장
		printf("[*] wait for client ...\n");

		// accept과정
		client_addr_size = sizeof(client_addr); 
		client_sock= accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);//클라이언트 연결 수락 +클라이언트 소켓 생성
		if (client_sock == -1) 
			error_handling("accpet() error");
		printf("[*] client connected\n");
		do
		{
		    len = read(client_sock, buf, 1024); //클라이언트 소켓으로부터 코드를 읽고 
		}while(write(writefd, buf, len) >= 1024); //temp.c 파일에 다시 쓰는 작업 1024바이트 단위로 반복
		close(writefd); // 파일디스크립터를 원래대로 돌려놓음

		pid = fork(); //현재 프로세스 복제 후 자식 프로세스 생성  pid가 0이면 자식 
		if(pid)   //부모 프로세스
		{
			char eof = EOF;//eof값 저장 
			waitpid(pid, NULL, 0); // 프로세스 스케쥴링 pid의 process id를 가진 프로세스(즉 자식 프로세스)가 종료되길 기다림
			write(client_sock, &eof, 1); // 서버 프로그램이 종료될때 client에게 eof를 송신
			close(client_sock); // 그리고 client를 종료한다
			printf("[*] session closed\n");
		}else//자식 프로세스  
		{ // 임시코드 컴파일 +리다이렉트  
			system("gcc temp.c"); // temp.c 컴파일 

			//리다이렉트 과정+ 소스코드 실행
			dup2(client_sock, 0); // 소켓의 fd를 표준입력에 복제 클라이언트가 입력가능 
			dup2(client_sock, 1); // 소켓의 fd를 표준 출력에 복제 클라이언트가 출력가능
			execl("a.out", NULL); //기본 출력파일인 a.out를 이용, 기존 프로세스 삭제 후 컴파일된 temp실행 
		}
    }
    	close(server_sock);//서버소켓 종료
    	return 0;
}


