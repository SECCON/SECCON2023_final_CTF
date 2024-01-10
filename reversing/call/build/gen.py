import random

flag = b"fL4g_cH3cK3r_bA5eD_0n_CFG_Ch3Ck!"

class TreeNode(object):
    def __init__(self, depth, is_correct, is_definite=True):
        self.depth = depth
        self.L = None
        self.R = None
        self.is_correct = is_correct
        self.is_definite = is_definite

def trace(root, route, n, debug=False):
    for _ in range(n):
        choice = route & 1
        route >>= 1
        if choice == 0:
            root = root.L
        else:
            root = root.R
        if debug:
            print(choice, root.is_correct, root.depth)
    return root

def put_correct(root, c, n):
    for _ in range(n):
        choice = c & 1
        c >>= 1
        if choice == 0:
            if root.L is None:
                root.L = TreeNode(root.depth + 1, True)
            root = root.L
        else:
            if root.R is None:
                root.R = TreeNode(root.depth + 1, True)
            root = root.R

def put_wrong(root, c, n):
    b = root.depth
    is_invalid = False
    candidates = []
    for _ in range(n):
        choice = c & 1
        c >>= 1
        if choice == 0:
            if root.L is None:
                root.L = TreeNode(root.depth + 1, True, False)
            root = root.L
        else:
            if root.R is None:
                root.R = TreeNode(root.depth + 1, True, False)
            root = root.R

        if root.is_correct == False:
            is_invalid = True

        odds = 0.01 * (100 - (b + n - root.depth) / (b + n) * 80)
        if root.is_definite != True and random.random() < odds:
            candidates.append(root)

    if is_invalid == False:
        if len(candidates):
            random.choice(candidates).is_correct = False
        else:
            root.is_correct = False

root = TreeNode(0, True)
queue = [root]
while len(queue):
    node = queue.pop(0)
    if node.depth >= 5:
        break
    node.L = TreeNode(node.depth + 1, True)
    node.R = TreeNode(node.depth + 1, True)
    queue.append(node.L)
    queue.append(node.R)

for i in range(32):
    r = trace(root, i, 5)
    put_correct(r, flag[i], 7)
    for c in range(128):
        if c != flag[i]:
            put_wrong(r, c, 7)

array = []
queue = [root]
while len(queue):
    node = queue.pop(0)
    if node.L:
        queue.append(node.L)
        array.append(node.L.is_correct)
    if node.R:
        queue.append(node.R)
        array.append(node.R.is_correct)

def trace_arr(array, route, n):
    index = offset = 0
    base = 1
    for _ in range(n):
        p = index + offset + (route & 1)
        print(array[p], p, index, offset, route & 1)
        base <<= 1
        offset <<= 1
        offset += 2 * (route & 1)
        route >>= 1
        index += base

#i = ord('a')<<5 | 0
#trace(root, i, 12, debug=True)
#trace_arr(array, i, 12)
#exit()

code = "#include <stdio.h>\n"
init = """
int init() {
  size_t(*func)();
"""

cnt = 0
for boolean in array:
    v = random.randint(0x7fff_ffff_ffff_ffff, 0xffff_ffff_ffff_ffff)
    if boolean:
        code += f"""
size_t FUNC_{cnt:04x}() {{
  return 0;
}}
size_t FUNC_{cnt+1:04x}() {{
  return 0;
}}"""
        init += f"  func = FUNC_{cnt:04x};\n"
        init += f"  func = FUNC_{cnt+1:04x};\n"
        cnt += 2
    else:
        code += f"""
size_t FUNC_{cnt:04x}() {{
  __asm {{
    xor eax, eax
    pop ebp
    ret
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    push ebp
    mov ebp, esp
  }}
  return 0;
}}"""
        init += f"  func = FUNC_{cnt:04x};\n"
        cnt += 1
init += "  return 0;\n}\n"

main = open("template.c", "r").read()

print(code + init + main)

