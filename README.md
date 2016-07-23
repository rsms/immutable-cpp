# Immutable++

Persistent immutable data structures providing practically O(1) for appends, updates and lookups based on work by Rich Hickey and by consequence Phil Bagwell.

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

A persistent random-accessible ordered collection of value type `T`.

Synopsis:

```cc
struct Array<T> {
  static ref<Array> empty();
  static ref<Array> create(std::initializer_list<typename Y>&&);
  static ref<Array> create(typename Iterable&&);
  static ref<Array> create(const typename Iterable&);
  static ref<Array> create(Iterator&& begin, const Iterator& end);
  static ref<Array> create(Iterator& begin, const Iterator& end);
  static ref<Array> create(typename It& begin, const typename It& end);
  static ref<Array> create(typename It&& begin, const typename It& end);

  uint32 size() const;
  
  ref<Array> push(Value<T>*) const;
  ref<Array> push(typename Any&&) const;
  ref<Array> push(Iterator&& begin, const Iterator& end) const;
  ref<Array> push(Iterator& begin, const Iterator& end) const;
  ref<Array> push(typename It&& begin, const typename It& end) const;
  ref<Array> push(typename It& begin, const typename It& end) const;

  ref<Array>          set(uint32 i, typename Any&&) const;
  ref<Array>          set(uint32 i, Value<T>*) const;
  const T&            get(uint32 i) const;
  Iterator            find(uint32 i) const;
  const ref<Value<T>> findValue(uint32 i) const;
  const ref<Value<T>> getValue(uint32 i) const;
  const T&            first() const;
  const T&            last() const;
  const ref<Value<T>> firstValue() const;
  const ref<Value<T>> lastValue() const;
  
  ref<Array> pop() const;
  ref<Array> rest() const;
  ref<Array> concat(ref<Array>) const;
  ref<Array> slice(uint32 start, uint32 end=END) const;
  ref<Array> splice(uint32 start, uint32 end, typename It&& it, const typename It& endit) const;
  ref<Array> splice(uint32 start, uint32 end, Iterator&& it) const;
  ref<Array> splice(uint32 start, uint32 end, Iterator& it) const;
  ref<Array> splice(uint32 start, uint32 end, ref<Array>) const;
  ref<Array> without(uint32 start, uint32 end=END) const;
  
  ref<TransientArrayT> asTransient() const;
  ref<Array>           modify(typename Func&& fn) const;
  
  int compare(const ref<Array>& other) const;

  Iterator        begin(uint32 start=0, uint32 end=END) const;
  const Iterator& end() const;
}
```

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
static ref<Array> create(std::initializer_list<typename Y>&&);         // 1
static ref<Array> create(typename Iterable&&);                         // 2
static ref<Array> create(const typename Iterable&);                    // 3
static ref<Array> create(Iterator&& begin, const Iterator& end);       // 4
static ref<Array> create(Iterator& begin, const Iterator& end);        // 5
static ref<Array> create(typename It& begin, const typename It& end);  // 6
static ref<Array> create(typename It&& begin, const typename It& end); // 7
```

- Form 1 takes an initializer list of any value type, and then constructs T's using the provided values in the initializer list by moving the values to T's constructor.
- Form 2 and 3 reads values from something that provides begin() and end() which returns [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator) and constructs T's with those values.
- Form 4 and 5 are specializations of 6 & 7 for [Array<T>::Iterator](#arrayiterator) that avoids copying of values (instead it just references the same values.)
- Form 6 and 7 reads values from something that conforms to [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator) and constructs T's with those values.


Examples:

```cc
// Emplacing three T's with values from implicit initializer list
auto a = Array<int>::create({1, 2, 3});

// Constructing T's (std::string) with values of a different type (const char*)
auto b = Array<std::string>::create(
  std::initializer_list<const char*>{"Foo", "Bar"});

// Constructing T's by moving values from an iterable
auto c = Array<int>::create(std::vector({1, 2, 3}));

// Constructoring T's by copying iterator values
std::vector values({1, 2, 3});
auto d = Array<int>::create(values.begin(), values.end());
```

#### size() → uint32

Number of values in the array


#### get(index), first(), last() → T

Directly access value at index without checking that index is within bounds. If index is out-of bounds, the behavior is undefined.

```cc
const T& get(uint32 index) const;
const T& first() const;
const T& last() const;

// Example:
auto a = Array<int>::create({1, 2, 3});
a->get(1); // == 2
a->last(); // == 3
a->get(9); // probably crash from memory violation. probably.
```

#### findValue(index), getValue(index), firstValue(), lastValue() → Value

Access [Value](#value) at index. Form 1 returns nullptr if index is out-of bounds. All other forms have undefined behavior if index is out-of bounds or if the array is empty.

```cc
const ref<Value<T>> findValue(uint32 i) const; // 1
const ref<Value<T>> getValue(uint32 i) const;  // 2
const ref<Value<T>> firstValue() const;        // 3
const ref<Value<T>> lastValue() const;         // 4

// Example:
auto a = Array<int>::create({1, 2, 3});
a->findValue(1)->value; // == 2
a->findValue(9);        // == nullptr
a->lastValue()->value;  // == 3
```

#### find(index) → Iterator
Returns an [Array<T>::Iterator](#arrayiterator) to value at index. If index is out-of bounds, the end() iterator is returned.

```cc
Iterator find(uint32 i) const;

// Example:
auto a = Array<int>::create({1, 2, 3});
auto I = a->find(1);
*I; // == 2
a->find(9) == a->end();
```

#### push(value) → Array
Produces an array with value(s) added to the end. This operation is sometimes called "conj" or "conjoin" in functional programming.

```cc
ref<Array> push(Any&&) const;                                               // 1
ref<Array> push(Value<T>*) const;                                           // 2
ref<Array> push(InputIt&& begin, const InputIt& end) const;                 // 3
ref<Array> push(InputIt& begin, const InputIt& end) const;                  // 4
ref<Array> push(Array::Iterator&& begin, const Array::Iterator& end) const; // 5
ref<Array> push(Array::Iterator& begin, const Array::Iterator& end) const;  // 6
```

- Form 1 constructs a value of type T in-place with Any (anything.)
- Form 2 references a value without any construction or copying of T.
- Form 3 and 4 takes arguments that conforms to [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator). The value type of InputIt must be either [Value<T>](#value) or some value that T can be constructed from, like T itself or an argument accepted by T's constructor.
- Form 5 and 6 are specializations of 3 & 4 for [Array<T>::Iterator](#arrayiterator) that avoids copying values.

Examples:

```cc
// Simple example
auto a = Array<int>::create({1, 2, 3});
a = a->push(4); // a => [1, 2, 3, 4]

// Example of constructing multiple Ts from arguments
// passed via initializer_list:
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

#### cons(value) → Array
Returns an array with value added to the beginning. This operation is sometimes called "push_front" or "shift" in some mutative implementations. Note that you can use `splice` to form arrays with multiple new items added to the beginning.

Form 1 constructs a value of type T in-place with Any (anything), while form 2 references a value without any construction or copying of T.

```cc
ref<Array> cons(typename Any&&) const; // 1
ref<Array> cons(Value<T>*) const;      // 2

// Example:
auto a = Array<int>::create({1, 2, 3});
a = a->cons(0); // => [0, 1, 2, 3]
```

#### set(index, value) → Array
Set value at index, where index must be less than size(). Returns nullptr if i is out-of bounds. For form 1, value can be anything that can be used to construct a T.

```cc
ref<Array> set(uint32 index, typename Any&&) const; // 1
ref<Array> set(uint32 i, Value<T>*) const;          // 2

// Example:
auto a = Array<int>::create({1, 2, 3});
a = a->set(1, 22); // => [1, 22, 3]
```

#### pop() → Array
Returns an array without the last value.

```cc
ref<Array> pop() const;

// Example:
auto a = Array<int>::create({1, 2, 3});
a = a->pop(); // => [1, 2]
```

#### rest() → Array
Returns an array without the first value. This operation is sometimes called "pop_front" or "unshift" in some mutative implementations.

```cc
ref<Array> rest() const;

// Example:
auto a = Array<int>::create({1, 2, 3});
a = a->rest(); // => [2, 3]
```

#### concat(Array) → Array
Returns an array that combines the target array's values followed by the argument array's values.

```cc
ref<Array> concat(ref<Array>) const;

// Example:
auto a = Array<int>::create({1, 2, 3});
auto b = Array<int>::create({4, 5, 6});
a = a->concat(b); // => [1, 2, 3, 4, 5, 6]
```

#### slice(start[, end]) → Array
Returns a slice of the target array, from start up until (but not including) end. If end is not provided, the effect is the same as setting end to Array::size(). nullptr is returned if start and/or end is out-of bounds.

```cc
ref<Array> slice(uint32 start, uint32 end=END) const;

// Example:
auto a = Array<int>::create({1, 2, 3, 4, 5});
a = a->slice(1, 4); // => [2, 3, 4]
```

#### splice(start, end, source...) → Array
Replaces values within the range [start, end) with values from an iterator or another array. Form 1 accepts anything that implements [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator). Form 2 and 3 accepts [Array<T>::Iterator](#arrayiterator). Form 4 uses another array for the source of values to be spliced in.

```cc
ref<Array> splice(uint32 start, uint32 end,
                  typename It&& it, const typename It& endit) const; // 1
ref<Array> splice(uint32 start, uint32 end, Iterator&& it) const;    // 2
ref<Array> splice(uint32 start, uint32 end, Iterator& it) const;     // 3
ref<Array> splice(uint32 start, uint32 end, ref<Array>) const;       // 4

// Example:
auto a = Array<int>::create({1, 2, 3, 4, 5});
auto b = Array<int>::create({66, 77, 88});
a = a->splice(1, 4, b); // => [1, 66, 77, 88, 5]
```

#### without(start[, end]) → Array
Removes values within the range [start, end). Returns null if i is out-of bounds. Note that `without(X,Y)` equivalent to `splice(X,Y,end(),end())`.

```cc
ref<Array> without(uint32 start, uint32 end=END) const;

// Example:
auto a = Array<int>::create({1, 2, 3, 4, 5});
a = a->without(1, 4); // => [1, 5]
```

#### asTransient() → TransientArray
Returns a [TransientArray](#transientarray) contaning the same values as this array. Transient arrays are useful for efficient batch modifications to an array.

```cc
ref<TransientArrayT> asTransient() const;

// Example:
auto a = Array<int>::create({1, 2, 3, 4, 5});
auto t = a->asTransient();
t->set(0, 11)
 ->set(2, 33); // can be chained
t->set(4, 55); // or not, since operations mutate t.
a = t->makePersistent(); // => [11, 2, 33, 4, 55]
```

#### modify(func(TransientArray)) → Array
Apply batch modifications using a transient. This method is really just a convenience for asTransient() ... makePersistent() as seen in the example above.

```cc
ref<Array> modify(typename Func&& fn) const;

// Example:
auto a = Array<int>::create({1, 2, 3, 4, 5});
a = a->modify([] (auto t) { // auto arg is a C++14 feature
  t->set(0, 11)
   ->set(2, 33); // can be chained
  t->set(4, 55); // or not, since operations mutate t.
});
// a => [11, 2, 33, 4, 55]
```

#### compare(Array) → int
Compare the values of the target array with the values in the array passed as an argument. Uses std::less<T> to compare values and thus T must support std::less (e.g. by implementing `operator<`)

```cc
int compare(const ref<Array>& other) const;

// Example:
auto a = Array<std::string>::create({"Foo", "Bar", "Cat"});
auto b = Array<std::string>::create({"Foo", "Bar", "Cat"});
auto c = Array<std::string>::create({"Foo", "Bar", "Catz"});
a->compare(b); // == 0
a->compare(c); // == 1
c->compare(a); // == -1
```

#### begin([start[, endIndex]]), end() → Iterator
begin() returns a new iterator that accesses the range [start,endIndex). If endIndex is not given, it has the same effect as passing `size()`. If start is not given, it has the same effect as passing `0`. end() returns the end iterator.

```cc
Iterator begin(uint32 start=0, uint32 end=END) const;
const Iterator& end() const;

// Example:
auto a = Array<int>::create({1, 2, 3});
for (auto I = a->begin(); I != a->end(); ++I) {
  printf("%d ", *I);
} // output: 1 2 3

// Example of iterating over a range:
auto a = Array<int>::create({1, 2, 3, 4, 5});
for (auto I = a->begin(1, 4); I != a->end(); ++I) {
  printf("%d ", *I);
} // output: 2 3 4

// Example of using non-standard method valid() (not using end())
auto a = Array<int>::create({1, 2, 3});
for (auto I = a->begin(); I.valid(); ++I) {
  printf("%d ", *I);
} // output: 1 2 3

// Range-based for-loops are supported as well:
auto a = Array<int>::create({1, 2, 3});
for (auto& v : *a) {
  printf("%d ", v);
} // output: 1 2 3
```

### Array<T>::Iterator
Iterator that implements [std::forward_iterator](http://en.cppreference.com/w/cpp/concept/ForwardIterator) with additional functionality described below.

Synopsis:

```cc
struct Array<T>::Iterator { // implements std::forward_iterator
  // is: movable, copyable, move-assignable, copy-assignable, comparable.

  // O(1) distance calculation
  difference_type distanceTo(const Iterator& rhs) const;

  // Access value
  T&              operator*();
  Value<T>*       value();
  const Value<T>* value() const;

  // True when this iterator has a value
  bool valid() const;
}
```

## TransientArray<T>

A non-persistent random-accessible ordered collection of value type `T` that provides a subset of the functionality of [Array](#array) but with more efficient modifications, making it suitable for batch updates and modifications. Transient arrays are *not* thread safe.

The following is a short synopsis instead of full documentation as all the methods listed here are documented under `Array` and work in the same way with the only exception that they mutate the target `TransientArray` rather than returning new arrays.

```cc
struct TransientArray<T> {
  uint32              size() const;
  ref<TransientArray> push(Value<T>*);
  ref<TransientArray> push(typename Any&&);
  ref<TransientArray> set(uint32 i, typename Any&&);
  ref<TransientArray> set(uint32 i, Value<T>*);
  const ref<Value<T>> findValue(uint32 i) const;
  const ref<Value<T>> getValue(uint32 i) const;
  const T&            get(uint32 i) const;
  ref<TransientArray> pop();
  const T&            first() const;
  const T&            last() const;
  const ref<Value<T>> firstValue() const;
  const ref<Value<T>> lastValue() const;
}
```

Note that [Array::create](#create--array) is implemented using a transient array for efficient creation, so there should be no need to use TransientArray for trivial array creation.

#### makePersistent() → Array
"Seals" the array and returns a persistent [Array](#array) which refers to the same underlying data.
Returns null if this transient array is not editable (e.g. `makePersistent()` has already been called.)

```cc
ref<Array<T>> makePersistent();

// Example:
// acquire Array<t> a in some way...
auto t = a->asTransient();
// apply changes to t ...
a = t->makePersistent(); // => Array<T>
a = t->makePersistent(); // => nullptr -- already sealed and referenced
```


## Value<T>

A reference-counted container for any value. Copying a `Value<T>` does not cause the underlying value to be copied, but instead just referenced in a thread-safe manner.

Synopsis:

```cc
struct Value<T> {
  T value; // value is stored here

  // construct T in-place
  Value(typename Args&&...);

  // allow implicit cast to T
  operator T&() { return value; }
  operator const T&() const { return value; }

  // Value is movable, copyable, move-assignable, copy-assignable, and comparable
}
```

## ref<T>

Trivial memory-pointer manager that implements automatic reference counting (thread-safe, intrusive.)

Synopsis:

```cc
struct ref<T> {
  ref();                       // == ref(nullptr)
  ref(T*);                     // retains a reference
  ref(const ref<T>&);          // retains a reference
  ref(ref<T>&&);               // takes over reference
  ref(const ref<typename U>&); // retains a reference to U::ptr()
  ~ref();                      // releases its reference

  // access to T
  T* ptr() const;
  operator T*() const;
  T* operator->() const;

  ref<T>& operator=(T*);            // releases its reference, then retains a reference
  ref<T>& operator=(const ref<T>&); // releases its reference, then retains a reference
  ref<T>& operator=(ref<T>&&);      // releases its reference, then takes over a reference
  ref<T>& operator=(const ref<typename U>&); // like operator=(T*) but gets pointer from U::ptr()

  void swap(T**);     // swap target's pointer with argument's pointer
  void swap(ref<T>&); // swap target with argument
protected:
  T* _ptr;
}
```

`ref` requires `T` to implement the following interface:

```cc
struct RefCounted {
  virtual void retain() const =0;
  virtual bool release() const =0; // true if count is zero and obj was deleted
  virtual bool hasSingleRef() const =0;
};
```


## Learn more

To learn more about the inner workings of the implementation, consider reading the ["Understanding Clojure's Persistent Vectors" series of blog posts](http://hypirion.com/musings/understanding-persistent-vector-pt-1).

To learn more about the concept of immutable data structures, check out Chris Okasaki's ["Purely functional data structures"](https://www.cs.cmu.edu/~rwh/theses/okasaki.pdf).

For a longer list of functional data structure reading, see ["New purely functional data structures published since 1998" on Stack Exchange](http://cstheory.stackexchange.com/questions/1539/whats-new-in-purely-functional-data-structures-since-okasaki#answer-1550).


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
