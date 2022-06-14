# Thread Pool

The implement for thread pool. This is help for manage threads, tasks so easy and safely.  
Required:
- C++ 14 is least
- CMake: minimum 3.20 to run examples.  

### Add to your project:  
Just copy files in `src/*` to your project to using threadpool

### Run examples
`mkdir build && cd build`  
`cmake ..`  
`cmake --build .`  
`./thread_pool`  

## Example  
Create thread pool  
`auto pool = ThreadPool(2, std::thread::hardware_concurrency(), 60s);`

Pusk a task for thread pool
```
pool.emplace<RunnableExample>("#run 1000 miles#");
pool.emplace<RunnableExample>("#cooking breakfast#");
pool.emplace<RunnableExample>("#feed the cat#");
pool.emplace<RunnableExample>("#take out the trash#");
pool.emplace<RunnableExample>("#play chess with grandma#");
pool.emplace<RunnableExample>("#blah blah blah#");
pool.emplace<RunnableExample>("#marry...#");
```

Notify stop thread pool  
`pool.terminate()`

Wait until workers actually finish.  
`pool.wait()`

Is there an easier way for you to use threadpool?

## Options with cmake  
Enable `TP_CONSOLE` if you want to show activities of a threadpool in the console.
