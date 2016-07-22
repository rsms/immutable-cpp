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
  
  ref<Array> push(ValueT*) const;
  ref<Array> push(typename Any&&) const;
  ref<Array> push(Iterator&& begin, const Iterator& end) const;
  ref<Array> push(Iterator& begin, const Iterator& end) const;
  ref<Array> push(typename It&& begin, const typename It& end) const;
  ref<Array> push(typename It& begin, const typename It& end) const;

  ref<Array>        set(uint32 i, typename Any&&) const;
  ref<Array>        set(uint32 i, ValueT*) const;
  const T&          get(uint32 i) const;
  Iterator          find(uint32 i) const;
  const ref<ValueT> findValue(uint32 i) const;
  const ref<ValueT> getValue(uint32 i) const;
  const T&          first() const;
  const T&          last() const;
  const ref<ValueT> firstValue() const;
  const ref<ValueT> lastValue() const;
  
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


#### get(index), first(), last() → T

Directly access value at index without checking that index is within bounds. If index is out-of bounds, the behavior is undefined.

```cc
const T& get(uint32 index) const;
const T& first() const;
const T& last() const;
```

#### findValue(index), getValue(index), firstValue(), lastValue() → Value

Access value at index with bounds checks. Form 1 returns nullptr if index is out-of bounds. All other forms have undefined behavior if index is out-of bounds or if the array is empty.

```cc
const ref<ValueT> findValue(uint32 i) const; // 1
const ref<ValueT> getValue(uint32 i) const;  // 2
const ref<ValueT> firstValue() const;        // 3
const ref<ValueT> lastValue() const;         // 4
```

#### find(index) → Iterator
Returns an iterator to value at index. Returns the end iterator if index is out-of bounds.

```cc
Iterator find(uint32 i) const;
```

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

#### set(index, value) → Array
Set value at index, where index must be less than size(). Returns nullptr if i is out-of bounds. For form 1, value can be anything that can be used to construct a T.

```cc
template <typename Any> ref<Array> set(uint32 index, Any&&) const; // 1
ref<Array> set(uint32 i, ValueT*) const;                           // 2
```

#### pop() → Array
Remove the last item.

```cc
ref<Array> pop() const;

// Example:
auto a = Array<int>::create({1, 2, 3});
a = a->pop(); // => [1, 2]
```

#### rest() → Array
Returns an array with all items but the first. Equiv to slice(1). Called "pop_front" or "shift" in some mutative implementations.

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
Replaces values within the range [start, end) with values from an iterator or another array. Form 1 accepts anything that implements [std::input_iterator](http://en.cppreference.com/w/cpp/concept/InputIterator). Form 2 and 3 accepts Array<T>::Iterator. Form 4 uses another array for the source of values to be spliced in.

```cc
template <typename It>
ref<Array> splice(uint32 start, uint32 end, It&& it, const It& endit) const; // 1
ref<Array> splice(uint32 start, uint32 end, Iterator&& it) const;            // 2
ref<Array> splice(uint32 start, uint32 end, Iterator& it) const;             // 3
ref<Array> splice(uint32 start, uint32 end, ref<Array>) const;               // 4

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
Returns a TransientArray contaning the same values as this array. Transient arrays are useful for efficient batch modifications to an array.

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
template <typename F> ref<Array> modify(F&& fn) const;

// Example: (C++14 specific because we use "auto" lambda argument)
auto a = Array<int>::create({1, 2, 3, 4, 5});
a = a->modify([] (auto t) {
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
```

#### begin([start[, endIndex]]), end() → Iterator
begin() returns a new iterator that accesses the range [start,endIndex). If endIndex is not given, it has the same effect as passing `size()`. If start is not given, it has the same effect as passing `0`. end() returns the end iterator.

```cc
Iterator begin(uint32 start=0, uint32 end=END) const;
const Iterator& end() const;
```

### Array<T>::Iterator
```cc
struct Array<T>::Iterator { // implements std::forward_iterator
  // is: movable, copyable, move-assignable, copy-assignable, comparable.

  // O(1) distance calculation
  difference_type distanceTo(const Iterator& rhs) const;

  // Access value
  T& operator*();
  ValueT* value();
  const ValueT* value() const;

  // number of items remaining until this==end()
  uint32 remaining() const;
}
```

## TransientArray<T>

A non-persistent random-accessible ordered collection of value type `T` that provides a subset of the functionality of `Array` but with more efficient modifications, making it suitable for batch updates to `Array`s.

The following is a short synopsis instead of full documentation as all the methods listed here are documented under `Array` and work in the same way with the only exception that they mutate the target `TransientArray` rather than returning new arrays.

```cc
struct TransientArray<T> {
  uint32              size() const;
  ref<TransientArray> push(ValueT*);
  ref<TransientArray> push(typename Any&&);
  ref<TransientArray> set(uint32 i, typename Any&&);
  ref<TransientArray> set(uint32 i, ValueT*);
  const ref<ValueT>   findValue(uint32 i) const;
  const ref<ValueT>   getValue(uint32 i) const;
  const T&            get(uint32 i) const;
  ref<TransientArray> pop();
  const T&            first() const;
  const T&            last() const;
  const ref<ValueT>   firstValue() const;
  const ref<ValueT>   lastValue() const;
}
```

#### makePersistent() → Array
"Seals" the array and returns a persistent array which refers to the same underlying data.
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
