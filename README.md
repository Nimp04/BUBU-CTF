# 입력 검증형 리버싱 문제

소스는 `challenge.c` 한 파일입니다. 같은 파일을 Windows와 Linux에서 각각 컴파일합니다.

`main`은 아래 순서로 함수를 호출합니다. 각 함수와 계산식에는 한국어 주석이 있습니다.

1. `read_input`: 입력 배열에 32바이트를 직접 읽고 길이를 검사합니다.
2. `rotate_blocks`: 8바이트씩 네 블록으로 보고, 인덱스 mod 4만큼 블록을 이동합니다.
3. `xor_by_index`: 이동 후 인덱스 mod 8에 따라 switch-case에서 XOR합니다.
4. `rotate_nibbles`: 각 바이트의 상위 4비트와 하위 4비트를 바꿉니다.
5. `compare_target`: memcmp로 32바이트를 비교합니다.

타깃은 원본 플래그에 위 변환을 적용한 값이며 main의 지역 배열입니다. 변환 규칙은 그대로 유지하고, 아래 새 플래그에 맞춰 타깃과 솔버 상수를 갱신했습니다. 성공하면 Correct, 실패하면 Wrong을 출력합니다.

```text
BUBU{P3rm_X0R_Sw4p_4r3_Th3_K3ys}
```

정확히 32바이트입니다. P3rm은 블록 간 재배치, X0R은 XOR, Sw4p은 상·하위 4비트 교환을 뜻합니다. 자세한 풀이와 최신 상수는 `WRITEUP.md`, 복원 코드는 `solve.py`에 있습니다.

입력은 줄바꿈을 제외하고 정확히 32바이트여야 합니다. LF, CRLF, 입력 직후 EOF를 지원합니다. Windows에서 입력이 자동 변환되지 않도록 하는 설정만 짧은 `#ifdef _WIN32` 안에 있습니다.

## Windows 빌드

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
gcc -O0 -Wall challenge.c -o challenge_windows.exe
```

## Linux 빌드

```sh
gcc -O0 -Wall challenge.c -o challenge_linux
```

최적화는 끄고 함수 이름은 남깁니다. 별도 소스 파일, 연결용 함수, 추가 최적화 옵션은 없습니다.

## 확인

`python solve.py` 또는 `python3 solve.py`로 새 플래그를 복원합니다. 솔버에는 복원한 입력의 순방향 변환이 타깃과 같은지 확인하는 assert가 포함되어 있습니다.

Windows에서는 `python solve.py | .\challenge_windows.exe`, Linux에서는 `python3 solve.py | ./challenge_linux`로 Correct를 확인할 수 있습니다. 이전 플래그는 Wrong입니다.

README와 Write-up에는 정답이 있으므로 참가자에게는 실행 파일만 제공합니다.
