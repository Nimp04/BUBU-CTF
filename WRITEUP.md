# 입력 검증형 리버싱 Write-up

실행 파일은 입력을 블록 이동, XOR, 4비트 회전 순서로 변환한 뒤 타깃 배열과 비교한다. 타깃에서 이 연산들을 역순으로 되돌리면 정답을 얻을 수 있다.

## 분석 자료와 필요한 추가 자료

주요 분석 근거는 제공된 IDA 디컴파일의 `main`, `rotate_blocks`, `xor_by_index`, `rotate_nibbles`다. 여기에는 타깃 상수, 함수 호출 순서, 각 변환의 계산식이 모두 들어 있다. 따라서 **현재 복사본만으로도 복원 알고리즘과 솔버를 작성할 수 있다.**

플래그 변경에 따라 타깃 32바이트와 그에 대응하는 QWORD 상수를 모두 갱신했다. 아래 main 발췌는 기존 IDA의 구조에 최신 상수를 반영한 설명용 재구성이다. 새 실행 파일을 IDA에서 다시 디컴파일해 얻은 원문은 아니다. 변환식은 변경하지 않았다.

다만 `read_input`과 `compare_target`의 본문은 제공되지 않았다. IDA 복사본만을 근거로 입력 길이 검사나 비교 범위를 단정할 수는 없다. 32번 도는 변환 루프가 있다는 사실만으로, 33바이트 입력을 거부한다고 증명할 수 있는 것은 아니다.

이 두 사항은 현재 공유된 `challenge.c`에서 보완 확인했다. 입력은 줄바꿈을 제외한 32바이트여야 하고, 비교는 `memcmp(data, target, 32) == 0`이다. 복원한 입력은 Windows와 Linux 실행 파일에서도 `Correct`를 출력했다.

소스를 보지 않는 완전한 IDA 분석 기록으로 작성하려면 `read_input`과 `compare_target`의 디컴파일을 추가하면 된다. 화면 캡처는 필수 자료가 아니다.

## 1. main에서 전체 흐름 확인

제공된 `main`의 핵심 흐름에 최신 타깃 상수를 반영하면 다음과 같다.

```c
v6[0] = 0xEF72E1D257B60105uLL;
v6[1] = 0xDDB0734017B667F7uLL;
v6[2] = 0x4FB48342E067C762uLL;
v6[3] = 0xBFB2455601C635A7uLL;

// read_input이 성공한 뒤 실행되는 부분
rotate_blocks(v8, v7);
xor_by_index(v7);
rotate_nibbles(v7);
if ((unsigned int)compare_target(v7, v6))
    puts_0("Correct");
```

호출 관계를 통해 배열의 역할을 구분할 수 있다.

| 변수 | 역할 |
|---|---|
| `v8` | 사용자가 입력한 원본 데이터 |
| `v7` | 변환 결과를 저장하는 32바이트 배열 |
| `v6` | 변환 결과와 비교하는 타깃 |

`_main()`과 입력 모드 설정은 검증 연산을 시작하기 전의 준비 코드다. 플래그를 복원할 때 필요한 것은 타깃 상수와 세 변환 함수다.

## 2. 타깃 배열 추출

`v6`은 `_QWORD v6[4]`로 보인다. QWORD는 8바이트이므로 배열 전체는 32바이트다.

x86-64에서는 정수의 하위 바이트부터 메모리에 저장되는 리틀 엔디언 방식을 사용한다. 따라서 첫 번째 상수는 다음 순서로 읽는다.

```text
0xEF72E1D257B60105
→ 05 01 B6 57 D2 E1 72 EF
```

네 상수를 각각 8바이트 리틀 엔디언으로 변환해 연결하면 타깃은 다음과 같다.

```text
05 01 B6 57 D2 E1 72 EF
F7 67 B6 17 40 73 B0 DD
62 C7 67 E0 42 83 B4 4F
A7 35 C6 01 56 45 B2 BF
```

이 값은 플래그 원문이 아니라, 플래그에 세 변환을 적용한 결과다.

## 3. rotate_blocks 분석

제공된 디컴파일의 바이트 이동 식은 다음과 같다.

```c
*(_BYTE *)(8 * ((i % 4 + i / 8) % 4) + i % 8 + a2)
    = *(_BYTE *)(i + a1);
```

오른쪽에서는 `a1 + i`의 1바이트를 읽고, 왼쪽에서는 `a2 + 목적지`에 그 값을 쓴다. 주소 표현을 배열로 바꾸면 다음과 같다.

```c
output[8 * ((i % 4 + i / 8) % 4) + i % 8] = input[i];
```

목적지 계산은 `8 * 어떤 번호 + i % 8`의 형태다. 따라서 데이터를 8바이트씩 묶고, 묶음 안의 위치는 유지한다고 해석할 수 있다.

각 항에 이름을 붙이면 의미가 명확해진다.

```c
block = i / 8;             // 현재 블록 번호: 0~3
position = i % 8;          // 블록 내부 위치: 0~7
rotations = i % 4;         // 이동할 블록 수: 0~3
next_block = (block + rotations) % 4;

output[next_block * 8 + position] = input[i];
```

`i`는 0부터 31까지이므로 네 블록이 존재한다. `% 4`는 마지막 블록을 넘어가면 다시 첫 블록으로 돌아오게 한다. 이동량은 바이트 값이 아니라 인덱스를 기준으로 정한다.

블록 내부 위치가 5인 바이트를 따라가면 순환 구조를 확인할 수 있다.

| 원본 인덱스 | 현재 블록 | 이동량 | 목적지 인덱스 |
|---:|---:|---:|---:|
| 5 | 0 | 1 | 13 |
| 13 | 1 | 1 | 21 |
| 21 | 2 | 1 | 29 |
| 29 | 3 | 1 | 5 |

즉 `5 → 13 → 21 → 29 → 5`로 이동한다. 블록 내부 위치는 변하지 않는다. 같은 위치의 바이트는 어느 블록에 있든 이동량이 같으므로, 각 위치의 네 바이트가 서로 충돌하지 않고 순환한다.

순방향이 `output[destination(i)] = input[i]`이므로 복원할 때는 다음처럼 목적지에서 값을 가져온다.

```python
flag[i] = block_moved_data[destination(i)]
```

이때 읽는 배열과 쓰는 배열을 분리해야 아직 읽지 않은 값이 덮어써지지 않는다.

## 4. xor_by_index 분석

디컴파일에서는 여러 if와 switch가 섞여 있지만, `i`는 0~31이고 `i % 8`은 항상 0~7이다. 실제 동작은 나머지에 따라 아래 상수를 XOR하는 것이다.

| `i % 8` | XOR 상수 |
|---:|---:|
| 0 | `0x12` |
| 1 | `0x23` |
| 2 | `0x34` |
| 3 | `0x45` |
| 4 | `0x56` |
| 5 | `0x67` |
| 6 | `0x78` |
| 7 | `0x89` |

XOR는 같은 상수를 두 번 적용하면 원래 값으로 돌아간다.

```text
(x XOR key) XOR key = x
```

따라서 이동 후 배열의 각 위치에 같은 상수를 다시 XOR하면 이 단계를 되돌릴 수 있다.

## 5. rotate_nibbles 분석

제공된 디컴파일의 핵심 연산은 다음과 같다.

```c
*result = (*result >> 4) | (16 * *result);
```

`16 * value`는 `value << 4`와 같다. 오른쪽으로 4비트 이동한 값과 왼쪽으로 4비트 이동한 값을 OR하고, 결과를 1바이트에 저장한다. 저장할 때 하위 8비트만 남으므로 상위 4비트와 하위 4비트가 바뀐다.

```text
0xAB → 0xBA
```

같은 연산을 다시 적용하면 `0xBA → 0xAB`가 된다. 따라서 이 단계는 같은 식으로 복원한다. Python 정수는 자동으로 8비트에 잘리지 않으므로 `& 0xFF`를 붙인다.

```python
original = ((value << 4) | (value >> 4)) & 0xFF
```

## 6. 역순 복원과 솔버

입력은 블록 이동 → XOR → 4비트 회전 순서로 처리된다. 따라서 타깃에서는 **4비트 회전 → XOR → 블록 이동의 복원** 순서로 되돌린다.

다음 코드는 갱신된 타깃 상수와 제공된 IDA 변환식만 사용한다. 정답 플래그나 작성자가 가진 소스 파일을 읽지 않는다. 같은 코드를 `solve.py`로 저장했다.

```python
words = [
    0xEF72E1D257B60105,
    0xDDB0734017B667F7,
    0x4FB48342E067C762,
    0xBFB2455601C635A7,
]
target = b"".join(word.to_bytes(8, "little") for word in words)
keys = [0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89]

# 1. 마지막에 적용한 4비트 회전을 되돌린다.
data = bytearray(((value << 4) | (value >> 4)) & 0xFF for value in target)

# 2. 같은 상수로 XOR한다.
for i in range(32):
    data[i] ^= keys[i % 8]

# 3. 원본 바이트가 이동했던 목적지에서 값을 가져온다.
flag = bytearray(32)
for i in range(32):
    block = i // 8
    position = i % 8
    rotations = i % 4
    destination = ((block + rotations) % 4) * 8 + position
    flag[i] = data[destination]

# 4. 순방향 변환으로 복원 결과를 자체 검증한다.
check = bytearray(32)
for i in range(32):
    destination = ((i // 8 + i % 4) % 4) * 8 + i % 8
    check[destination] = flag[i]
for i in range(32):
    value = check[i] ^ keys[i % 8]
    check[i] = ((value << 4) | (value >> 4)) & 0xFF
assert bytes(check) == target

print(flag.decode("ascii"))
```

## 7. 결과와 실행 검증

`python solve.py`를 실행하면 다음 32바이트 플래그가 복원된다.

```text
BUBU{P3rm_X0R_Sw4p_4r3_Th3_K3ys}
```

`P3rm`은 Permutation(블록 간 바이트 재배치), `X0R`은 XOR, `Sw4p`은 상·하위 4비트 교환을 뜻한다. 전체 문구는 "Permutation, XOR, Swap are the keys"이며, 세 변환의 핵심 원리가 플래그에 담겨 있다.

솔버의 순방향 자체 검증을 통과했고, 새 타깃으로 재빌드한 Windows와 Linux 실행 파일에 각각 입력해 다음 결과를 확인했다.

| 입력 | Windows | Linux |
|---|---|---|
| 복원한 32바이트 플래그 | `Correct` | `Correct` |
| 변경 전 32바이트 플래그 | `Wrong` | `Wrong` |
| 플래그의 마지막 바이트 제거 | `Wrong` | `Wrong` |
| 플래그 뒤에 `A` 추가 | `Wrong` | `Wrong` |
| `A` 32개 | `Wrong` | `Wrong` |

분석 중 IDA를 직접 조작하거나 디버거로 메모리를 추적하지는 않았다. 이 Write-up의 디컴파일 근거는 제공된 복사본이며, 누락된 입력·비교 함수는 공유 소스로 보완하고 실행으로 검증했다.
