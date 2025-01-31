Goal
--
본 과목을 수강하는 학생들도 라즈베리파이 실습에 VSCode의 원격개발도구를 활용하는 학생이 많다. 그러나 원격개발도구의

백엔드 프로세스는 임베디드 프로세서에서는 다소 무거운 프로세스일 수 있다. 따라서 본 과제에서는 컴파일할 코드만을 전송

하고 실시간으로 컴파일하여 매우 가벼운 원격개발을 지원하는 원격개발도구를 직접 만들어본다


Overview
--

구현을 구체적으로 생각해보기 위해 다음 시나리오를 참조하자.

1. 자신의 컴퓨터에서 라즈베리파이에서 실행할 코드를 작성한다.
2. 
3. 클라이언트 프로그램에 해당 코드파일과 접속할 라즈베리파이의 IP주소와 포트번호를 실행 아규먼트로 입력한다.
4. 
5. 클라이언트 프로그램이 해당 라즈베리파이의 서버 프로그램와 소켓 연결을 시도 후, 코드 파일을 전송된다.
6. 
7. 서버 프로그램은 코드 파일을 받아 gcc 명령을 통해 컴파일 후 실행 파일을 실행한다.
8. 
9. 이후 클라이언트 프로그램의 stdio, stdout은 마치 자신이 작성한 코드가 실행되는 프로그램의 stdio, stdout을 이용하는 것
처럼 보여야한다.
a. 서버프로그램의 stdin과 stdout을 클라이언트 소켓 스트림에 redirect 한다.

b. 클라이언트 프로그램의 입력을 라즈베리파이의 서버 프로그램에 전송하고, 서버 프로그램으로부터 전달받은 내용을

출력한다.

Requirement
--
**클라이언트 프로그램**
i. 접속할 라즈베리파이의 IP주소, 포트번호, 파일의 경로를 아규먼트로 받는다.

ii. 코드 파일을 서버에 전송한다.

iii. 서버 측에서 전달된 코드의 실행 내용을 출력한다.

iv. 입력을 서버 프로그램으로 전달한다.

v. 서버로부터 EOF를 수신하면 프로그램을 종료한다.

**서버 프로그램**

i. 서버 프로그램은 백엔드로써 작동해야하기 때문에, 한 클라이언트와 연결 수립/해제 이후에도 다시 클라이언트와 연결할 수
있도록 한다.

ii. 클라이언트와 연결 이후 전달 받은 코드파일을 임의의 경로(혹은 서버프로그램이 존재하는 경로)에 저장하고, 해당 파일로
system 함수를 통해 gcc 컴파일 명령을 수행하도록 한다.

iii. 클라이언트 프로그램과 연결된 순간부터 stdout으로 출력되는 메세지를 전부 클라이언트 측 소켓으로 redirect한다.

iv. 클라이언트 프로그램으로 부터 전달 받는 모든 입력을 stdin으로 redirect한다.

v. 클라이언트 연결 해제 이후 시스템 메세지의 출력이 정상적으로 서버에서 출력되도록 redirect된 file descriptor를 다시 원
래대로 돌려놓는다.

vi. 서버에서 실행한 프로그램이 종료되면 클라이언트에 EOF를 송신 후 다시 연결 대기 상태로 돌아간다.
