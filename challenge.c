#include <stdio.h>   // getchar, puts: 입력을 읽고 결과를 출력합니다.
#include <string.h>  // memcmp: 두 배열의 바이트를 비교합니다.

// Windows 빌드에만 필요한 입력 모드 설정입니다.
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// 한 줄을 읽습니다. 줄바꿈을 제외하고 32바이트이면 1, 아니면 0입니다.
static int read_input(unsigned char input[32])
{
    int i;
    // getchar의 EOF 값을 구분하기 위해 int로 받습니다.
    int ch;

    // 임시 배열 없이 input에 32바이트를 직접 읽습니다.
    for (i = 0; i < 32; i++) {
        ch = getchar();
        if (ch == EOF || ch == '\n') {
            return 0;
        }
        input[i] = ch;
    }

    // 다음 문자가 줄바꿈 또는 입력 종료여야 길이가 정확히 32바이트입니다.
    ch = getchar();
    if (ch == '\r') {
        // Windows식 줄바꿈은 '\r' 다음에 '\n'이 와야 합니다.
        ch = getchar();
        if (ch != '\n') {
            return 0;
        }
    }
    return (ch == '\n' || ch == EOF) && !ferror(stdin);
}

// 1~2단계: 8바이트씩 네 블록으로 보고, 같은 위치를 유지하며 블록을 옮깁니다.
static void rotate_blocks(const unsigned char input[32], unsigned char output[32])
{
    int i;

    for (i = 0; i < 32; i++) {
        int block = i / 8;          // 현재 블록 번호: 0~3.
        int position = i % 8;      // 블록 안의 위치: 0~7.
        int rotations = i % 4;     // 이동할 블록 수: 0~3.
        int next_block = (block + rotations) % 4; // 마지막 블록 다음은 0번.

        // 별도 배열에 써서 아직 읽지 않은 원본 바이트를 덮어쓰지 않습니다.
        output[next_block * 8 + position] = input[i];
    }
}

// 3단계: 이동 후 인덱스의 나머지에 따라 각 바이트를 XOR합니다.
static void xor_by_index(unsigned char data[32])
{
    int i;

    for (i = 0; i < 32; i++) {
        // ^=는 기존 바이트와 상수를 XOR한 결과를 같은 위치에 저장합니다.
        switch (i % 8) {
        case 0: data[i] ^= 0x12; break;
        case 1: data[i] ^= 0x23; break;
        case 2: data[i] ^= 0x34; break;
        case 3: data[i] ^= 0x45; break;
        case 4: data[i] ^= 0x56; break;
        case 5: data[i] ^= 0x67; break;
        case 6: data[i] ^= 0x78; break;
        case 7: data[i] ^= 0x89; break;
        }
    }
}

// 4단계: 각 바이트를 4비트 회전합니다. 상위 4비트와 하위 4비트가 바뀝니다.
static void rotate_nibbles(unsigned char data[32])
{
    int i;

    for (i = 0; i < 32; i++) {
        // << 4는 하위 4비트를 위로, >> 4는 상위 4비트를 아래로 옮깁니다.
        // unsigned char에 저장하면 하위 8비트만 남습니다. 예: 0xAB -> 0xBA.
        data[i] = (data[i] << 4) | (data[i] >> 4);
    }
}

// 모든 바이트가 타깃과 같으면 1, 하나라도 다르면 0을 반환합니다.
static int compare_target(const unsigned char data[32], const unsigned char target[32])
{
    // memcmp는 지정한 32바이트가 모두 같으면 0을 반환합니다.
    return memcmp(data, target, 32) == 0;
}


int main(void)
{
    unsigned char input[32]; // 사용자가 입력한 원본 바이트입니다.
    unsigned char moved[32]; // 블록 이동과 이후 변환 결과를 저장합니다.
    // 원본 플래그에 1~4단계를 적용한 비교 값입니다. 함수의 지역 배열입니다.
    unsigned char target[32] = {
        0x05, 0x01, 0xB6, 0x57, 0xD2, 0xE1, 0x72, 0xEF,
        0xF7, 0x67, 0xB6, 0x17, 0x40, 0x73, 0xB0, 0xDD,
        0x62, 0xC7, 0x67, 0xE0, 0x42, 0x83, 0xB4, 0x4F,
        0xA7, 0x35, 0xC6, 0x01, 0x56, 0x45, 0xB2, 0xBF
    };

#ifdef _WIN32
    // Windows에서도 입력을 바이트 그대로 읽습니다.
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        puts("Wrong");
        return 1;
    }
#endif

    // 입력 실패 또는 32바이트가 아닌 입력은 바로 Wrong입니다.
    if (!read_input(input)) {
        puts("Wrong");
        return 1;
    }

    rotate_blocks(input, moved); // 1~2단계: 블록 이동.
    xor_by_index(moved);         // 3단계: XOR.
    rotate_nibbles(moved);       // 4단계: 4비트 회전.

    // 변환한 입력과 타깃을 비교합니다.
    if (!compare_target(moved, target)) {
        puts("Wrong");
        return 1;
    }

    // 모든 바이트가 같으면 정답입니다.
    puts("Correct");
    return 0;
}
