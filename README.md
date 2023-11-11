# Singleton Pattern are DANGEROUS 
... When you want to control the order of initialization/deinitialization of your objects across shared libraries.

Consider the case where you have a beautiful logger class, which is a singleton, and another class which will put some information to log. It is also totally legit that you want to log when the object is being contructed and being destructed. 

In the object, codes can be like following:
```cpp
sth::sth(){
  logger::instance().log("sth is being constructed");
}
sth::~sth(){
  logger::instance().log("sth is being destructed");
}

// may be in other file, or same file

// This is also possible in case sth is actually a 
// factory and the polymorphic class is registered
// in following pattern
static auto whatever = sth::instance().do_something();
```

In this case, you might think that the order of initialization and deinitialization of the logger and the object is guaranteed. Since C++ standard seems to guarantee for objects static storage duration, they are destructed as if `std::atexit` is right after the completion of the constructor of the object. The finish of construction of `logger` is sequenced before the finish of construction of `sth`. That runtime *should* be destructing `sth` before `logger`.

But the point of [this repository](https://github.com/karuboniru/singleton_pitfall/) is to prove things can go wrong when you are doing this in a complexed
project without properly specifying the dependency of shared libraries.

Just compile the project
```
mkdir build && cd build && cmake .. && cmake --build . 
```

 - `./wrong`
    ```
    Start of sth::sth(), I will try to put logs
    logger::logger() you can start putting logs now
    LOG     std is being initialized
    End of sth::sth(), the runtime will consider I am fully constructed then
    LOG     sth::do_something() is called
    logger::~logger() you should not being putting logs anymore
    LOG     I am dead, I cannot put logs anymore 
            But I will try to put this log: std is being deinitialized
    ```

  - `./right{,1,2}`
    ```
    Start of sth::sth(), I will try to put logs
    logger::logger() you can start putting logs now
    LOG     std is being initialized
    End of sth::sth(), the runtime will consider I am fully constructed then
    LOG     sth::do_something() is called
    LOG     std is being deinitialized
    logger::~logger() you should not being putting logs anymore
    ```

This seems to be legal in C++ standard that the order don't matter when things goes to beyond the boundary of shared libraries. There is some discussions [here](https://stackoverflow.com/questions/54562874/destruction-order-of-static-objects-in-shared-libraries) you can refer to.

And when it comes to the implementation of glibc, things related to destruction and dynamic libraries are:
 - The data structure controlling the destruct of all static objects is `exit_function_list` [structure](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/stdlib/exit.h#L55-L60). This is basicially a chain list containing several `exit_function` structs. Which `exit_function` struct record function pointer to call and information indicating when to call.
 - New entries is always added [to the end of the list](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/stdlib/cxa_atexit.c#L79-L141)
 - When a global object [is done constructed, an entry will be added](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/stdlib/cxa_atexit.c#L32-L70) to the `exit_function_list` structure to register the destructor of the object.
 - When `ld.so` loads any dynamic library, it will initialize all global objects from it. 

Now we are starting the program:
 - `ld.so` loads every dynamic libraries, setup relocating things. And then initialize all global static objects from dynamic libraries. The control flow is given back to `__libc_start_main_impl`
 - In `__libc_start_main_impl`, if the program is dynamically linked, [`_dl_fini`](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/csu/libc-start.c#L311-L312) will be registered to the `exit_function_list` structure. This happens after dynamic linked libraries finished loading and before `main()` is called.
 - `__libc_start_main_impl` will then initialize all global static objects in the same TU where `main()` is.
 - Then `main()` is called, and the program starts, control flow is now fully in the hands of the programmer.
 - When `main()` is about to return, `exit_function_list` looks lile:

    ```
                dl_init()                          __libc_start_main_impl()
                    │                                │                 │
                    │                                │                 │
                    │                                │                 │
                    │                          ┌─────┘                 │
                    │                          │                       │
                    │                          │                       │
    ┌───────────────▼──────────────────┬───────▼───────┬───────────────▼──────────────────┬────────────────────────────┐
    │        Destructors of            │               │        Destructors of            │      Destructors of        │
    │                                  │               │                                  │                            │
    │ Global Objects Initialized From  │  _dl_fini()   │ Global Objects Initialized From  │  Objects Initialized From  │
    │                                  │               │                                  │                            │
    │        Dynamic Libraries         │               │            Main.o                │    Ordinary Control Flow   │
    └──────────────────────────────────┴───────────────┴──────────────────────────────────┴────────────────────────────┘
    ```

 - When program exits, `__GI_exit` will be called and it will call `__run_exit_handlers`, which will iterate through the `exit_function_list` structure and call the destructors in the reverse order of initialization.
 - For objects initialized after `_dl_fini` is registered, the destructors will be called in the reverse order of initialization, this is the ordinary order as we expected and stated in [std::exit()](https://en.cppreference.com/w/cpp/utility/program/exit)
 - `_dl_fini` is a bit tricky since it will [do a resort of loaded shared libraries](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/elf/dl-fini.c#L91-L94), by the order that one will be in front of those it depends on. After the sort, `_dl_call_fini` for each of the shared library [will be called in the order of the sorted list](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/elf/dl-fini.c#L114).
 - `_dl_call_fini` calls `__cxa_finalize` with the address of `__dso_handle` of given shared library as argument. `__cxa_finalize` also iterates through the `exit_function_list` structure and call the destructors in the reverse order of initialization, but [it will only select the destructors that are registered with the given `__dso_handle`](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/stdlib/cxa_finalize.c#L25-L27) in this case. So `_dl_call_fini` will only destory local static objects that initialized from `anything::get_instance()` inside the shared library.
 - Both `__cxa_finalize` and `__run_exit_handlers` will set a flag in the `exit_function_list` structure after calling the destructor, so that the destructor will not be called again when other `__cxa_finalize` or `__run_exit_handlers` is iterating through the entry twice.


So, if we don't properly specify the dependency of shared libraries, and the initialization of those local static objects are initialized from shared libraries, `__run_exit_handlers` will call `_dl_fini` before looking at those entries of global objects, and `_dl_fini` will lead to the destruction of the global objects in the order given by the dependency of shared libraries, not given by the order of initialization.

And if we did not specify the dependency of shared libraries properly, it becomes the order of linking. This leads to **wrong** order of destruction of global objects in some cases.

## Recommendation if you really want to get the order right
 - Avoid using singleton pattern across shared libraries.
 - Avoid using other global objects in destructors of global objects if you have to do above.
 - If you still have to do above, always to correctly specify the dependency of shared libraries.
    - In this case, `sth` uses `logger`, so `sth` should be linked to `logger`
    - Or order the shared libraries in the linking command, so that the dependency is the correct order that library is in front of those it depends on.
 - If you still don't want to do as suggested, don't be surprised when things go wrong.
 - And this is only the conclusion I got from the implementation of glibc, I am not sure if this is the case for things like `vc++` or things from MacOS. :(

