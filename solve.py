"""변경된 타깃 상수와 제공된 IDA 변환식으로 플래그를 복원한다."""

words = [
    0xEF72E1D257B60105,
    0xDDB0734017B667F7,
    0x4FB48342E067C762,
    0xBFB2455601C635A7,
]
target = b"".join(word.to_bytes(8, "little") for word in words)
keys = [0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89]

# 마지막 연산부터 되돌린다. 8비트의 4비트 회전은 같은 연산으로 복원된다.
data = bytearray(((value << 4) | (value >> 4)) & 0xFF for value in target)

# XOR는 같은 상수를 다시 적용하면 원래 값이 된다.
for i in range(32):
    data[i] ^= keys[i % 8]

# 순방향의 input[i]가 도착했던 위치에서 값을 다시 가져온다.
flag = bytearray(32)
for i in range(32):
    block = i // 8
    position = i % 8
    rotations = i % 4
    destination = ((block + rotations) % 4) * 8 + position
    flag[i] = data[destination]

# 복원한 입력을 순방향으로 변환해 원래 타깃과 일치하는지 확인한다.
check = bytearray(32)
for i in range(32):
    destination = ((i // 8 + i % 4) % 4) * 8 + i % 8
    check[destination] = flag[i]
for i in range(32):
    value = check[i] ^ keys[i % 8]
    check[i] = ((value << 4) | (value >> 4)) & 0xFF
assert bytes(check) == target, "순방향 검증 결과가 타깃과 다르다"

print(flag.decode("ascii"))
