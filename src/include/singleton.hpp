#ifndef SINGLETON_HPP
#define SINGLETON_HPP

template<class T, void ** obj_>
class Singleton {
public:
    template <class ...Args>
    static void create(Args... args) {
        *obj_ = new T(args...);
    }
    static T * singleton() {
        return (T *)*obj_;
    }
};

#endif
