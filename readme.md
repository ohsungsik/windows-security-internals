# test.executefrommemory

여러 보안 취약점, OS 동작 원리 등의 이해를 위한 테스트 프로젝트.


## 하위 항목

### executefrommemory/

메모리에 기계어 코드를 써넣고 실행할 수 있는지 테스트한다.
DEP(Data Execution Prevention)가 이를 어떻게 차단하는지,
그리고 VirtualProtect로 실행 권한을 부여하면 어떻게 우회되는지 확인한다.

### bufferoverflow/

버퍼 오버플로우가 프로그램의 실행 흐름을 어떻게 바꾸는지 보여준다.
strcpy 같은 길이 체크 없는 함수로 버퍼를 넘겨 쓰면
바로 뒤에 있는 함수 포인터(실제로는 리턴 주소)가 덮어써져서
공격자가 원하는 코드가 실행되는 과정을 시뮬레이션한다.

## 빌드

Visual Studio C++ 도구가 설치되어 있어야 한다.

```
build.bat
```

각 하위 폴더의 cpp 파일을 자동으로 찾아서 빌드한다.
새 폴더를 추가하면 별도 수정 없이 빌드 대상에 포함된다.
