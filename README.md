# Singleton Pattern are BAD 
... When you want to control the order of initialization/deinitialization of your objects.

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
project. 

Specifically, from what I have observed, when you are triggering the initializion of the singleton in some global object initialization, the destruction of classes involved can be dependent on the order of linking.

I don't know the exact reason, because it turns out to be when things goes across boundaries of shared libraries, some definitions from C++ standard are not so clear to me. But please let me know if you have any idea.