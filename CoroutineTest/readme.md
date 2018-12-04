# Coroutine Testing
- MiLi: good old macro-based switch hack
- [nim asyncio]() 
- [libcopp](https://github.com/owt5008137/libcopp) Claims high perf, uses boost.context...
- [cppcoro](https://github.com/lewissbaker/cppcoro) Appears to have a nice api
- [tonbit/coroutine](https://github.com/tonbit/coroutine) Fiber-based coroutine emulation.
- [stackless_coroutine](https://github.com/jbandela/stackless_coroutine) Uses a series of lambdas to simulate stackless coroutines. Seems verbose.
- [co2](https://github.com/jamboree/co2) Looks like it uses same underlying mechanism as stackless_coroutine, adding macros to cut down on syntax.
- [libco](https://github.com/Tencent/libco) Production-ready, but no documentation.
- [continuable](https://naios.github.io/continuable/) Promise-based async, looks high quality.
- [boost.coroutine]() Keeps full stack with boost.context, seems slow.

