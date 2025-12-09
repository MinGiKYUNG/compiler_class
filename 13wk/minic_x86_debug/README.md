# MiniC x86 디버그 버전

- `parser/lexer.l`, `parser/parser.y` 로 분리.
- `int add(int a, int b)` 같은 **2개 인자 함수 호출**을 파싱해서
- 바로 x86-64 SysV 어셈블리를 출력하는 간단 컴파일러입니다.
- **디버그 정보 출력 기능이 추가**되었습니다.

## 빌드

### Linux/Mac
```bash
make
```

### Windows
```bash
# 자동 OS 감지 (Windows 환경에서)
make

# 또는 명시적으로 Windows 타겟 지정
make windows
```

### 필요한 도구
- **Linux/Mac**: `flex`, `bison`, `gcc`
- **Windows**: `flex`, `bison`, `gcc` (MinGW/MSYS2 환경)

## 사용법

### 기본 사용법

**Linux/Mac:**
```bash
./minic_x86 test/add2.minic > out.s
gcc -no-pie -o out out.s
./out
echo $?
```

**Windows:**
```bash
minic_x86.exe test/add2.minic > out.s
gcc -no-pie -o out.exe out.s
out.exe
echo %ERRORLEVEL%
```

### 디버그 정보와 함께 컴파일

**Linux/Mac:**
```bash
# 디버그 정보를 터미널에 직접 출력
./minic_x86 test/factorial.minic > out.s

# 디버그 정보를 파일로 저장
./minic_x86 test/factorial.minic 2>debug.log > out.s

# 디버그 정보와 어셈블리 코드를 모두 터미널에 출력
./minic_x86 test/factorial.minic | tee out.s

# 디버그 정보를 실시간으로 보면서 어셈블리 코드는 파일로 저장
./minic_x86 test/factorial.minic 2>&1 | tee debug.log > out.s
```

**Windows:**
```bash
# 디버그 정보를 터미널에 직접 출력
minic_x86.exe test/factorial.minic > out.s

# 디버그 정보를 파일로 저장
minic_x86.exe test/factorial.minic 2>debug.log > out.s

# 디버그 정보와 어셈블리 코드를 모두 터미널에 출력 (Windows에서는 type 명령어 사용)
minic_x86.exe test/factorial.minic > out.s & type out.s
```

디버그 정보는 `stderr`로 출력되므로 `2>debug.log`로 저장할 수 있습니다.

### 디버그 정보 확인하기

**Linux/Mac:**
```bash
# 디버그 로그 파일 내용 확인
cat debug.log

# 실시간으로 디버그 정보 보면서 컴파일
./minic_x86 test/factorial.minic 2>&1 | tee debug.log > out.s

# 생성된 어셈블리 코드 확인
cat out.s

# 실행 파일 생성 및 실행
gcc -no-pie -o prog out.s
./prog
echo $?  # 프로그램 반환값 확인
```

**Windows:**
```bash
# 디버그 로그 파일 내용 확인
type debug.log

# 생성된 어셈블리 코드 확인
type out.s

# 실행 파일 생성 및 실행
gcc -no-pie -o prog.exe out.s
prog.exe
echo %ERRORLEVEL%  # 프로그램 반환값 확인
```

## 테스트 파일

- `test/add2.minic` : 2개 인자를 받는 `add(a, b)` 예제
- `test/factorial.minic` : 재귀 함수를 이용한 팩토리얼 계산 예제
- `test/multi_ops.minic` : 여러 연산과 함수 호출을 조합한 예제
- `test/nested_calls.minic` : 중첩된 함수 호출 예제

## 디버그 기능

- `DEBUG` 매크로가 정의되어 있을 때 디버그 정보가 출력됩니다.
- Makefile의 `CFLAGS`에 `-DDEBUG` 옵션이 추가되어 있습니다.
- 디버그 정보는 다음과 같은 내용을 포함합니다:
  - 함수 생성 시작/종료
  - 변수 할당 정보 (이름, 오프셋)
  - 함수 호출 정보
  - 변수 로드 정보

## 폴더 구조

- `parser/lexer.l` : Flex lexer
- `parser/parser.y` : Bison parser (함수/호출, 2개 인자 이상도 문법상 지원)
- `include/ast.h` : AST 정의
- `include/codegen_x86.h` : x86 코드 생성기 인터페이스
- `src/ast.c` : AST 생성 함수
- `src/codegen_x86.c` : 매우 단순한 x86-64 코드 생성기 (디버그 정보 출력 추가)
- `src/main.c` : 엔트리, `yyparse()` → `gen_x86_program()` (디버그 정보 출력 추가)
- `test/` : 테스트용 샘플 minic 파일들

## 12wk/minic_x86_call_func2와의 차이점

1. **디버그 옵션 추가**: Makefile에 `-DDEBUG` 플래그 추가
2. **디버그 정보 출력**: 코드 생성 과정에서 상세한 디버그 정보 출력
3. **추가 테스트 파일**: factorial, multi_ops, nested_calls 예제 추가
4. **개선된 에러 처리**: 더 상세한 디버그 정보 제공
