from pwn import remote

YES = '♡♧'
NO = '♧♡'
MID = '♡'

def rot(s, k):
    k %= len(s)
    return s[k:] + s[:k]

def enc(bit):
    return YES if bit else NO

# 辞書の作成
ans_dict = {}
for a in range(2):
    for b in range(2):
        s = enc(a) + MID + enc(b)
        s = s[1] + s[0] + s[2:]
        for k in range(5):
            rotated_s = rot(s, k)
            ans_dict[rotated_s] = 'y' if (a and b) else 'n'

def main():
    # 接続先
    io = remote("34.170.146.252", 43344)

    for _ in range(10):
        io.recvuntil(b'Open cards: ')
        
        cards = io.recvline().decode().strip()
        
        # 辞書から解答を取得して送信
        ans = ans_dict[cards]
        io.sendlineafter(b'> ', ans.encode())

    # 出力を取得
    print(io.recvall().decode())

if __name__ == '__main__':
    main()