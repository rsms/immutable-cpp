#pragma once
#include <assert.h>
#include <stdint.h>
#include <functional>

namespace immutable {


#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

#if __has_attribute(unused)
  #define IMMUTABLE_UNUSED __attribute__((unused))
#else
  #define IMMUTABLE_UNUSED
#endif

#if __has_attribute(always_inline)
  #define IMMUTABLE_ALWAYS_INLINE __attribute__((always_inline))
#else
  #define IMMUTABLE_ALWAYS_INLINE inline
#endif
  
// target architecture
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
  #define IMMUTABLE_TARGET_ARCH_X86 1
#elif defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(_M_AMD64)
  #define IMMUTABLE_TARGET_ARCH_X64 1
#elif defined(__arm__) || defined(_M_ARM) || defined(__ARM__)
  #if defined(__arm64__) || defined(__aarch64__)
    #define IMMUTABLE_TARGET_ARCH_ARM64 1
  #else
    #define IMMUTABLE_TARGET_ARCH_ARM32 1
  #endif
#else
  #error "Unsupported target architecture"
#endif

// primitive types
using intz   = intptr_t;
using uintz  = uintptr_t;
using int32  = int32_t;
using uint32 = uint32_t;
using int8   = int8_t;
using uint8  = uint8_t;

// does the compiler and libc provide C11 stdatomic.h?
#if !defined(__STDC_NO_ATOMICS__) && \
    ( (defined(__has_feature) && __has_feature(c_atomic)) || \
    (defined(__has_extension) && __has_extension(c_atomic)) )
  // Declaring something atomic_t(T) generally means:
  // - Load and store instructions are not reordered (similar to "volatile").
  // - Instead of a MOV, an XCHG instruction is used for stores.
  // It does _not_ imply thread memory synchronization.
  // For thread synchronization, you need to use the Atomic* functions below.
  #define IMMUTABLE_HAS_STDATOMIC
  // Note: std::atomic goes bananas if we include stdatomic.h
  // instead, we define the parts we need ourselves.
  // TODO: clang-specific for now; test and update for other compilers.

  typedef enum memory_order {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
  } memory_order;

  using atomici32 = _Atomic(int_least32_t);
  using atomicu32 = _Atomic(uint_least32_t);
  
  #define atomic_load_explicit __c11_atomic_load
  #define atomic_fetch_sub_explicit __c11_atomic_fetch_sub
#else
  // does the compiler provide __sync intrinsics?
  #if !defined(__clang__) && (!defined(__GNUC__) || (__GNUC__ < 4))
    // No stdatomic and no __sync_* built-ins -- we are out of luck
    #error "Unsupported target architecture: Missing atomic-operation intrinsics"
  #endif

  typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
  } memory_order;

  using atomici32 = int32;
  using atomicu32 = uint32;
#endif


// special-case function that increments an integer for i.e. refcounting
inline static void IMMUTABLE_UNUSED
atomic_incr32(volatile atomicu32* ptr, uint32 delta) {
#if IMMUTABLE_TARGET_ARCH_X64 || \
     (IMMUTABLE_TARGET_ARCH_X86 && \
      (defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)) \
     )
  // all x86 archs but i386
  asm __volatile__ (
    "lock xaddl %1, %0\n" // add delta to operand
    : // no output
    : "m" (*ptr), "r" (delta)
  );
#elif defined(IMMUTABLE_HAS_STDATOMIC)
  atomic_fetch_add_explicit(ptr, delta, memory_order_acq_rel);
#else
  __sync_add_and_fetch(ptr, delta);
#endif
}

inline static uint32 IMMUTABLE_UNUSED
atomic_fetch_decr32(volatile atomicu32* ptr, uint32 delta) {
#ifdef IMMUTABLE_HAS_STDATOMIC
  #if IMMUTABLE_TARGET_ARCH_X64 || IMMUTABLE_TARGET_ARCH_X86
    // loads and stores are always ordered on IA-32, IA-64 and AMD64
    return atomic_fetch_sub_explicit(ptr, delta, memory_order_relaxed);
  #else
    return atomic_fetch_sub_explicit(ptr, delta, memory_order_acquire);
  #endif
#else
  return __sync_fetch_and_sub(ptr, delta);
#endif
}


//inline static void IMMUTABLE_UNUSED
//mem_fence() {
//#ifdef IMMUTABLE_HAS_STDATOMIC
//  #if IMMUTABLE_TARGET_ARCH_X64 || IMMUTABLE_TARGET_ARCH_X86
//  // loads and stores are always ordered on IA-32, IA-64 and AMD64
//    atomic_thread_fence(memory_order_relaxed);
//  #else
//    atomic_thread_fence(memory_order_acquire);
//  #endif
//#else
////#elif IMMUTABLE_TARGET_ARCH_X64 || IMMUTABLE_TARGET_ARCH_X86
////  asm __volatile__ (
////    "" // loads and stores are ordered on IA-32, IA-64 and AMD64
////    : : : "memory"
////  );
////#else
//  __sync_synchronize();
//#endif
//}


inline static uint32 atomic_acq_load32(volatile atomicu32* ptr) {
#ifdef IMMUTABLE_HAS_STDATOMIC
  #if IMMUTABLE_TARGET_ARCH_X64 || IMMUTABLE_TARGET_ARCH_X86
    // loads and stores are always ordered on IA-32, IA-64 and AMD64
    return atomic_load_explicit(ptr, memory_order_relaxed);
  #else
    return atomic_load_explicit(ptr, memory_order_acquire);
  #endif
#else
  uint32 v = *ptr;
  __sync_synchronize();
  return v;
#endif
}

// ————————————————————————————————————————————————————————————————————————————————————
// language utils

template <class T> struct rmref      { typedef T type; };
template <class T> struct rmref<T&>  { typedef T type; };
template <class T> struct rmref<T&&> { typedef T type; };

// perfect forwarding
template <class T> inline T&&
fwd(typename rmref<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}

// construct something
template <typename T, typename... Args> inline void
construct(T* p, Args&&... args) {
  ::new((void*)p)T(fwd<Args>(args)...);
}

template <typename T> inline T
min(const T& a, const T& b) {
  return a < b ? a : b;
}


// ————————————————————————————————————————————————————————————————————————————————————
// Reference counting

// Interface defining the reference count implementation methods
struct RefCounted {
  virtual void retain() const =0;
  virtual bool release() const =0; // true if count is zero and obj was deleted
  virtual bool hasSingleRef() const =0;
 protected:
  virtual ~RefCounted() {}
};


// Class that implements atomic reference counting.
struct RefCount {
  RefCount(uint32 initialCount) : _refcount(initialCount) {}
  RefCount() : RefCount(0) {}

  void retain() const {
    atomic_incr32(&_refcount, 1);
  }

  bool release() const {
    return atomic_fetch_decr32(&_refcount, 1) == 1;
  }

  bool hasSingleRef() const {
    return atomic_acq_load32(&_refcount) == 1;
  }

 private:
  RefCount(const RefCount&) = delete;
  void operator=(const RefCount&) = delete;

  mutable atomicu32 _refcount;
};
  
  
#define IMMUTABLE_REFCOUNTED_IMPL(T)        \
  public:                                   \
    void retain() const override {          \
      _refcount.retain();                   \
    }                                       \
    bool release() const override {         \
      if (_refcount.release()) {            \
        const_cast<T*>(this)->dealloc();    \
        return true;                        \
      }                                     \
      return false;                         \
    }                                       \
    bool hasSingleRef() const override {    \
      return _refcount.hasSingleRef();      \
    }                                       \
  private:                                  \
    ::immutable::RefCount _refcount;


// Ref manages a reference to a RefCounted object
template <class T>
class ref {
public:
  typedef T element_type;
  
  ref() : _ptr(nullptr) {
  }
  
  ref(T* p) : _ptr(p) {
    if (_ptr) { _ptr->retain(); }
  }
  
  ref(const ref<T>& r) : _ptr(r._ptr) {
    if (_ptr) { _ptr->retain(); }
  }
  
  ref(ref<T>&& r) : _ptr(r._ptr) {
    r._ptr = nullptr;
  }
  
  template <typename U>
  ref(const ref<U>& r) : _ptr(r.ptr()) {
    if (_ptr) { _ptr->retain(); }
  }
  
  ~ref() {
    if (_ptr) { _ptr->release(); }
  }
  
  T* ptr() const { return _ptr; }
  
  // Allow ref<C> to be used in boolean expression
  // and comparison operations.
  operator T*() const { return _ptr; }
  
  T* operator->() const {
    assert(_ptr != nullptr);
    return _ptr;
  }
  
  ref<T>& operator=(T* p) {
    // retain first so that self assignment should work
    if (p) { p->retain(); }
    T* old_ptr = _ptr;
    _ptr = p;
    if (old_ptr) { old_ptr->release(); }
    return *this;
  }
  
  ref<T>& operator=(const ref<T>& r) {
    return *this = r._ptr;
  }

  ref<T>& operator=(ref<T>&& r) {
    if (r._ptr != _ptr) {
      T* old_ptr = _ptr;
      _ptr = r._ptr;
      if (old_ptr) { old_ptr->release(); }
    }
    r._ptr = nullptr;
    return *this;
  }
  
  template <typename U>
  ref<T>& operator=(const ref<U>& r) {
    return *this = r.ptr();
  }
  
  void swap(T** pp) {
    T* p = _ptr;
    _ptr = *pp;
    *pp = p;
  }
  
  void swap(ref<T>& r) {
    swap(&r._ptr);
  }
  
protected:
  T* _ptr;
};


template <typename T>
ref<T> makeref(T* t) {
  return ref<T>(t);
}


using TypeTag = char;

#if (!defined(NDEBUG) || defined(DEBUG)) && !defined(IMMUTABLE_WITH_TYPE_TAGS)
  // Enable type-tags by default for debug builds
  #define IMMUTABLE_WITH_TYPE_TAGS
#endif

#ifdef IMMUTABLE_WITH_TYPE_TAGS
  #define ImmutableTypeTagHead             TypeTag typeTag;
  #define ImmutableAssertTypeTag(ptr, tag) assert((ptr)->typeTag == tag)
#else
  #define ImmutableTypeTagHead
  #define ImmutableAssertTypeTag(ptr, tag) ((void)0)
#endif


// Optionally type-tagged object
struct Object : RefCounted {
  #ifdef IMMUTABLE_WITH_TYPE_TAGS
  ImmutableTypeTagHead
  Object(TypeTag t) : typeTag(t) {}
  #else
  Object(TypeTag) {}
  #endif
};


// Reference-counted container for a value of any type
template <typename T>
struct Value : Object {
  T value; // the actual value is stored here

  // Forwarding constructor
  template <class... Args>
  Value(Args&&... args)
    : Object(TYPE_TAG), value(fwd<Args>(args)...)
  {}

  // allow implicit cast to T
  operator T&() { return value; }
  operator const T&() const { return value; }
  
  // comparison
  bool operator<(const Value& rhs) const { return std::less<T>()(value, rhs.value); }
  bool operator>(const Value& rhs) const { return std::greater<T>()(value, rhs.value); }
  bool operator<=(const Value& rhs) const { return std::greater_equal<T>()(value, rhs.value); }
  bool operator>=(const Value& rhs) const { return std::less_equal<T>()(value, rhs.value); }
  bool operator==(const Value& rhs) const { return std::equal_to<T>()(value, rhs.value); }
  bool operator!=(const Value& rhs) const { return std::not_equal_to<T>()(value, rhs.value); }

  // copyable and movable
  Value(const Value&) = default;
  Value(Value&&) = default;
  Value& operator=(const Value&) = default;
  Value& operator=(Value&&) = default;

  void dealloc() { delete this; }
  static constexpr TypeTag TYPE_TAG = 'V';
  IMMUTABLE_REFCOUNTED_IMPL(Value)
};


} // namespace

