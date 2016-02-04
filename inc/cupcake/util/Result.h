
#ifndef CUPCAKE_RESULT_H
#define CUPCAKE_RESULT_H

#include "cupcake/util/Error.h"

/*
 * Baseically a std::pair<T, Error> for when a valid result or an error might
 * be possible. Has some convenience methods.
 *
 * This expects an object with a trivial default constructor. Will destroy the
 * returned object unless it's transfered with get().
 */
template<typename T>
class Result {
public:
    explicit Result(T& value) : obj(value), err(0) {}
    Result(T&& value) : obj(std::move(value)), err(0) {}
    explicit Result(Error error) : obj(), err(error) {}
    Result(const Result&) = default;
	Result(Result&& other) : obj(std::move(other.obj)), err(other.err) {}
    Result& operator=(const Result&) = default;
	Result& operator=(Result&& other) {
		if (this != other) {
			obj = std::move(other.obj);
			err = std::move(other.err);
		}
		return *this;
	}
    ~Result() = default;
    
    bool ok()  const {
        return err == 0;
    }
    
    Error error() const {
        return err;
    }
    
    T& get() {
        return obj;
    }
    
private:
    T obj;
    Error err;
};

#endif // CUPCAKE_RESULT_H
