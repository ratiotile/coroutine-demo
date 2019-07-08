# Circular Queues improved it to 7110903000ns @1000/1000 frames
# and 7084399600 @1000/1000000 steps, seems to be worse
# switching back to seq for futures @1000/1000000 step: 6122277400, about the same
# with the asserts compiled out: 6095274000 ns, slightly better
# let's try switching the inner seqs back to circular arrays.
# @1000/1000000 step: 7010390200 nc, which isn't that much better than before