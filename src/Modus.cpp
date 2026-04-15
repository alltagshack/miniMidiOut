#include <iostream>
#include "Modus.h"

Modus::Modus (Modus::Value val) : _modus(val){}

void Modus::set (Modus::Value val) {
    _modus = val;
}

Modus::Value Modus::get () {
    return _modus;
}

void Modus::next () {
    if (_modus == Value::SINUS) _modus = Value::SAW;
    else if (_modus == Value::SAW) _modus = Value::SQUARE;
    else if (_modus == Value::SQUARE) _modus = Value::TRIANGLE;
    else if (_modus == Value::TRIANGLE) _modus = Value::NOISE;
    else if (_modus == Value::NOISE) _modus = Value::SINUS;
    else ;
}

void Modus::print () {
    if (_modus == Value::SINUS) std::cout << "~~~~ sinus\n";
    else if (_modus == Value::SAW) std::cout << "//// saw\n";
    else if (_modus == Value::SQUARE) std::cout << "_||_ square\n";
    else if (_modus == Value::TRIANGLE) std::cout << "/\\/\\ triangle\n";
    else if (_modus == Value::NOISE) std::cout << "XXXX noise\n";
    else ;
}

void Modus::set (char m) {
    if (m == '\0') {
        next();
        print();
        return;
    }

    if (m == 'i')
        _modus = Value::SINUS;
    else if (m == 'a')
        _modus = Value::SAW;
    else if (m == 'q')
        _modus = Value::SQUARE;
    else if (m == 'r')
        _modus = Value::TRIANGLE;
    else if (m == 'o')
        _modus = Value::NOISE;
    else ;
    print();
}
