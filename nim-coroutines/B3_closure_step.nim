#[ Tasks using the closure iterator method. Should be lighter weight than async.
Araq said Nim compiles the state machine to Duff's Device like in C++. Flatten
the array of arrays by removing Frames.
]#
import math
import times
import typetraits
import circular-queue/circular_queue

const
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
  TaskCoro = iterator (): Task
  Task = ref object
    run: TaskCoro
    caller: Task
    worker: Worker

let end_coro = Task()
let next_frame = Task()

proc newTask(caller: Task = nil): Task =
  result = Task(
    run: nil,
    caller: caller, 
  )
  if caller != nil:
    result.worker = caller.worker

proc goToMine(this: Task): Task  =
  #echo "in goToMine"
  let task = newTask(this)
  let it = iterator(): Task {.closure.} =
    while not this.worker.isAtMine():
      #echo "moving"
      this.worker.moveToMine()
      yield next_frame
    #echo "at mine"
    return end_coro
  task.run = it
  return task

proc doGather(this: Task): Task =
  result = newTask(this)
  result.run = iterator(): Task {.closure.} =
    while true:
      #echo "mining"
      this.worker.gather()
      yield next_frame
      if not this.worker.isMining():
        break
    return end_coro

proc doDropoff(this: Task): Task =
  result = newTask(this)
  result.run = iterator(): Task {.closure.} =
    while not this.worker.isAtHome():
      #echo "move home"
      this.worker.moveToHome()
      yield next_frame
    #echo "dropoff"
    this.worker.dropoff()
    return end_coro

proc runWorker(w: Worker): Task =
  let t = newTask()
  t.worker = w
  t.run = iterator(): Task {.closure.} =
    while true:
      #echo "wait for goToMine"
      yield goToMine(t)
      #echo "wait for doGather"
      yield doGather(t)
      #echo "wait for doDropoff"
      yield doDropoff(t)
  return t

type
  TaskManager* = ref object
    queue*: CircularQueue[NUM_TASKS, Task]
    workers: seq[Worker]

proc newTaskManager(): TaskManager = 
  var tm = TaskManager(
    queue: newCircularQueue[NUM_TASKS, Task](),
  )
  return tm

proc addTask(this: var TaskManager) =
  let w = newWorker()
  let task = runWorker(w)
  this.workers.add(w)
  this.queue.add(task)

proc step(this: var TaskManager) =
  while not this.queue.isEmpty():
    let x = this.queue.pop()
    let status = x.run()
    if status == next_frame: # await next frame
      this.queue.add(x)
      break
    elif status == end_coro and x.caller != nil: # end of task
      this.queue.add(x.caller)
      continue
    elif status.caller != nil: # new task
      this.queue.add(status)
      continue
 
var tm = newTaskManager()

for i in 1..NUM_TASKS:
  tm.addTask()

let start_t = getTime()
for i in 1..FRAMES_TO_RUN:
  tm.step()
let end_t = getTime()
let diff = end_t - start_t
var total_mined = 0
for w in tm.workers:
  total_mined += w.total_mined
echo "Mined a total of " & $total_mined & " took " & $diff
echo "ticks: " & $tm.workers[0].ticks

#[ Mined a total of 27000 took 18 seconds, 456 milliseconds, 343 microseconds, and 600 nanoseconds
in release mode, not as good as await; the total is off, which means some steps are doing nothing.
Also the GC is probably killing the speed here.

Removing the RootObj: 18307824600 ns, slightly better
Switch to single array queue:
  205526400 ns : 206ns per cycle, 2x slower than await
Simplified indirections:
  045505800 ns : 46ns per cycle, great! nearly as good as case statments
  027503500 ns : 28ns per cycle, with -x:off; a 40% slower than C++
]#