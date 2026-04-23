/// test.bufferoverflow.cpp
///
/// [목적]
/// 버퍼 오버플로우가 어떻게 프로그램의 실행 흐름을 바꾸는지 보여준다.
///
/// [시나리오]
/// 간단한 로그인 시스템이 있다.
/// - 프로그램이 파일에서 사용자 이름을 읽어온다.
/// - 읽어온 이름을 내부 버퍼에 strcpy로 복사한다.
/// - 정상적인 파일이면 문제없이 동작한다.
/// - 공격자가 조작한 파일이면 오버플로우가 발생하여
///   권한 체크 함수가 "무조건 허용" 함수로 바뀐다.
///
/// [배경 지식 - 스택의 구조]
/// 함수를 호출하면 메모리의 "스택" 영역에 다음 정보가 순서대로 쌓인다:
///
///     [높은 주소]
///     +---------------------------+
///     | 리턴 주소                  |   <- 함수가 끝나면 여기 적힌 주소로 돌아간다
///     +---------------------------+
///     | 지역 변수들                |   <- 함수 안에서 선언한 변수들
///     +---------------------------+
///     [낮은 주소]
///
/// 지역 변수에 데이터를 쓸 때 낮은 주소 -> 높은 주소 방향으로 채워진다.
/// 버퍼 크기를 넘겨서 쓰면 그 뒤에 있는 데이터(리턴 주소 등)를 덮어쓴다.
///
/// [이 데모의 구조]
/// 실제 스택 공격은 OS 보호 기능 때문에 복잡하므로,
/// 같은 원리를 구조체로 시뮬레이션한다.
///
///   struct LoginContext {
///       char username[8];           <- 지역 변수 역할
///       bool (*authCheck)(...)      <- 리턴 주소 역할 (함수 포인터)
///   };
///
///   username에 8바이트보다 긴 데이터를 strcpy하면
///   authCheck 함수 포인터가 덮어써진다.
///
/// [테스트 흐름]
/// 1단계: 정상 입력 파일(admin)로 로그인 -> 정상 허용
/// 2단계: 정상 입력 파일(guest)로 로그인 -> 정상 거부
/// 3단계: 공격자가 조작한 파일로 로그인  -> 권한 체크 우회!
///
/// \date 2026-04-23
/// \author ohsungsik<ohsungsik@outlook.com>

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <Windows.h>

//-----------------------------------------------------------------------------
// 정상적인 권한 체크 함수.
// 이름이 "admin"이면 허용, 아니면 거부한다.
//-----------------------------------------------------------------------------
bool StrictAuthCheck(const char* name)
{
	printf("  [strictAuthCheck] '%s' 권한을 확인합니다...\n", name);

	if (strcmp(name, "admin") == 0)
	{
		printf("  [strictAuthCheck] 관리자 확인됨. 접근 허용.\n");
		return true;
	}

	printf("  [strictAuthCheck] 관리자가 아닙니다. 접근 거부.\n");
	return false;
}

//-----------------------------------------------------------------------------
// 공격자가 실행시키고 싶은 함수.
// 누가 들어오든 무조건 허용한다.
//-----------------------------------------------------------------------------
bool AlwaysAllow(const char* name)
{
	(void)name;
	printf("  [alwaysAllow] *** 권한 체크 우회! 무조건 허용! ***\n");
	return true;
}

//-----------------------------------------------------------------------------
// 로그인 시스템의 내부 구조를 시뮬레이션하는 구조체.
//
// 실제 스택에서 지역 변수(username)와 리턴 주소(authCheck)가
// 나란히 있는 것과 같은 배치이다.
//
//   메모리 배치:
//   [낮은 주소]  username[0]..username[7]  [높은 주소] authCheck
//
//   username에 8바이트보다 많이 쓰면 authCheck가 덮어써진다.
//-----------------------------------------------------------------------------
struct LoginContext
{
	char username[8];                   // 사용자 이름 버퍼 (8바이트)
	bool (*authCheck)(const char*);     // 권한 체크 함수 포인터
};

//-----------------------------------------------------------------------------
// 로그인을 처리하는 함수.
// 파일에서 읽어온 데이터를 username 버퍼에 복사한다.
//
// 취약점: strcpy는 입력 길이를 체크하지 않는다.
// 파일 내용이 8바이트보다 크면 authCheck가 덮어써진다.
//-----------------------------------------------------------------------------
void ProcessLogin(LoginContext* const ctx, const char* const inputFromFile)
{
	printf("  버퍼 크기: 8바이트\n");

	//
	// 여기가 취약점이다!
	// strcpy는 데이터의 길이를 확인하지 않고 그대로 복사한다.
	// 파일에서 읽어온 데이터가 8바이트보다 크면
	// username을 넘어서 authCheck 함수 포인터까지 덮어쓴다.
	//
	strcpy(ctx->username, inputFromFile);
}

//-----------------------------------------------------------------------------
// 파일에 데이터를 쓰는 헬퍼 함수.
// 바이너리 모드로 쓴다. (공격 페이로드에 0x00이 아닌 바이트가 포함될 수 있으므로)
//-----------------------------------------------------------------------------
void WriteFile(const char* const path, const void* const data, const size_t size)
{
	FILE* f = fopen(path, "wb");
	fwrite(data, 1, size, f);
	fclose(f);
}

//-----------------------------------------------------------------------------
// 파일에서 데이터를 읽어오는 함수.
// 실제 프로그램에서 사용자 입력을 파일이나 네트워크에서 읽어오는 것과 같다.
//-----------------------------------------------------------------------------
int readFile(const char* const path, char* const buf, const size_t bufSize)
{
	FILE* file = fopen(path, "rb");
	if (!file) return 0;
	const int n = static_cast<int>(fread(buf, 1, bufSize, file));
	fclose(file);
	buf[n] = '\0';
	return n;
}

int main()
{
	/// 콘솔 출력 코드 페이지를 UTF-8로 설정한다.
	/// 이렇게 하지 않으면 한글이 깨져서 출력된다.
	SetConsoleOutputCP(CP_UTF8);

	char fileData[256];

	///=========================================================================
	/// [준비] 입력 파일 3개를 만든다.
	///=========================================================================
	///
	/// 실제 상황에서는 이 파일들이 이미 존재한다고 가정한다.
	/// - input_admin.bin : 정상 사용자 "admin"
	/// - input_guest.bin : 정상 사용자 "guest"
	/// - input_attack.bin: 공격자가 조작한 파일

	// 정상 입력 파일 2개
	WriteFile("input_admin.bin", "admin", 6);   // "admin" + null
	WriteFile("input_guest.bin", "guest", 6);   // "guest" + null

	/// 공격자가 조작한 파일을 만든다.
	///
	/// 공격자는 이 프로그램을 분석해서 다음을 알아냈다:
	///   - username 버퍼가 8바이트라는 것
	///   - 버퍼 바로 뒤에 함수 포인터가 있다는 것
	///   - alwaysAllow 함수의 메모리 주소
	///
	/// 그래서 파일 내용을 이렇게 구성한다:
	///   [8바이트 아무 값][alwaysAllow의 주소 8바이트]
	///
	/// 프로그램이 이 파일을 읽어서 strcpy하면
	/// 8바이트 뒤의 함수 포인터가 alwaysAllow로 덮어써진다.

	{
		char attackData[32];
		memset(attackData, 'A', 8);                              // 8바이트 채움
		void* addr = (void*)AlwaysAllow;                         // 뒤에 넣을 주소
		memcpy(attackData + 8, &addr, sizeof(void*));            // 주소를 이어붙인다
		attackData[8 + sizeof(void*)] = '\0';                    // null 종료

		WriteFile("input_attack.bin", attackData, 8 + sizeof(void*) + 1);
	}

	printf("입력 파일 3개 생성 완료.\n");
	printf("  input_admin.bin  : 정상 입력 (admin)\n");
	printf("  input_guest.bin  : 정상 입력 (guest)\n");
	printf("  input_attack.bin : 공격자가 조작한 입력\n\n");

	///=========================================================================
	/// [1단계] 정상 파일로 로그인 - admin
	///=========================================================================
	///
	/// input_admin.bin 에서 "admin"을 읽어온다.
	/// 5바이트 + null = 6바이트. 버퍼(8바이트)에 충분히 들어간다.
	/// authCheck는 건드리지 않는다.
	
	printf("=== 1단계: 정상 파일로 로그인 (admin) ===\n\n");

	LoginContext ctx1;
	ctx1.authCheck = StrictAuthCheck;

	readFile("input_admin.bin", fileData, sizeof(fileData));
	printf("  파일에서 읽은 값: \"%s\" (%zu바이트)\n", fileData, strlen(fileData));

	ProcessLogin(&ctx1, fileData);

	printf("  authCheck 변조: 없음\n");
	bool allowed = ctx1.authCheck(ctx1.username);
	printf("  결과: %s\n\n", allowed ? "접근 허용" : "접근 거부");

	///=========================================================================
	/// [2단계] 정상 파일로 로그인 - guest
	///=========================================================================
	///
	/// input_guest.bin 에서 "guest"를 읽어온다.
	/// 역시 버퍼 범위 내이므로 오버플로우 없음.
	/// strictAuthCheck가 "admin"이 아니므로 거부한다.
	
	printf("=== 2단계: 정상 파일로 로그인 (guest) ===\n\n");

	LoginContext ctx2;
	ctx2.authCheck = StrictAuthCheck;

	readFile("input_guest.bin", fileData, sizeof(fileData));
	printf("  파일에서 읽은 값: \"%s\" (%zu바이트)\n", fileData, strlen(fileData));

	ProcessLogin(&ctx2, fileData);

	printf("  authCheck 변조: 없음\n");
	allowed = ctx2.authCheck(ctx2.username);
	printf("  결과: %s\n\n", allowed ? "접근 허용" : "접근 거부");

	///=========================================================================
	/// [3단계] 공격자가 조작한 파일로 로그인
	///=========================================================================
	///
	/// input_attack.bin 에서 데이터를 읽어온다.
	/// 이 파일에는 8바이트 쓰레기 + alwaysAllow 주소가 들어있다.
	///
	/// 프로그램은 이것이 정상적인 사용자 이름이라고 생각하고
	/// strcpy로 버퍼에 복사한다.
	/// 하지만 데이터가 8바이트를 넘기 때문에
	/// authCheck 함수 포인터가 alwaysAllow로 덮어써진다.

	printf("=== 3단계: 공격자가 조작한 파일로 로그인 ===\n\n");

	LoginContext ctx3;
	ctx3.authCheck = StrictAuthCheck;

	printf("  공격 전 authCheck: %p (strictAuthCheck)\n", ctx3.authCheck);

	const int bytesRead = readFile("input_attack.bin", fileData, sizeof(fileData));
	printf("  파일에서 읽은 크기: %d바이트 (버퍼 8바이트를 초과!)\n", bytesRead);

	ProcessLogin(&ctx3, fileData);

	printf("  공격 후 authCheck: %p (alwaysAllow = %p)\n", ctx3.authCheck, AlwaysAllow);
	printf("  authCheck 변조: 발생!\n");
	allowed = ctx3.authCheck("hacker");
	printf("  결과: %s\n\n", allowed ? "접근 허용" : "접근 거부");

	//=========================================================================
	// [정리]
	//=========================================================================

	printf("=== 정리 ===\n\n");
	printf("이 데모에서               실제 공격에서\n");
	printf("---------------------     ----------------------------\n");
	printf("username[8]               함수의 지역 변수 (스택)\n");
	printf("authCheck 함수포인터      스택의 리턴 주소\n");
	printf("strcpy(ctx->username)     strcpy, gets 등 취약한 함수\n");
	printf("alwaysAllow               공격자의 악성 코드 (shellcode)\n");
	printf("input_attack.bin          조작된 네트워크 패킷, 파일 등\n");

	return 0;
}
