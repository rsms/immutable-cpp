#pragma once
#include "base.h"
#include <iterator>

namespace immutable {
  struct ArrayImp;
  template <typename T> struct TransientArray;
  static constexpr uint32 END = 0xffffffff;
  

  // Persistent array (aka vector aka random-access list)
  template <typename T>
  struct Array : RefCounted {
    using ValueT = Value<T>;
    using TransientArrayT = TransientArray<T>;
    struct Iterator;

    // The empty array
    static ref<Array> empty();
    
    // Create an array with values from initializer list
    template <typename Y> static ref<Array> create(std::initializer_list<Y>&&);

    // Create an array with values from iterable
    template <typename Iterable> static ref<Array> create(Iterable&&);
    template <typename Iterable> static ref<Array> create(const Iterable&);
    static ref<Array> create(Iterator&& begin, const Iterator& end);
    static ref<Array> create(Iterator& begin, const Iterator& end);
    template <typename It> static ref<Array> create(It& begin, const It& end);
    template <typename It> static ref<Array> create(It&& begin, const It& end);

    // Number of values in this array
    uint32 size() const { return _end - _start; }
    
    // Append value to the end. Form 2 constructs a value T in-place.
    ref<Array> push(ValueT*) const; // 1
    template <typename Arg> ref<Array> push(Arg&&) const; // 2
    
    // Append values from iterator to end.
    // The value type of the iterator must be either Value<T> or some value that T can
    // be constructed from (like T itself or an argument accepted by T's constructor)
    ref<Array> push(Iterator&& begin, const Iterator& end) const;
    ref<Array> push(Iterator& begin, const Iterator& end) const;
    template <typename It> ref<Array> push(It&& begin, const It& end) const;
    template <typename It> ref<Array> push(It& begin, const It& end) const;

    // Prepend value to the beginning. Form 2 constructs a value T in-place.
    ref<Array> cons(ValueT*) const; // 1
    template <typename Arg> ref<Array> cons(Arg&&) const; // 2

    // Set value at index i, where i must be less than size().
    // Returns nullptr if i is out-of bounds. Form 1 constructs a value T in-place.
    template <typename Arg> ref<Array> set(uint32 i, Arg&&) const; // 1
    ref<Array> set(uint32 i, ValueT*) const; // 2
    
    // Access value at index. If i >= size() the behavior is undefined.
    const T& get(uint32 i) const;

    // Find value at index. Returns the end iterator if index is out-of bounds.
    Iterator find(uint32 i) const;

    // Find value at index. Returns nullptr if index is out-of bounds.
    const ref<ValueT> findValue(uint32 i) const;
    
    // Access value at index. If i >= size() the behavior is undefined.
    const ref<ValueT> getValue(uint32 i) const;
    
    // Access first and last value
    const T& first() const;
    const T& last() const;
    const ref<ValueT> firstValue() const;
    const ref<ValueT> lastValue() const;
    
    // Remove the last item
    ref<Array> pop() const;
    
    // Returns an array with all items but the first. Equiv to slice(1)
    // Called "pop_front" or "shift" in some mutative implementation.
    ref<Array> rest() const;
    
    // Returns a version of this array with other array added to the end
    ref<Array> concat(ref<Array>) const;
    
    // Returns a slice of this array, from start up until (but not including) end.
    // Returns null if start and/or end is out-of bounds.
    ref<Array> slice(uint32 start, uint32 end=END) const;
    
    // Replaces values within the range [start, end) with values from iterator it.
    template <typename It>
    ref<Array> splice(uint32 start, uint32 end, It&& it, const It& endit) const;
    ref<Array> splice(uint32 start, uint32 end, Iterator&& it) const;
    ref<Array> splice(uint32 start, uint32 end, Iterator& it) const;
    
    // Replaces values within the range [start, end) with values from another array.
    ref<Array> splice(uint32 start, uint32 end, ref<Array>) const;
    
    // Removes values within the range [start, end). Returns null if i is out-of bounds.
    ref<Array> without(uint32 start, uint32 end=END) const;
    
    // return a new TransientArray contaning the same values as this array
    ref<TransientArrayT> asTransient() const;
    
    // apply modification with a transient.
    // F should return either Array<T> or TransientArray<T>, e.g.
    //   auto a = Array<int>::create({1,2,3});
    //   a = a->modify([] (ref<TransientArray<int>> t) { // param can be `auto` in C++14
    //     t->set(0, 10)->set(1, 20)->set(2, 30);
    //   });
    //   assert(a->get(1, 20));
    //
    template <typename F> ref<Array> modify(F&& fn) const;
    
    // True if this array has the same values as the other array.
    // Compares values using std::less<T>.
    int compare(const ref<Array>& other) const;
    
    // True if the other array refers to the same underlying data.
    bool operator==(const ref<Array>& rhs) const;
    bool operator!=(const ref<Array>& rhs) const { return !(*this == rhs); }
    
    // Iteration
    Iterator begin(uint32 start=0, uint32 end=END) const;
    const Iterator& end() const;
    
    // forward iterator
    struct Iterator {
      typedef std::forward_iterator_tag iterator_category;
      typedef uint32 difference_type;
      typedef T      value_type;
      typedef T*     pointer;
      typedef T&     reference;
      
      Iterator() {};
      Iterator(const Iterator&) = default; // copyable
      Iterator(Iterator&&) = default; // movable
      
      Iterator& operator++(); // ++i
      Iterator operator++(int); // i++
      Iterator& operator=(const Iterator& rhs) = default;
      Iterator& operator=(Iterator&& rhs) = default;
      
      // O(1) distance calculation
      difference_type distanceTo(const Iterator& rhs) const;

      T& operator*();
      ValueT* value();
      const ValueT* value() const;
      bool valid() const;

      bool operator==(const Iterator& rhs) const;
      bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }
      
    protected:
      friend struct Array;
      friend struct ArrayImp;

      Iterator(const Array* a, uint32 absstart, uint32 absend);
      explicit Iterator(const void*) : _a(nullptr) {} // used by ArrayImp::END_ITERATOR
      
      ref<Array>   _a;
      uint32       _i = 0;
      uint32       _end;
      uint32       _base;
      ref<Object>* _slots = nullptr;
      uint32       _slotlen;
    };
    
    // lower-case name for STL compatibility
    typedef Iterator iterator;
    
    // copyable and movable
    Array(const Array&) = default;
    Array(Array&&) = default;
    Array& operator=(const Array&) = default;
    Array& operator=(Array&&) = default;

  protected:
    friend struct ArrayImp;
    
    uint32      _start; // index offset used when this array is a slice of another array
    uint32      _end;   // _end - _offs = number of values in the list
    uint32      _shift; // BITS times (the depth of this trie minus one)
    ref<Object> _root;  // trie root
    ref<Object> _tail;  // holds the last few entries for efficieny reasons

    Array() = delete; // use Array::empty() instead
    Array(uint32 start, uint32 end, uint32 shift, ref<Object> root, ref<Object> tail);
    Array(ref<Object> root, ref<Object> tail);

    void dealloc() { delete this; }

    IMMUTABLE_REFCOUNTED_IMPL(Array)
  };
  

  // Transient version of Array to be used for efficient batch modifications.
  // Supports only a subset of the operations provided by Array.
  // If there's an operation provided in Array that isn't provided here, it means
  // that using the operation on Array is as efficient as it would be if implemented
  // for a transient.
  template <typename T> struct TransientArray : RefCounted {
    using ValueT = Value<T>;
    
    // Number of items in this array
    uint32 size() const { return _end - _start; }

    // "seal" the transient array and return a persistent array that refers to
    // the same root. Returns null if this transient array is not editable
    // (e.g. makePersistent() has already been called.)
    ref<Array<T>> makePersistent();
    
    // Append value to the end. Form 2 constructs a value T in-place.
    ref<TransientArray> push(ValueT*); // 1
    template <typename Arg> ref<TransientArray> push(Arg&&); // 2
    
    // Set value at index i, where i must be less than size().
    // Returns nullptr if i is out-of bounds. Form 1 constructs a value T in-place.
    template <typename Arg> ref<TransientArray> set(uint32 i, Arg&&); // 1
    ref<TransientArray> set(uint32 i, ValueT*); // 2

    // Find value at index. Returns nullptr if index is out-of bounds.
    const ref<ValueT> findValue(uint32 i) const;
    
    // Find value at index. If i >= size() the behavior is undefined.
    const ref<ValueT> getValue(uint32 i) const;
    
    // Find value at index. If i >= size() the behavior is undefined.
    const T& get(uint32 i) const;

    // Remove the last item
    ref<TransientArray> pop();
    
    // Access first and last value
    const T& first() const;
    const T& last() const;
    const ref<ValueT> firstValue() const;
    const ref<ValueT> lastValue() const;

  private:
    friend struct ArrayImp;
    friend struct Array<T>;
    
    uint32      _start;
    uint32      _end;
    uint32      _shift;
    ref<Object> _root;
    ref<Object> _tail;
    
    TransientArray(uint32 start, uint32 end, uint32 shift, Object* root, Object* tail)
      : _start(start), _end(end), _shift(shift), _root(root), _tail(tail)
    {}
    
    void dealloc() { delete this; }
    
    IMMUTABLE_REFCOUNTED_IMPL(TransientArray)
  };
  
  
  struct ArrayImp {
    static constexpr uint32  BITS     = 5;            // 5, 4, 3, 2 ...
    static constexpr uint32  BRANCHES = 1 << BITS;    // 2^5=32, 2^4=16, 2^3=8, 2^2=4 ...
    static constexpr uint32  MASK     = BRANCHES - 1; // 31 (or 0x1f), 15, 7, 3 ...

    struct N;
    using  A = Array<void*>;
    using  TA = TransientArray<void*>;
    using  ItFunc = std::function<A::ValueT*()>;

    static A EMPTY;
    static N EMPTY_NODE;
    static A::Iterator END_ITERATOR;
    
    // Note: The below functions all expect normalized, absolute indexes.

    // Array
    static ref<Object>* slotsFor(A*, uint32 i, uint32& length); // unchecked
    static Object* findValue(A*, uint32 i); // checked
    static Object* getValue(A*, uint32 i);  // unchecked
    static Object* firstValue(A*); // unchecked
    static A*      set(A*, uint32 i, Object*);
    static A*      push(A*, Object*);
    static A*      cons(A*, Object*);
    static A*      pop(A*);
    static A*      slice(A*, uint32 start, uint32 end);
    static A*      without(A*, uint32 start, uint32 end);
    static A*      splice(A*, uint32 start, uint32 end, A::Iterator& it);
    static A*      splicefn(A*, uint32 start, uint32 end, const ItFunc& next);
    
    // Array -> TransientArray
    static TA*     createTransient(A*);

    // TransientArray
    static ref<Object>* slotsFor(TA*, uint32 i, uint32& length); // unchecked
    static Object* findValue(TA*, uint32 i); // checked
    static Object* getValue(TA*, uint32 i);  // unchecked
    static Object* firstValue(TA*); // unchecked
    static TA*     set(TA*, uint32 i, Object*);
    static TA*     push(TA*, Object*);
    static TA*     pop(TA*);
    
    // TransientArray -> Array
    static A*      createPersistent(TA*);

    struct detail;
  };


  // —————————————————————————————————————————————————————————————————————
  // TransientArray
  
  template <typename T>
  inline ref<Array<T>> TransientArray<T>::makePersistent() {
    return (Array<T>*)ArrayImp::createPersistent((ArrayImp::TA*)this);
  }
  
  
  template <typename T>
  inline ref<TransientArray<T>>
  TransientArray<T>::push(typename TransientArray<T>::ValueT* v) {
    assert(v != nullptr);
    return (TransientArray<T>*)ArrayImp::push((ArrayImp::TA*)this, v);
  }
  
  template <typename T>
  template <typename Arg>
  inline ref<TransientArray<T>> TransientArray<T>::push(Arg&& arg) {
    return push(new ValueT(fwd<Arg>(arg)));
  }
  
  
  template <typename T>
  inline ref<TransientArray<T>> TransientArray<T>::set(uint32 i, ValueT* v) {
    assert(v != nullptr);
    i += _start;
    if (i >= _end) {
      return nullptr; // index out-of bounds
    }
    return (TransientArray<T>*)ArrayImp::set((ArrayImp::TA*)this, i, v);
  }
  
  template <typename T>
  template <typename Arg>
  inline ref<TransientArray<T>> TransientArray<T>::set(uint32 i, Arg&& arg) {
    return set(i, new ValueT(fwd<Arg>(arg)));
  }
  
  
  template <typename T>
  inline const ref<Value<T>> TransientArray<T>::findValue(uint32 i) const {
    i += _start;
    if (i >= _end) {
      return nullptr;
    }
    Object* obj = ArrayImp::findValue((ArrayImp::TA*)this, i);
    if (obj) {
      ImmutableAssertTypeTag(obj, ValueT::TYPE_TAG);
    }
    return static_cast<ValueT*>(obj);
  }
  
  
  template <typename T>
  inline const ref<Value<T>> TransientArray<T>::getValue(uint32 i) const {
    return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::TA*)this, i + _start));
  }
  
  template <typename T>
  inline const T& TransientArray<T>::get(uint32 i) const {
    return getValue(i)->value;
  }
  
  
  template <typename T>
  inline const ref<typename TransientArray<T>::ValueT> TransientArray<T>::firstValue() const {
    if (!size()) {
      return nullptr;
    }
    if (_start == 0) {
      return static_cast<ValueT*>(ArrayImp::firstValue((ArrayImp::TA*)this));
    }
    return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::TA*)this, _start));
  }
  
  template <typename T>
  inline const ref<typename TransientArray<T>::ValueT> TransientArray<T>::lastValue() const {
    if (size()) {
      return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::TA*)this, _end - 1));
    }
    return nullptr;
  }
  
  template <typename T>
  inline const T& TransientArray<T>::first() const {
    return firstValue()->value;
  }
  
  template <typename T>
  inline const T& TransientArray<T>::last() const {
    return lastValue()->value;
  }
  
  template <typename T>
  inline ref<TransientArray<T>> TransientArray<T>::pop() {
    return size() ? (TransientArray<T>*)ArrayImp::pop((ArrayImp::TA*)this) : this;
  }
  
  
  // —————————————————————————————————————————————————————————————————————
  // Array
  
  template <typename T>
  inline ref<Array<T>> Array<T>::empty() {
    return (Array<T>*)&ArrayImp::EMPTY;
  }
  
  //template <typename T>
  //inline Array<T>::Array(ref<TransientArrayT> t)
  //  : _root(ArrayImp::consPersistent(t, this))
  //{
  //  // Note: If t has already been made persistent, this constructs
  //  // a copy of the empty array.
  //}

  template <typename T>
  template <typename It>
  inline ref<Array<T>> Array<T>::create(It& I, const It& E) {
    auto t = empty()->asTransient();
    for (; I != E; ++I) {
      t = t->push(*I);
    }
    return t->makePersistent();
  }
  
  template <typename T>
  template <typename It>
  inline ref<Array<T>> Array<T>::create(It&& I, const It& E) {
    auto t = empty()->asTransient();
    for (; I != E; ++I) {
      t = t->push(*I);
    }
    return t->makePersistent();
  }
  
  // specialization for Array<T>::Iterator
  template <typename T>
  inline ref<Array<T>> Array<T>::create(Iterator&& I, const Iterator& E) {
    auto t = empty()->asTransient();
    for (; I != E; ++I) {
      t = t->push(I.value());
    }
    return t->makePersistent();
  }

  template <typename T>
  inline ref<Array<T>> Array<T>::create(Iterator& I, const Iterator& E) {
    auto t = empty()->asTransient();
    for (; I != E; ++I) {
      t = t->push(I.value());
    }
    return t->makePersistent();
  }
  
  template <typename T>
  template <typename Iterable>
  inline ref<Array<T>> Array<T>::create(Iterable&& vals) {
    auto t = empty()->asTransient();
    auto I = vals.begin();
    auto E = vals.end();
    for (; I != E; ++I) {
      t = t->push(std::move(*I));
    }
    return t->makePersistent();
  }
  
  template <typename T>
  template <typename Iterable>
  inline ref<Array<T>> Array<T>::create(const Iterable& vals) {
    auto t = empty()->asTransient();
    for (auto& v : vals) {
      t = t->push(v);
    }
    return t->makePersistent();
  }
  
  template <typename T>
  template <typename Y>
  inline ref<Array<T>> Array<T>::create(std::initializer_list<Y>&& vals) {
    auto t = empty()->asTransient();
    auto I = vals.begin();
    auto E = vals.end();
    for (; I != E; ++I) {
      t = t->push(std::move(*I));
    }
    return t->makePersistent();
  }
  
  // Constructor only used for initialization of ArrayImp::EMPTY
  template <typename T>
  inline Array<T>::Array(ref<Object> root, ref<Object> tail)
    : Array(0, 0, ArrayImp::BITS, root, tail)
  {
    retain();
  }

  template <typename T>
  inline Array<T>::Array(
    uint32 start, uint32 end, uint32 shift, ref<Object> root, ref<Object> tail)
    : _start(start), _end(end), _shift(shift), _root(root), _tail(tail)
  {}
  
  
  template <typename T>
  inline ref<Array<T>> Array<T>::push(ValueT* v) const {
    return (Array<T>*)ArrayImp::push((ArrayImp::A*)this, v);
  }
  
  template <typename T>
  template <typename Arg> ref<Array<T>> Array<T>::push(Arg&& arg) const {
    return push(new ValueT(fwd<Arg>(arg)));
  }
  
  template <typename T>
  template <typename It>
  inline ref<Array<T>> Array<T>::push(It&& I, const It& E) const {
    return modify([&](ref<TransientArray<T>> t) {
      for (; I != E; ++I) {
        t->push(*I);
      }
    });
  }
  
  template <typename T>
  template <typename It>
  inline ref<Array<T>> Array<T>::push(It& I, const It& E) const {
    return modify([&](ref<TransientArray<T>> t) {
      for (; I != E; ++I) {
        t->push(*I);
      }
    });
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::push(Iterator&& I, const Iterator& E) const {
    return modify([&](ref<TransientArray<T>> t) {
      for (; I != E; ++I) {
        t->push(I.value());
      }
    });
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::push(Iterator& I, const Iterator& E) const {
    return modify([&](ref<TransientArray<T>> t) {
      for (; I != E; ++I) {
        t->push(I.value());
      }
    });
  }
  
  
  template <typename T>
  inline ref<Array<T>> Array<T>::cons(ValueT* v) const {
    return (Array<T>*)ArrayImp::cons((ArrayImp::A*)this, v);
  }
  
  template <typename T>
  template <typename Arg> ref<Array<T>> Array<T>::cons(Arg&& arg) const {
    return cons(new ValueT(fwd<Arg>(arg)));
  }
  
  
  template <typename T>
  inline ref<Array<T>> Array<T>::set(uint32 i, ValueT* v) const {
    assert(v != nullptr);
    i += _start;
    if (i >= _end) {
      return nullptr; // index out-of bounds
    }
    return (Array<T>*)ArrayImp::set((ArrayImp::A*)this, i, v);
  }
  
  template <typename T>
  template <typename Arg>
  inline ref<Array<T>> Array<T>::set(uint32 i, Arg&& arg) const {
    return set(i, new ValueT(fwd<Arg>(arg)));
  }


  template <typename T>
  inline typename Array<T>::Iterator Array<T>::find(uint32 i) const {
    return Iterator(this, _start + i, _end);
  }
  

  template <typename T>
  inline const ref<Value<T>> Array<T>::findValue(uint32 i) const {
    i += _start;
    if (i >= _end) {
      return nullptr;
    }
    Object* obj = ArrayImp::findValue((ArrayImp::A*)this, i);
    if (obj) {
      ImmutableAssertTypeTag(obj, ValueT::TYPE_TAG);
    }
    return static_cast<ValueT*>(obj);
  }
  

  template <typename T>
  inline const ref<Value<T>> Array<T>::getValue(uint32 i) const {
    return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::A*)this, i + _start));
  }
  
  template <typename T>
  inline const T& Array<T>::get(uint32 i) const {
    return getValue(i)->value;
  }
  
  template <typename T>
  inline const ref<typename Array<T>::ValueT> Array<T>::firstValue() const {
    if (!size()) {
      return nullptr;
    }
    if (_start == 0) {
      return static_cast<ValueT*>(ArrayImp::firstValue((ArrayImp::A*)this));
    }
    return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::A*)this, _start));
  }
  
  template <typename T>
  inline const ref<typename Array<T>::ValueT> Array<T>::lastValue() const {
    if (size()) {
      return static_cast<ValueT*>(ArrayImp::getValue((ArrayImp::A*)this, _end - 1));
    }
    return nullptr;
  }
  
  template <typename T>
  inline const T& Array<T>::first() const {
    return firstValue()->value;
  }
  
  template <typename T>
  inline const T& Array<T>::last() const {
    return lastValue()->value;
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::pop() const {
    return size() ? (Array<T>*)ArrayImp::pop((ArrayImp::A*)this)
                  : const_cast<Array<T>*>(this);
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::rest() const {
    return slice(1);
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::concat(ref<Array> other) const {
    return push(other->begin(), other->end());
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::slice(uint32 start, uint32 end) const {
    return (Array<T>*)ArrayImp::slice(
      (ArrayImp::A*)this,
      start + _start,
      end == END ? _end : end + _start
    );
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::without(uint32 start, uint32 end) const {
    return (Array<T>*)ArrayImp::without(
      (ArrayImp::A*)this,
      start + _start,
      end == END ? _end : end + _start
    );
  }
  
  template <typename T>
  inline ref<Array<T>> Array<T>::splice(uint32 start, uint32 end, ref<Array> a) const {
    return splice(start, end, a->begin());
  }
  
  template <typename T>
  inline ref<Array<T>>
  Array<T>::splice(uint32 start, uint32 end, Iterator& it) const {
    return (Array<T>*)ArrayImp::splice(
      (ArrayImp::A*)this,
      start + _start,
      end == END ? _end : end + _start,
      (ArrayImp::A::Iterator&)it
    );
  }

  template <typename T>
  inline ref<Array<T>>
  Array<T>::splice(uint32 start, uint32 end, Iterator&& it) const {
    return (Array<T>*)ArrayImp::splice(
      (ArrayImp::A*)this,
      start + _start,
      end == END ? _end : end + _start,
      (ArrayImp::A::Iterator&)it
    );
  }

  template <typename T>
  template <typename It>
  inline ref<Array<T>>
  Array<T>::splice(uint32 start, uint32 end, It&& it, const It& endit) const {
    return (Array<T>*)ArrayImp::splicefn(
      (ArrayImp::A*)this,
      start + _start,
      end == END ? _end : end + _start,
      [it=std::move(it), endit] () mutable -> ArrayImp::A::ValueT* {
        // iterator function wrapping type-sensitive iterator to produce
        // type-insensitive values
        if (it == endit) { return nullptr; }
        auto v = (ArrayImp::A::ValueT*)new ValueT(std::move(*it));
        ++it;
        return v;
      }
    );
  }
  
  template <typename T>
  inline ref<TransientArray<T>> Array<T>::asTransient() const {
    return (TransientArrayT*)ArrayImp::createTransient((ArrayImp::A*)this);
  }
  
  
  template <typename T>
  template <typename F>
  inline ref<Array<T>> Array<T>::modify(F&& fn) const {
    auto t = asTransient();
    fn(t);
    return t->makePersistent();
  }
  
  template <typename T>
  inline bool Array<T>::operator==(const ref<Array>& rhs) const {
    return this == rhs.ptr();
    // Note: (_root == rhs->_root && _start == rhs->_start) is not needed as we never
    // create copies of an array unless the size changes, in which case the arrays
    // are different.
  }
  
  template <typename T>
  inline int Array<T>::compare(const ref<Array>& other) const {
    if (operator==(other)) {
      // same underlying data
      return 0;
    }
    if (size() < other->size()) {
      return -1;
    }
    if (size() > other->size()) {
      return 1;
    }
    auto E = end();
    auto ai = begin();
    auto bi = other->begin();
    while (ai != E) {
      // Note: Value implements comparison operators for T
      auto a = ai.value();
      auto b = bi.value();
      if (a != b) { // not same values
        if (*a < *b) {
          return -1;
        }
        if (*a > *b) {
          return 1;
        }
      }
      ++ai;
      ++bi;
    }
    return 0;
  }
  
  // —————————————————————————————————————————————————————————————————————
  // Array::Iterator
  
  template <typename T>
  inline typename Array<T>::Iterator Array<T>::begin(uint32 start, uint32 end) const {
    // Note: Iterator constructor handles the case when end==END
    return Iterator(
      this,
      _start + start,
      end == END ? _end : min(_start + end, _end)
    );
  }
  
  template <typename T>
  inline const typename Array<T>::Iterator& Array<T>::end() const {
    return (Array<T>::Iterator&)ArrayImp::END_ITERATOR;
  }
  
  template <typename T>
  inline Array<T>::Iterator::Iterator(const Array* a, uint32 start, uint32 end)
    // Note: start and end are absolute
    : _a(const_cast<Array*>(a))
    , _i(start)
    , _end(end)
    , _base(_i - (_i % ArrayImp::BRANCHES))
  {
    if (_i < _end) {
      _slots = ArrayImp::slotsFor((ArrayImp::A*)a, _i, _slotlen);
    } else {
      _slots = nullptr;
    }
  }
  
  template <typename T>
  inline Value<T>* Array<T>::Iterator::value() {
    Object* obj = _slots[_i & ArrayImp::MASK];
    ImmutableAssertTypeTag(obj, ValueT::TYPE_TAG);
    return static_cast<ValueT*>(obj);
  }

  template <typename T>
  inline const Value<T>* Array<T>::Iterator::value() const {
    Object* obj = _slots[_i & ArrayImp::MASK];
    ImmutableAssertTypeTag(obj, ValueT::TYPE_TAG);
    return static_cast<const ValueT*>(obj);
  }

  template <typename T>
  inline bool Array<T>::Iterator::valid() const {
    return _slots && _i < _end;
  }
  
  template <typename T>
  inline T& Array<T>::Iterator::operator*() {
    return value()->value;
  }
  
  template <typename T>
  inline typename Array<T>::Iterator& Array<T>::Iterator::operator++() { // ++i
    ++_i;
    if (_i < _end) {
      if (_i - _base == ArrayImp::BRANCHES) {
        _slots = ArrayImp::slotsFor((ArrayImp::A*)_a.ptr(), _i, _slotlen);
        _base += ArrayImp::BRANCHES;
      }
    } else {
      // reached end
      _slots = nullptr;
    }
    return *this;
  }
  
  template <typename T>
  inline typename Array<T>::Iterator Array<T>::Iterator::operator++(int) { // i++
    Iterator copy(*this);
    operator++();
    return copy;
  }
  
  template <typename T>
  inline bool Array<T>::Iterator::operator==(const Array<T>::Iterator& rhs) const {
    if (_slots != rhs._slots) { return false; }
    if (!_slots) { // both slots are null
      // == end
      return true;
    }
    return _i == rhs._i;
  }
  
  
  template <typename T>
  inline typename Array<T>::Iterator::difference_type
  Array<T>::Iterator::distanceTo(const Array<T>::Iterator& other) const {
    if (!_slots) { // E - I
      return other._end - other._i;
    }
    if (!other._slots) { // I - E
      return _end - _i;
    }
    return (_i > other._i) ? _i - other._i : other._i - _i;
  }


} // namespace
