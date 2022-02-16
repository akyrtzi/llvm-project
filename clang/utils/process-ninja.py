#!/usr/bin/env python3

import argparse, os, sys, json
from threading import Semaphore, Thread, Event, Lock
from subprocess import Popen
import multiprocessing


args = None

def parse_args():
  parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description="""Ninja build inspector.
    """)

  parser.add_argument('-manifest', required=False,
    dest='manifest', help="path to build.json"
  )

  parser.add_argument('-jobs', required=False,
    default=multiprocessing.cpu_count(),
    dest='jobs', type=int,
    help="number of parallel jobs"
  )

  parser.add_argument('-start-from', required=False,
    default=1,
    dest='start_from_n', type=int,
    help="index of invocation to start from"
  )

  parser.add_argument('-extra-compiler-args', required=False,
    default="",
    dest='extra_compiler_args',
    help="args to append to compiler invocations"
  )

  parser.add_argument('-quiet', required=False,
    dest='quiet', action='store_true',
    help="don't print status for processing files"
  )

  parser.add_argument('-target', required=False,
    dest='target', help="target name to inspect"
  )

  parser.add_argument('-cmake-files', required=False,
    dest='cmake_files', help="path to cmakeFiles json file"
  )

  return parser.parse_args()

def popen_and_call(popen_args, on_exit):
    """
    Runs the given args in a subprocess.Popen, and then calls the function
    on_exit when the subprocess completes.
    on_exit is a callable object, and popen_args is a list/tuple of args that 
    would give to subprocess.Popen.
    """
    def run_in_thread(on_exit, popen_args):
        proc = Popen(popen_args)
        ret = proc.wait()
        on_exit(ret)
        return
    thread = Thread(target=run_in_thread, args=(on_exit, popen_args))
    thread.start()
    # returns immediately after the thread starts
    return thread

should_exit = False
glob_lock = Lock()

class CommandBase(object):
  def __init__(self, cmd, manifest):
    self.cmd = cmd
    self.manifest = manifest

  def is_phony(self):
    return isPhony(self.cmd)

  def get_inputs_without_phonies(self):
    inps = set()
    for target in self.inputs():
      cmd = self.manifest.cmd_wrapper(target)
      if cmd is None:
        inps.add(target)
        continue
      if cmd.is_phony():
        inps.update(cmd.get_inputs_without_phonies())
        continue
      inps.add(target)
    return inps

class PhonyCommand(CommandBase):
  def __init__(self, cmd, manifest):
    CommandBase.__init__(self, cmd, manifest)

  def inputs(self):
    return self.cmd.get('inputs',[]) + self.cmd.get('implicit_inputs',[]) + self.cmd.get('order_only_inputs',[])

class GenericCommand(CommandBase):
  def __init__(self, cmd, manifest):
    CommandBase.__init__(self, cmd, manifest)

  def inputs(self):
    return self.cmd.get('inputs',[]) + self.cmd.get('implicit_inputs',[])

class CompileCommand(CommandBase):
  def __init__(self, cmd, manifest):
    CommandBase.__init__(self, cmd, manifest)

  def inputs(self):
    return self.cmd.get('inputs',[]) + self.cmd.get('implicit_inputs',[]) + self.cmd.get('order_only_inputs',[])

class Manifest(object):
  def __init__(self, path):
    manifest = json.load(open(path, 'r'))
    self.commands = manifest['commands']
    cmdIdxByOutput = {}
    for i in range(0, len(self.commands)):
      cmd = self.commands[i]
      for output in cmd['outputs']:
        assert(not output in cmdIdxByOutput)
        cmdIdxByOutput[output] = i
    self.cmdIdxByOutput = cmdIdxByOutput

  def info_for_target(self, target):
    cmd = self.cmd_wrapper(target)
    if cmd is None:
      pout("'%s' is existing input" % target)
      return
    inputs = sorted(list(cmd.get_inputs_without_phonies()))
    result = {
      'inputs': inputs,
      'outputs': cmd.cmd['outputs'],
      'rule': cmd.cmd['rule'],
      'command': cmd.cmd['command'],
    }
    dict_print(result)    

  def print_object_depends(self, target):
    objs = self.get_all_object_depends(target)
    for obj in objs:
      pout(obj)

  def compile_object_depends(self, target):
    global args
    sema = Semaphore(args.jobs)

    global should_exit
    global glob_lock

    threads = []
    objs = self.get_all_object_depends(target)
    total = len(objs)
    pout("total TUs: %d" % (total))
    for i in range(args.start_from_n-1, total):
      obj = objs[i]
      sema.acquire()
      thread = self.create_compiler_proc(obj, i, total, sema)
      threads.append(thread)

      stop = False
      glob_lock.acquire()
      stop = should_exit
      glob_lock.release()
      if stop:
        break

    while len(threads) != 0:
      thread = threads.pop(0)
      thread.join()

      stop = False
      glob_lock.acquire()
      stop = should_exit
      glob_lock.release()
      if stop:
        break

  def create_compiler_proc(self, obj, i, total, sema):
    ninja_cmd = self.cmdForTarget(obj)
    output = ninja_cmd['outputs'][0]
    cmd = ninja_cmd['command']
    global args
    cmd += ' ' + args.extra_compiler_args
    if not args.quiet:
      print_status("[%d/%d] %s" % (i+1, total, output))

    def finished(ret):
      if ret != 0:
        pout("FAILED: [%d/%d] %s " % (i+1, total, cmd))
      global should_exit
      global glob_lock
      glob_lock.acquire()
      should_exit = ret != 0
      glob_lock.release()
      sema.release()

    thread = popen_and_call(['/bin/sh', '-c', cmd], finished)
    return thread

  def get_all_object_depends(self, target):
    objs = []
    objs += self.get_object_depends(target)
    deps = self.get_all_depends(target)
    for dep in deps:
      if dep.endswith('.a'):
        objs += self.get_object_depends(dep)
    objs = sorted(set(objs))
    return objs

  def get_object_depends(self, target):
    deps = self.get_all_depends(target)
    objs = []
    for dep in deps:
      if dep.endswith('.o'):
        objs.append(dep)
    return objs

  def get_all_depends(self, target):
    inps = self.getInputs(target)
    all_inps = set()
    for prop in ['inputs', 'implicit_inputs']:
      all_inps.update(inps[prop])
    return sorted(all_inps)

  def cmd_wrapper(self, target):
    cmd = self.cmdForTarget(target)
    if cmd is None:
      return None
    if isPhony(cmd):
      return PhonyCommand(cmd, self)
    if isCompile(cmd):
      return CompileCommand(cmd, self)
    return GenericCommand(cmd, self)

  def inspect(self, target):
    inputs = self.getInputs(target)
    dict_print(inputs)

  def getInputs(self, target):
    cmd = self.cmdForTarget(target)
    if not cmd:
      raise Exception("%s not found" % target)
    results = self.getTargetsResolvingPhonies(cmd, ['inputs', 'implicit_inputs', 'order_only_inputs'])
    # results['inputs'].update(results['implicit_inputs'])
    # results.pop('implicit_inputs')
    for prop in ['inputs', 'implicit_inputs', 'order_only_inputs']:
      results[prop] = sorted(list(results[prop]))
    return results

  def getTargetsResolvingPhonies(self, cmd, properties):
    assert(cmd)
    results = {}
    phonies = set()
    for prop in properties:
      items = set()
      for item in cmd.get(prop, []):
        subcmd = self.cmdForTarget(item)
        if isPhony(subcmd):
          phonies.add(item)
        else:
          items.add(item)
      results[prop] = items
    for phonyName in phonies:
      subcmd = self.cmdForTarget(phonyName)
      phonyResults = self.getTargetsResolvingPhonies(subcmd, properties)
      for prop in properties:
        results[prop].update(phonyResults[prop])
    return results

  def cmdForTarget(self, target):
    idx = self.cmdIdxByOutput.get(target, None)
    if idx is None:
      return None
    return self.commands[idx]

def isPhony(cmd):
  if cmd is None:
    return False
  return cmd['rule'] == 'phony'

def isCompile(cmd):
  if cmd is None:
    return False
  return cmd['rule'].startswith("CXX_COMPILER_") or cmd['rule'].startswith("C_COMPILER_")

def dict_print(dict):
  pout(json.dumps(dict, sort_keys=True, indent=4))

def print_cmakeFiles(cmakeFilesPath):
  cmakeFiles = json.load(open(cmakeFilesPath, 'r'))
  inputs = cmakeFiles["inputs"]
  paths = []
  for inp in inputs:
    if "isGenerated" in inp or "isExternal" in inp:
      continue
    paths.append(inp["path"])
  for path in sorted(paths):
    pout(path)

def main():
  global args
  args = parse_args()

  if args.cmake_files:
    print_cmakeFiles(args.cmake_files)
    return

  manifest = Manifest(args.manifest)

  pout("parallel jobs: %d" % (args.jobs))

  # manifest.inspect(args.target)
  # manifest.info_for_target(args.target)
  # manifest.print_object_depends(args.target)
  manifest.compile_object_depends(args.target)

def pout(str, end=None):
  print(str, flush=True, end=end)

def print_status(str):
  if sys.stdout.isatty():
    pout(' '*os.get_terminal_size().columns + '\r', end='')
    pout(str+'\r', end='')
  else:
    pout(str)

if __name__ == "__main__":
  main()
