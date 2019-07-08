#[ Simulate the C++ switch-based coroutine with case statements as a baseline,
as a basic state machine. Try to expand it to use frame containers for tasks,
and to execute per frame instead of per step to see if it will hurt performance.

Trying to emit switch statements for Duff's device won't work because Nim
messes up the braces.
]#

import math
import times
import circular-queue/circular_queue

const
  FRAMES = 2 # future frames to keep
  DISTANCE_TO_MINE = 10
  WORKER_SPEED = 1
  WORKER_CAPACITY = 1
  FRAMES_TO_MINE = 10
  NUM_TASKS = 1000
  FRAMES_TO_RUN = 1000000

type
  Worker = ref object
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
  Task* = ref object of RootObj
    worker: Worker
    yield_point: int
    caller: Task

#proc newTask(): ptr Task =
#  result = cast[ptr Task](alloc0(sizeof(Task)))

proc newTask(): Task =
  result = Task()

let end_coro*: Task = newTask()
let next_frame*: Task = newTask()

method run(this: Task): Task {.base.}=
  return end_coro

template LINE_NUM*(): auto =
  instantiationInfo().line

type
  GoToMine = ref object of Task
  
proc newGoToMine(w: Worker, c: Task): GoToMine =
  GoToMine(worker: w, caller: c)

method run(this: GoToMine): Task =
  if not this.worker.isAtMine():
    this.worker.moveToMine()
    return next_frame
  else:
    return end_coro

type Gather = ref object of Task

method run(this: Gather): Task =
  case this.yield_point
  of 0:
    this.worker.gather()
    this.yield_point = 1
    return next_frame
  of 1:
    if not this.worker.isMining():
      this.yield_point = 0
      return end_coro
    else:
      this.worker.gather()
      return next_frame
  else:
    raise newException(IndexError, "invalid yield point")

type Dropoff = ref object of Task

method run(this: Dropoff): Task =
  if not this.worker.isAtHome():
    this.worker.moveToHome()
    return next_frame  
  else:
    this.worker.dropoff()
    return end_coro

type MainTask = ref object of Task

method run(this: MainTask): Task =
  case this.yield_point
  of 0:
    this.yield_point = 1
    #echo "going to mine"
    return GoToMine(worker: this.worker, caller: this)
  of 1:
    this.yield_point = 2
    #echo "starting to gather"
    return Gather(worker: this.worker, caller: this)
  of 2:
    this.yield_point = 0
    #echo "going to dropoff"
    return Dropoff(worker: this.worker, caller: this)
  else:
    raise newException(IndexError, "invalid yield point")

type TaskManager = object
    q: CircularQueue[NUM_TASKS, Task]
    tasks: seq[Task]

proc newTaskManager(): TaskManager =
  TaskManager(
    q: newCircularQueue[NUM_TASKS,Task](), 
    tasks: newSeq[Task]()
  )

proc run(this: var TaskManager) =
  while not this.q.isEmpty():
    var curr = this.q.pop()
    var awt = curr.run()
    if awt == next_frame:
      this.q.add(curr)
      break
    elif awt == end_coro and curr.caller != nil:
      this.q.add(curr.caller)
      continue
    elif awt.caller != nil:
      this.q.add(awt)
      continue

proc addTask(this: var TaskManager) =
  let t = MainTask(worker: newWorker(), caller: nil)
  this.tasks.add(t)
  this.q.add(t)

echo "Coroutine simulation with case and if statements"
var taskmanager = newTaskManager()

for i in 1..NUM_TASKS:
  taskmanager.addTask()
let start_t = getTime()

var frames = 0
for i in 1..FRAMES_TO_RUN:
  #echo "frame: " & $i
  taskmanager.run()
  inc frames
let end_t = getTime()
let diff = end_t - start_t
var total_mined = 0
var total_ticks = 0

for t in taskmanager.tasks:
  total_mined += t.worker.total_mined
  total_ticks += t.worker.ticks

echo "ran " & $frames & " frames, " & $total_ticks & " ticks."
echo "Mined a total of " & $total_mined & " took " & $diff

#[ parameters: 1000/1000000, runs only 1 worker per frame(tick)
  Amazing, now takes 44200400 ns for 1000000 ticks, 44ns per tick
  C++ took           19876503 ns, or 20ns per tick


]#