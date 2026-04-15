#ifndef __MODUS_H
#define __MODUS_H 

struct Modus {
    enum class Value { SINUS, SAW, SQUARE, TRIANGLE, NOISE };

    Modus(Value);
    Value get ();
    void set (Value);
    void set (char);
    void next ();
    void print ();

    bool operator==(Value v) const noexcept { return _modus == v; }
    bool operator!=(Value v) const noexcept { return _modus != v; }

private:
    Value _modus;
};

#endif
