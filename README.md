# Immutable++

Persistent immutable data structures.

```cc
auto a = Array<int>::empty();
a = a->push(1);
a = a->push(2);
a = a->push(3);

auto b = a->push(4);
b = b->set(2, 33);

for (auto& value : *a) {
  printf("%d ", value);
}
// output: 1 2 3

for (auto& value : *b) {
  printf("%d ", value);
}
// output: 1 2 33 4
```

Configure:
```sh
./configure.py --debug
```

To build and run tests:
```sh
ninja && build/debug/bin/test
```

To build and debug:
```sh
ninja && lldb -bo r build/debug/bin/test
```

## Array<T>

A random-accessible ordered collection of values of type `T`.

#### empty() → Array
The empty array

```cc
ref<Array> Array::empty();
```

Example:

```cc
auto a = Array<int>::empty();
auto b = Array<std::string>::empty();
assert(a->size() == 0 && b->size() == 0);
assert(a.ptr() == b.ptr()); // actually same underlying memory
```

#### create(...) → Array
Create an array from values.

```cc
ref<Array> Array::create(std::initializer_list<Y>&&);                          // 1
ref<Array> Array::create(InputIterable&&);                                     // 2
ref<Array> Array::create(const InputIterable&);                                // 3
ref<Array> Array::create(InputIt& begin, const InputIt& end);                  // 4
ref<Array> Array::create(InputIt&& begin, const InputIt& end);                 // 5
ref<Array> Array::create(Array::Iterator&& begin, const Array::Iterator& end); // 6
ref<Array> Array::create(Array::Iterator& begin, const Array::Iterator& end);  // 7
```

- Form 2 and 3 takes something that provides begin() and end() which returns [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator)
- Form 4 and 5 takes something that conforms to [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator)
- Form 6 and 7 are specializations of 4 & 5 for Array::Iterator that avoids copying of values (instead it just references the same values.)

Example:

```cc
auto a = Array<int>::create({1, 2, 3});
```

#### size() → uint32
Number of values in the array

#### push(value) → Array
Produces an array with value(s) added to the end

```cc
ref<Array> push(Any&&) const;                                               // 1
ref<Array> push(Value<T>*) const;                                           // 2
ref<Array> push(InputIt&& begin, const InputIt& end) const;                 // 3
ref<Array> push(InputIt& begin, const InputIt& end) const;                  // 4
ref<Array> push(Array::Iterator&& begin, const Array::Iterator& end) const; // 5
ref<Array> push(Array::Iterator& begin, const Array::Iterator& end) const;  // 6
```

- Form 1 constructs a value of type T in-place with Any (anything.)
- Form 2 references a value without constructing, copying or moving a T.
- Form 3 and 4 takes arguments that conforms to [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator). The value type of InputIt must be either Value<T> or some value that T can be constructed from, like T itself or an argument accepted by T's constructor.
- Form 5 and 6 are specializations of 3 & 4 for Array::Iterator that avoids copying values.

Basic example:

```cc
auto a = Array<int>::create({1, 2, 3});
a = a->push(4);
for (auto& v : *a) {
  printf("%d ", v);
}
// output: 1 2 3 4
```

Example of constructing multiple Ts from arguments passed via initializer_list:

```cc
struct Monster {
  Monster(int hp, int xp=1) : hp(hp), xp(xp) {}
  int hp;
  int xp;
};
auto monsters = Array<Monster>::empty();
auto args = std::initializer_list<int>{10, 15, 20};
monsters = monsters->push(args.begin(), args.end());
for (auto& monster : *monsters) {
  printf("hp: %d, xp: %d\n", monster.hp, monster.xp);
}
// output:
// hp: 10, xp: 1
// hp: 15, xp: 1
// hp: 20, xp: 1
```



## MIT license

Copyright (c) 2016 Rasmus Andersson <http://rsms.me/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
