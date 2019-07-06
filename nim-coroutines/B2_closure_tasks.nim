#[ Tasks using the closure iterator method. Should be lighter weight than async.
Araq said Nim compiles the state machine to Duff's Device like in C++.
]#
import deques
import math
import times
import typetraits

const
  FRAMES = 2 # future frames to keep
  DISTANCE_TO_MINE = 10
  WORKER_SPEED = 1
  WORKER_CAPACITY = 1
  FRAMES_TO_MINE = 10
  NUM_TASKS = 1000
  FRAMES_TO_RUN = 1000000

type
  Worker = ref object of RootObj
    total_mined: int
    ticks: int
    carrying: int
    position: int
    mining_progress: int

proc newWorker(): Worker =
  var w = Worker(
    carrying: 0, 
    position: 0, 
    mining_progress: 0,
    ticks: 0,
    total_mined: 0,
  )
  return w

proc gather(this: Worker) =
  this.ticks += 1
  this.mining_progress += 1
  if(this.mining_progress >= FRAMES_TO_MINE):
    this.carrying = WORKER_CAPACITY
    this.mining_progress = 0

proc isMining(this: Worker): bool =
  return this.mining_progress > 0

proc dropoff(this: Worker) =
  this.total_mined += this.carrying
  this.carrying = 0

proc moveToMine(this: Worker) =
  this.ticks += 1
  this.position += WORKER_SPEED

proc moveToHome(this: Worker) =
  this.ticks += 1
  this.position -= WORKER_SPEED

proc isAtMine(this: Worker): bool =
  return this.position >= DISTANCE_TO_MINE

proc isAtHome(this: Worker): bool =
  return this.position <= 0

type
  TaskStatusCode = enum
    next_frame, done, wait_for
  Task = iterator (): TaskStatus
  TaskStatus = object
    code*: TaskStatusCode
    next*: Task
    caller: Task
  TaskContext = ref object
    caller: Task
    worker: Worker

proc newTaskStatus(code: TaskStatusCode, next: Task=nil): TaskStatus =
  TaskStatus(
    code: code,
    next: next,
  )

proc newTaskContext(w: Worker): TaskContext =
  result = TaskContext(
    worker: w,
    caller: nil, 
  )

iterator a1(): TaskStatus {.closure.} =
  echo "a1: A"
  yield newTaskStatus(next_frame)
  echo "a1: B"
  yield newTaskStatus(next_frame)
  echo "a1: C"
  yield newTaskStatus(next_frame)
  echo "a1: D"
  return newTaskStatus(done)

iterator a2(): TaskStatus {.closure.} =
  echo "a2: A"
  yield newTaskStatus(next_frame)
  echo "a2: B"
  yield newTaskStatus(next_frame)
  echo "a2: C"
  return newTaskStatus(done)

proc goToMine(this: TaskContext): Task  =
  #echo "in goToMine"
  let it = iterator(): TaskStatus {.closure.} =
    while not this.worker.isAtMine():
      #echo "moving"
      this.worker.moveToMine()
      yield newTaskStatus(next_frame)
    #echo "at mine"
    return newTaskStatus(done, this.caller)
  result = it

proc doGather(this: TaskContext): Task =
  result = iterator(): TaskStatus {.closure.} =
    while true:
      this.worker.gather()
      yield newTaskStatus(next_frame)
      if not this.worker.isMining():
        break
    return newTaskStatus(done, this.caller)

proc doDropoff(this: TaskContext): Task =
  result = iterator(): TaskStatus {.closure.} =
    while not this.worker.isAtHome():
      this.worker.moveToHome()
      yield newTaskStatus(next_frame)
    this.worker.dropoff()
    return newTaskStatus(done, this.caller)

proc runWorker(this: TaskContext): Task =
  let it = iterator(): TaskStatus {.closure.} =
    while true:
      #echo "wait for goToMine"
      yield newTaskStatus(wait_for, goToMine(this))
      #echo "wait for doGather"
      yield newTaskStatus(wait_for, doGather(this))
      #echo "wait for doDropoff"
      yield newTaskStatus(wait_for, doDropoff(this))
  this.caller = it
  return it

type
  TaskManager* = ref object of RootObj
    frames*: Deque[seq[Task]]
    total_mined: int
    workers: seq[Worker]

proc newTaskManager(): TaskManager = 
  var tm = TaskManager(
    frames: initDeque[seq[Task]](),
    total_mined: 0
  )
  for i in 1..FRAMES:
    tm.frames.addLast(newSeq[Task]())
  return tm

proc addTask(this: var TaskManager, t: TaskContext) =
  this.workers.add(t.worker)
  this.frames[0].add(t.runWorker())

proc popFrame(this: var TaskManager) =
  this.frames.popFirst()
  this.frames.addLast(newSeq[Task]())

proc runTask(this:var TaskManager) =
  let x: Task = this.frames[0].pop()
  let status: TaskStatus = x()
  if status.code == done:
    if status.next != nil:
      this.frames[0].add(status.next)
  elif status.code == next_frame:
    this.frames[1].add(x)
  elif status.code == wait_for:
    this.frames[0].add(status.next)

proc runFrame(this: var TaskManager) =
  while this.frames.peekFirst().len() > 0:
    this.runTask()
  this.popFrame()

proc step(this: var TaskManager) =
  if this.frames.peekFirst().len() == 0:
    this.popFrame()
  else:
    this.runTask()

var tm = newTaskManager()

for i in 1..NUM_TASKS:
  var w = newWorker()
  var tc = newTaskContext(w)
  tm.addTask(tc)

let start_t = getTime()
for i in 1..FRAMES_TO_RUN:
  #tm.runFrame()
  tm.step()
let end_t = getTime()
let diff = end_t - start_t
for w in tm.workers:
  tm.total_mined += w.total_mined
echo "Mined a total of " & $tm.total_mined & " took " & $diff

#[ Mined a total of 27000 took 18 seconds, 456 milliseconds, 343 microseconds, and 600 nanoseconds
in release mode, not as good as await; the total is off, which means some steps are doing nothing.
Also the GC is probably killing the speed here.
]#