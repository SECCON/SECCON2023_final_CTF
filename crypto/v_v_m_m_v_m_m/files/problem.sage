import os
from Crypto.Cipher import AES
import signal
signal.alarm(3600)

q = 16
vsize = 22
o1 = 22
o2 = 22
m = o1 + o2
n = vsize + o1 + o2
K = GF(q)

poly2int = {}
for i in range(q):
    poly2int[K(Integer(i).digits(2))] = i

# r: random_element()
# e.g. size1 = 4, size2 = 6
#
#   size2
#   vvvvvv
#  [rrrrrr000] < size1
#  [0rrrrr000] < size1
#  [00rrrr000] < size1
#  [000rrr000] < size1
#  [000000000]
#  [000000000]
#  [000000000]
#  [000000000]
#  [000000000]
def make_UD_matrix(size1, size2):
    res = Matrix(K, n, n)
    small_size = min(size1, size2)
    big_size = max(size1, size2)

    for i in range(small_size):
        for j in range(big_size):
            if i <= j:
                res[i,j] = K.random_element()
    return res

# res = mat * vec
def product(mat, vec):
    res = []
    for i in range(mat.nrows()):
        element = Matrix(K, n, n)
        for j in range(mat.ncols()):
            element += mat[i,j] * vec[j]
        res.append(element)

    return res

def gen_key():
    S = random_matrix(K, n, n)
    T = random_matrix(K, m, m)
    while not S.is_invertible(): S = random_matrix(K, n, n)
    while not T.is_invertible(): T = random_matrix(K, m, m)

    Fs = []
    for _ in range(o1): Fs.append(make_UD_matrix(vsize,vsize+o1))
    for _ in range(o2): Fs.append(make_UD_matrix(n,vsize+o1))

    SFSs = []
    for F in Fs: SFSs.append(S * F * S.transpose())
    pubkey = SFSs
    pubkey = product(T, SFSs)

    return (pubkey, (S, T, Fs))

def eval_v(pubkey, v):
    return vector([v * matp * v for matp in pubkey])

def vec2string(vec):
    res=""
    for e in vec:
        res += str(poly2int[e]) + ","
    return res[:-1]

def mat2strings(mat):
    res = []
    for row in mat:
        res.append(vec2string(row))
    return res
    
def strings2vec(str):
    res = []
    vals = str.split(',')
    for val in vals:
        res.append(K(Integer(int(val)).digits(2)))
    return vector(K, res)

def strings2mat(strs,nrows,ncols):
    mat = []
    for row in strs:
        mat.append(strings2vec(row))
        assert ncols == len(mat[-1])
    assert nrows == len(mat)
    return matrix(K, mat)

message = random_vector(K, n)
for challenge_id in range(2):
    pubkey, privkey = gen_key()
    ciphertext = eval_v(pubkey, message)

    pubkey_str = []
    for i in range(len(pubkey)):
        pubkey_str.append(mat2strings(pubkey[i]))
        print(f"pubkey[{i}]: {pubkey_str[i]}")

    vecs = []
    for i in range(22):
        vecs.append(strings2vec(input(f"vec{i}:")))
    V = VectorSpace(K,n)
    S = V.subspace(vecs)
    assert S.dimension() == 22
    vec = S.random_element()

    print("eval(vec) result:", vec2string(eval_v(pubkey, vec)))
    print("eval(message+vec)-eval(message) result:", vec2string(eval_v(pubkey, vec+message)-eval_v(pubkey, message)))

answer = strings2vec(input("ans:"))
if answer == message:
    flag = os.getenv("FLAG", "SECCON{dummyflagdummyflagdummyf}")
    print("flag:", flag)
else:
    print("flag:", "fail")
