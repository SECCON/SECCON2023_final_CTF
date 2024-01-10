from pwn import remote
import pickle
import random
import subprocess
import re
import time
import os

q = 16
v = 22
o1 = 22
o2 = 22
m = o1 + o2
n = v + o1 + o2
K = GF(q)

poly2int = {}
for i in range(q):
    poly2int[K(Integer(i).digits(2))] = i

def Eval(F,x):
    return vector([ x*M*x for M in F])

def Differential(F,x,y):
    return vector([ (x*M*y) + (y*M*x)  for M in F ])

# makes a matrix upper diagonal
def Make_UD(M):
    n = M.ncols()
    for i in range(n):
        for j in range(i+1,n):
            M[i,j] += M[j,i]
            M[j,i] = K(0) 
    return M

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

def get_pubkey(io):
    pubkey = []
    for i in range(m):
        io.recvuntil("pubkey")
        memo = io.readline().split(b':')[1]
        mat = strings2mat(eval(memo), n, n)
        Make_UD(mat)
        pubkey.append(mat)
    return pubkey

def get_resultvec(io):
    io.recvuntil("result")
    res = strings2vec(io.recvline().split(b":")[1].decode('utf-8'))
    return res

# reference: https://github.com/WardBeullens/BreakingRainbow/blob/main/Attack_demo/Full_Attack.sage
def full_attack(PK):
    # read PK
    # q,n,m,o2,PK = pickle.load( open( "pk.p", "rb" ) )
    global attempts
    attempts = 0
    K = GF(q)

    if q != 16:
        print("Demo only for q = 16")
        exit()

    #if (n-o2) % 2 != 0:
    #    print("Demo only for parameters with n-o2 even")
    #    exit()

    if (n-o2)-2*(m-02) > 2:
        print("Demo only for when (n-o2)-2*(m-02) is not too large, otherwise Kipnis-Shamir is too slow.")
        exit()

    # compile Wiedemann XL
    print("Compiling Wiedemann XL ...")

    with open("BreakingRainbow/xl-20160426/Makefile") as f:
        lines = f.readlines()

    lines[0] = "Q = "+str(q)+"\n"
    lines[1] = "M = "+str(m-1)+"\n"
    lines[2] = "N = "+str(n-m-2)+"\n"

    print("",lines[0],lines[1],lines[2])

    with open("./BreakingRainbow/xl-20160426/Makefile", "w") as f:
        f.writelines(lines)

    subprocess.run("cd BreakingRainbow/xl-20160426; make", shell=True)
    subprocess.run("cp BreakingRainbow/xl-20160426/xl .", shell=True)

    load('./BreakingRainbow/Rainbow.sage')
    load('./BreakingRainbow/SimpleAttack.sage')

    while True:
        # make a guess and compose an MQ system
        print("Make a guess and compose MQ system")
        guess,system_filename,D_x_ker = Attack(PK)

        print("guess = ",guess)

        #Run Wiedemann XL 
        print("Run Wiedemann XL")
        subprocess.run("./xl --challenge "+system_filename+" --all | tee WXL_output.txt", shell = True)

        matches = [line for line in open('WXL_output.txt') if re.search(r'is sol',line)]

        if len(matches) >= 1:
            break


    solution_string = matches[0]

    print("Solution found after %d attempts" % attempts)
    print(solution_string)

    print("Parsed solution:")

    y =  vector( [0] + [str_to_elt(str.lower(s[1:])) for s in solution_string.split()[0:n-m-2]] + [1])
    print(y)

    print("y")
    print(D_x_ker.transpose()*y)
    y = D_x_ker.transpose()*y

    Eval_guess = Eval(PK,guess)
    Eval_y  = Eval(PK,y)

    print(Eval_guess)
    print(Eval_y)

    alpha = (Eval_y[0]/Eval_guess[0])
    print("alpha:", alpha)
    oil_vec = sqrt(alpha)*guess + y

    print("vector in O2 is x = sqrt(alpha)*guess + y:")
    print(oil_vec)
    print("Sanity check: Pk(x) should be zero:")
    print("Pk(x) = ",Eval(PK,oil_vec))


    print("Finishing the attack:")

    # Finding W space
    basis_Fn = (K**n).basis()
    W_spanning_set = [ Differential(PK,e,oil_vec) for e in basis_Fn ]
    W = matrix((K**m).span(W_spanning_set).basis()).transpose()

    # Finding O2 space
    W_perp = matrix(W.kernel().basis())
    P1 = [sum( [ W_perp[j,i]*PK[i] for i in range(m) ] ) for j in range(m-o2)] # P1 is inner layer of Rainbow

    O2 = K**n
    for P in P1:
        O2 = O2.intersection((P+P.transpose()).kernel())

    O2 = matrix(O2.basis()).transpose()

    # Finding O1 space

    #extend basis of O2 to a basis of K^n 
    Basis = O2
    for e in basis_Fn:
        if e not in Basis.column_space():
            Basis = Basis.augment(matrix(K,n,1, list(e)))

    UOV_pk = [ Basis.transpose()*P*Basis for P in P1 ] 
    for p in UOV_pk:
        Make_UD(p)
    UOV_pk = [ p[-n+o2:,-n+o2:] for p in UOV_pk ]

    print("Kipnis Shamir attack to find vectors in O1:")
    # Kipnis-Shamir attack look for eigenvalues of MM untill we have a basis for Oil space
    # We check if an eigenvalue is in the oil space by checking Eval(UOV_pk).is_zero()

    OV_O_basis = []
    while len(OV_O_basis) < m-o2:
        M = sum([ K.random_element()*(P+P.transpose()) for P in UOV_pk ] )
        M0 = UOV_pk[0]+UOV_pk[0].transpose()

        to_delete = None
        if (n-o2) % 2 == 1:
            to_delete = random.randrange(n-o2)
            M = M.delete_rows([to_delete]) 
            M0 = M0.delete_rows([to_delete]) 
            M = M.delete_columns([to_delete]) 
            M0 = M0.delete_columns([to_delete]) 

        if not M.is_invertible():
            continue
        MM = M0*M.inverse()

        cp = MM.characteristic_polynomial()
        for f,a in factor(cp):
            fMM_ker = f(MM).kernel()
            if fMM_ker.rank() == 2:
                b1,b2 = fMM_ker.basis()
                for v in K:
                    o = b1+v*b2
                    if (n-o2) % 2 == 1:
                        o_list = list(o)
                        o_list.insert(to_delete,K(0))
                        o = vector( o_list )

                    if Eval(UOV_pk,o).is_zero():
                        if not o in span(OV_O_basis,K):
                            OV_O_basis.append(o)
                            print("O1 vectors found: %d" % len(OV_O_basis))
                            break
                        else:
                            print("Vector is not new :(")
                if len(OV_O_basis) == m-o2:
                    break
            if len(OV_O_basis) == m-o2:
                break


    #extend basis to K^n
    O1_basis = [ Basis*vector([K(0)]*o2 + list(b)) for b in OV_O_basis] + O2.columns()
    O1 = matrix(O1_basis).transpose()

    print("Write recovered SK")
    pickle.dump( [O2,O1,W] , open( "sk_recovered.p", "wb" ) )
    return O2, O1, W
    
# --------------------------------------------------

def send_vecs(io, vecs):
    for i in range(len(vecs)):
        print(io.recvuntil(":"))
        io.sendline(vec2string(vecs[i]))
    
def solve_o(io, pubkey, r, mat):
    temp = get_resultvec(io) # Eval(pubkey, r+o)
    for i in range(1,16):
        rr = K(Integer(i).digits(2)) * r
        rhs = temp - Eval(pubkey, rr)
        PRo = PolynomialRing(K, mat.ncols(), 'o')
        o_vars = vector(PRo.gens())
        o = mat * o_vars
        lhs = Differential(pubkey, rr, o)
        eqs = []
        for i in range(len(rhs)):
            eqs.append(lhs[i]-rhs[i])
        ideal = PRo.ideal(eqs).variety(K)
        if len(ideal) != 1:
            continue

        o = []
        print("o_vars:", ideal[0])
        for i in range(mat.ncols()):
            o.append(ideal[0][o_vars[i]])
        o = vector(K, o)
        print("o:", o)
        return vector(rr + mat*o)

def get_x_eqs(io, x_vars, pubkey, vec):
    a = get_resultvec(io)
    print("eval(sig+vec)-eval(sig):", vec2string(a))
    print("eval(vec):", vec2string(Eval(pubkey, vec)))
    rhs = a - Eval(pubkey, vec)
    print("rhs", rhs)
    print("vec", vec2string(vec))
    lhs = Differential(pubkey, x_vars, vec)
    eqs = []
    for i in range(len(rhs)):
        eqs.append(lhs[i]-rhs[i])
    return eqs

def solve(io, x_vars):
    pubkey = get_pubkey(io)
    print("--------------------------------------------------pubkey--------------------------------------------------")
    print(pubkey[0])
    print("--------------------------------------------------pubkey--------------------------------------------------")

    O2, O1, W = full_attack(pubkey)
    r = random_vector(K, n)

    vecs = []
    SubO2 = []
    for i in range(21):
        vecs.append(O2.column(i))
        SubO2.append(O2.column(i))
    SubO2 = matrix(SubO2).transpose()

    vecs.append(r)

    send_vecs(io, vecs)
    vec = solve_o(io, pubkey,r,SubO2)
    if vec == None:
        return None
    
    return get_x_eqs(io, x_vars, pubkey, vec)

def solve2(io):
    PRx = PolynomialRing(K, n, 'x')
    x_vars = vector(PRx.gens())
    eq1 = solve(io, x_vars) # Todo: loop
    if eq1 == None:
        return None 
    eq2 = solve(io, x_vars)
    if eq2 == None:
        return None
    print(eq1)
    print("--------------------------------------------------eq1--------------------------------------------------")
    print(eq2)
    print("--------------------------------------------------eq2--------------------------------------------------")
    ideal = PRx.ideal(eq1+eq2).variety(K)

    print("--------------------------------------------------ideal--------------------------------------------------")
    print(ideal)
    if len(ideal) != 1:
        return None 

    ans = []
    for i in range(n):
        ans.append(ideal[0][x_vars[i]])
    ans = vector(K, ans)

    print(io.recvuntil("ans:"))
    io.sendline(vec2string(ans))
    io.recvuntil("flag:")
    res = io.recvline()
    return res

cnt = 0
while True:
    cnt += 1
    print("challenge:", cnt)
    io = remote(os.getenv("SECCON_HOST"), int(os.getenv("SECCON_PORT")))
    res = solve2(io)
    io.close()
    if res != None:
        print(res)
        break
    break

print("cnt: ", cnt)
