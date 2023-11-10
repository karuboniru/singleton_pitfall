# Singleton Pattern are DANGEROUS 
... When you want to control the order of initialization/deinitialization of your objects across shared libraries.

Consider the case where you have a beautiful logger class, which is a singleton, and another 
class which will put some information to log. It is also totally legit that you want 
to log when the object is being contructed and being destructed. 

In the object, codes can be like following:
```cpp
sth::sth(){
  logger::instance().log("sth is being constructed");
}
sth::~sth(){
  logger::instance().log("sth is being destructed");
}
```

But the point of this repository is to prove things can go wrong when you are doing this in a complexed
project without properly specifying the dependency of shared libraries.

This seems to be legal in C++ standard that the order don't matter when things goes to beyond the boundary of shared libraries. There is some discussions [here](https://stackoverflow.com/questions/54562874/destruction-order-of-static-objects-in-shared-libraries) you can refer to.

And when it comes to the implementation of glibc:
 - When a global object is being constructed, an entry will be added to a `exit_function_list` [structure](https://github.com/bminor/glibc/blob/d1dcb565a1fb5829f9476a1438c30eccc4027d04/stdlib/cxa_atexit.c#L73) to register the destructor of the object.
 - Further, if there is any global object being initialized from shared libraries, an entry citing `_dl_fini` will also be added to the `exit_function_list` structure when the shared library loading is finished.
 - When program exits, `__GI_exit` will be called and it will call `__run_exit_handlers`, which will iterate through the `exit_function_list` structure and call the destructors in the reverse order of initialization.
 - So, when the initialize of one global object is triggered from a shared library, `_dl_fini` will be called before the destructor of the global object. And when the global object is initialized from the main program, `_dl_fini` will be called after the destructor of the global objects.
 - `_dl_fini` is even tricky since it will do a resort of loaded shared libraries, by the order that one is in front of those it depends on. After the sort, `_dl_call_fini` for each of the shared library will be called in the order of the sorted list.
 - `_dl_call_fini` calls `__cxa_finalize` with the address of `__dso_handle` of given shared library as argument. `__cxa_finalize` also iterates through the `exit_function_list` structure and call the destructors in the reverse order of initialization, but it will only select the destructors that are registered with the given `__dso_handle`, which usually means the destructors of the global objects in the shared library.
 - Both `__cxa_finalize` and `__run_exit_handlers` will set a flag in the `exit_function_list` structure after calling the destructor, so that the destructor will not be called again when other `__cxa_finalize` or `__run_exit_handlers` is iterating through the `exit_function_list` structure.

So, if we don't properly specify the dependency of shared libraries, and the global objects are initialized from shared libraries, `__run_exit_handlers` will call `_dl_fini` before looking at those entries of global objects, and `_dl_fini` will lead to the destruction of the global objects in the order given by the dependency of shared libraries. 

And if we did not specify the dependency of shared libraries properly, it becomes the order of linking. This leads to **wrong** order of destruction of global objects in some cases.

## Recommendation if you really want to get the order right
 - Avoid using singleton pattern across shared libraries.
 - Avoid using other global objects in destructors of global objects.
 - If you have to, always to correctly specify the dependency of shared libraries.
    - In this case, `sth` uses `logger`, so `sth` should be linked to `logger`
    - Or order the shared libraries in the linking command, so that the dependency is the correct order that library is in front of those it depends on.

