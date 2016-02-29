
#ifndef CUPCAKE_RESULT_H
#define CUPCAKE_RESULT_H

/*
 * Baseically a std::pair<T, Error> for when a valid result or an error might
 * be possible. Has some convenience methods.
 *
 * This expects an object with a trivial default constructor. Will destroy the
 * returned object unless it's transfered with get().
 */
template<typename T, typename E>
class Result {
public:
    explicit Result(T& value) : obj(value), err() {}
    Result(T&& value) : obj(std::move(value)), err() {}
    explicit Result(E error) : obj(), err(error) {}
    Result(const Result&) = default;
	Result(Result&& other) : obj(std::move(other.obj)), err(other.err) {}
    Result& operator=(const Result&) = default;
    Result& operator=(Result&& other) = default;
    ~Result() = default;
    
    bool ok()  const {
        return err == E();
    }
    
    E error() const {
        return err;
    }
    
    T& get() {
        return obj;
    }
    
private:
    T obj;
    E err;
};

#endif // CUPCAKE_RESULT_H
