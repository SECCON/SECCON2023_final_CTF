import angr
import claripy

def ok(state):
    if b"Correct!" in state.posix.dumps(1):
        return True
    return False

def ng(state):
    if b"Wrong..." in state.posix.dumps(1):
        return True
    return False

proj = angr.Project('../files/remov', auto_load_libs=False)

arg = claripy.BVS('arg', 8*0x30)

state = proj.factory.entry_state(args=['./remov', arg])
simgr = proj.factory.simulation_manager(state)
simgr.explore(find=ok, avoid=ng)
print("len(simgr.found) = {}".format(len(simgr.found)))

if len(simgr.found) > 0:
    s = simgr.found[0]
    print("argv[1] = {!r}".format(s.solver.eval(arg, cast_to=bytes)))
    print("stdin = {!r}".format(s.posix.dumps(0)))
