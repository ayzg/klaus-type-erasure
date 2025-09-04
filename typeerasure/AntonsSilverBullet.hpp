// A few years ago I watched  Klaus Iglberger's CppCon presentation "There is no
// silver bullet"
//

#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <typeindex>
#include <vector>

template <typename T>
constexpr T absolute(T value) {
  return (value < 0) ? -value : value;
}

// Type Erasure Sample Code.
//
// Implementation of Klaus Iglberger's C++ Type Erasure Design Pattern.
//
// References:
// - Breaking Dependencies: Type Erasure - A Design Analysis,
//   by Klaus Iglberger, CppCon 2021.
//   - Video: https://www.youtube.com/watch?v=4eeESJQk-mw
//   - Slides:
//   https://meetingcpp.com/mcpp/slides/2021/Type%20Erasure%20-%20A%20Design%20Analysis9268.pdf

// High Level Summary of the Design
// - `class Shape` and global functions (`serialize()`, `draw()`, etc.)
//   - The external client facing interface.
//   - Holds a pointer to `Interface` internally.
// - `class Interface`
//   - The internal interface of the Bridge Design Pattern.
//   - It is needed to hide the template parameter of `Model<T>`.
// - `class Model<T>`
//   - The templated implementation of `Interface`.
//   - Routes virtual functions to global functions.

// CAUTION: The following deleted functions serve 2 purposes:
// 1. Prevent the compiler from complaining about missing global functions
//    `serialize()` and `draw()` when seeing the using declarations in
//    `Model::serialize()` and `Model::draw()`, as if the compiler
//    did not see the `friend` definitions within `class Shape`.
// 2. Prevent runaway recursion in case a concrete `Shape` such as `Circle`
//    does not define a `serialize(const Circle&)` or `draw(const Circle&)`
//    function.
template <typename T>
constexpr std::string Format(const T& shape) = delete;

template <typename T>
constexpr int Calculate(const T& shape) = delete;

// #ifdef __clang__

// CAUTION: Workaround for clang and msvc.
// The following forward declarations of explicit specialization of
// `serialize()` and `draw()` prevent Clang from complaining about redefintion
// errors.
class Shape;
class ShapeView;

template <>
constexpr std::string Format(const Shape& shape);

template <>
constexpr int Calculate(const Shape& shape);


template <>
constexpr std::string Format(const ShapeView& shape);

template <>
constexpr int Calculate(const ShapeView& shape);

// #endif  // __clang__

// Concepts to allow sfiae in static asserts
// We should declare one for each of the methods of the erased type.
template <typename T>
concept ShapeHasMemberFormat = requires(T t) {
  { t.Format() } -> std::same_as<std::string>;
};

template <typename T>
concept ShapeHasStaticFormat = requires(T t) {
  { Format(t) } -> std::same_as<std::string>;
};

template <typename T>
concept ShapeHasMemberCalculate = requires(T t) {
  { t.Calculate() } -> std::same_as<int>;
};

template <typename T>
concept ShapeHasStaticCalculate = requires(T t) {
  { Calculate(t) } -> std::same_as<int>;
};

template <class T>
struct IndirectShape;

template <class T>
struct ShapeBaseCRTP {
  friend IndirectShape<T>;
  friend ShapeBaseCRTP<IndirectShape<T>>;
  friend Shape;
  int sizex{0};
  int sizey{0};

 public:
  std::string ShapeBaseCRTP_Format() const {
    return std::format("[X:{}|Y:{}]\n", sizex, sizey);
  }

 protected:
  template <class SelfT>
  std::string Format(this const SelfT& self) {
    if constexpr (ShapeHasMemberFormat<T>) {
      return self.ShapeBaseCRTP_Format() + static_cast<const T&>(self).Format();
    } else if constexpr (ShapeHasStaticFormat<T>) {
      using ::Format;
      return self.ShapeBaseCRTP_Format() + Format(static_cast<const T&>(self));
    } else {
      return self.ShapeBaseCRTP_Format();
    }
  };

  template <class SelfT>
  constexpr int Calculate(this const SelfT& self) {
    if constexpr (ShapeHasMemberCalculate<T>) {
      return static_cast<const T&>(self).Calculate();
    } else if constexpr (ShapeHasStaticCalculate<T>) {
      using ::Format;
      return Calculate(static_cast<const T&>(self));
    } else {
      return 0;
    }
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ShapeBaseCRTP& shape) {
    return os << shape.Format();
  }

  friend std::ostream& operator<<(std::ostream& os, ShapeBaseCRTP&& shape) {
    return os << shape.Format();
  }
};

template <class T>
struct IndirectShape : public T, public ShapeBaseCRTP<IndirectShape<T>> {
  template <class SelfT>
  std::string Format(this const SelfT& self) {
    if constexpr (ShapeHasMemberFormat<T>) {
      return static_cast<const T&>(self).Format();
    } else if constexpr (ShapeHasStaticFormat<T>) {
      using ::Format;
      return Format(static_cast<const T&>(self));
    } else {
      return static_cast<const ShapeBaseCRTP<T>&>(self).Format();
    }
  };

  template <class SelfT>
  constexpr int Calculate(this const SelfT& self) {
    if constexpr (ShapeHasMemberCalculate<T>) {
      return static_cast<const T&>(self).Calculate();
    } else if constexpr (ShapeHasStaticCalculate<T>) {
      using ::Calculate;
      return Calculate(static_cast<const T&>(self));
    } else {
      return static_cast<const ShapeBaseCRTP<T>&>(self).Calculate();
    }
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const IndirectShape& shape) {
    return os << shape.Format();
  }

  friend std::ostream& operator<<(std::ostream& os, IndirectShape&& shape) {
    return os << shape.Format();
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Main Type Erased Shape Class */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Shape {
  // NOTE: Definition of the explicit specialization has to appear separately
  // later outside of class `Shape`, otherwise it results in error such as:
  //
  // ```
  // error: defining explicit specialization 'serialize<Shape>' in friend
  // declaration
  // ```
  //
  // Reference: https://en.cppreference.com/w/cpp/language/friend
  friend constexpr std::string Format<>(const Shape& shape);

  friend constexpr int Calculate<>(const Shape& shape);

  friend std::ostream& operator<<(std::ostream& os, const Shape& shape) {
    return os << *shape.pimpl_;
  }

  // The External Polymorphism Design Pattern
  class Interface {
   public:
    constexpr virtual ~Interface() {}
    virtual void print(std::ostream& os) const = 0;
    constexpr virtual std::string Format() const = 0;
    constexpr virtual int Calculate() const = 0;

    // The Prototype Design Pattern
    constexpr virtual std::unique_ptr<Interface> clone() const = 0;
    constexpr virtual std::type_index typeidx() const = 0;
    friend std::ostream& operator<<(std::ostream& os, const Interface& shape) {
      shape.print(os);
      return os;
    }
  };

  template <class T>
  class Model : public Interface {
    friend Shape;
    T object_;

   public:
    constexpr Model(const T& value) : object_{value} {}

    constexpr std::string Format() const override {
      if constexpr (std::is_base_of_v<ShapeBaseCRTP<T>, T>) {
        return static_cast<const ShapeBaseCRTP<T>&>(object_).Format();
      } else if constexpr (ShapeHasMemberFormat<T>) {
        return object_.Format();
      } else if constexpr (ShapeHasStaticFormat<T>) {
        using ::Format;
        return Format(object_);
      } else {
        static_assert(ShapeHasMemberFormat<T> || ShapeHasStaticFormat<T>,
                      "No 'Format' method found for type 'T'.");
      }
    }

    constexpr int Calculate() const override {
      if constexpr (std::is_base_of_v<ShapeBaseCRTP<T>, T>) {
        return static_cast<const ShapeBaseCRTP<T>&>(object_).Calculate();
      } else if constexpr (ShapeHasMemberCalculate<T>) {
        return object_.Calculate();
      } else if constexpr (ShapeHasStaticCalculate<T>) {
        using ::Calculate;
        return Calculate(object_);
      } else {
        static_assert(ShapeHasMemberCalculate<T> || ShapeHasStaticCalculate<T>,
                      "No 'Calculate' method found for type 'T'.");
      }
    }

    void print(std::ostream& os) const override { os << object_; }

    // The Prototype Design Pattern
    constexpr std::unique_ptr<Interface> clone() const override {
      return std::make_unique<Model>(*this);
    }

    constexpr std::type_index typeidx() const override { return typeid(T); }
  };

  // The Bridge Design Pattern
  std::unique_ptr<Interface> pimpl_;

 public:
  // A constructor template to create a bridge.
  template <class T>
  constexpr Shape(const T& x) : pimpl_{new Model<T>(x)} {}

  constexpr Shape(const Shape& s) : pimpl_{s.pimpl_->clone()} {}

  constexpr Shape(Shape&& s) noexcept : pimpl_{std::move(s.pimpl_)} {}

  constexpr Shape& operator=(const Shape& s) {
    pimpl_ = s.pimpl_->clone();
    return *this;
  }

  constexpr Shape& operator=(Shape&& s) noexcept {
    pimpl_ = std::move(s.pimpl_);
    return *this;
  }

  constexpr std::type_index typeidx() const { return pimpl_->typeidx(); }

  template <class T>
  constexpr T& as() {
    return static_cast<Model<T>&>(*pimpl_).object_;
  }

  template <class T>
  constexpr const T& as() const {
    return static_cast<const Model<T>&>(*pimpl_).object_;
  }

  template <class T>
  constexpr bool is() const {
    return pimpl_->typeidx() == typeid(T);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Type Erased Shape Pointer Class */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ShapeView {
  // NOTE: Definition of the explicit specialization has to appear separately
  // later outside of class `Shape`, otherwise it results in error such as:
  //
  // ```
  // error: defining explicit specialization 'serialize<Shape>' in friend
  // declaration
  // ```
  //
  // Reference: https://en.cppreference.com/w/cpp/language/friend
  friend constexpr std::string Format<>(const ShapeView& shape);

  friend constexpr int Calculate<>(const ShapeView& shape);

  friend std::ostream& operator<<(std::ostream& os, const ShapeView& shape) {
    return os << *shape.pimpl_;
  }

  // The External Polymorphism Design Pattern
  class Interface {
   public:
    constexpr virtual ~Interface() {}
    virtual void print(std::ostream& os) const = 0;
    constexpr virtual std::string Format() const = 0;
    constexpr virtual int Calculate() const = 0;

    // The Prototype Design Pattern
    constexpr virtual std::unique_ptr<Interface> clone() const = 0;
    constexpr virtual std::type_index typeidx() const = 0;
    constexpr virtual bool is(std::type_index idx) const = 0;
    friend std::ostream& operator<<(std::ostream& os, const Interface& shape) {
      shape.print(os);
      return os;
    }
  };

  template <class T>
  class Model : public Interface {
    friend Shape;
    T* object_;

   public:
    constexpr Model(T* value) : object_{value} {}

    constexpr std::string Format() const override {
      if constexpr (std::is_base_of_v<ShapeBaseCRTP<T>, T>) {
        return static_cast<const ShapeBaseCRTP<T>&>(*object_).Format();
      } else if constexpr (ShapeHasMemberFormat<T>) {
        return object_->Format();
      } else if constexpr (ShapeHasStaticFormat<T>) {
        using ::Format;
        return Format(*object_);
      } else {
        static_assert(ShapeHasMemberFormat<T> || ShapeHasStaticFormat<T>,
                      "No 'Format' method found for type 'T'.");
      }
    }

    constexpr int Calculate() const override {
      if constexpr (std::is_base_of_v<ShapeBaseCRTP<T>, T>) {
        return static_cast<const ShapeBaseCRTP<T>&>(*object_).Calculate();
      } else if constexpr (ShapeHasMemberCalculate<T>) {
        return object_->Calculate();
      } else if constexpr (ShapeHasStaticCalculate<T>) {
        using ::Calculate;
        return Calculate(*object_);
      } else {
        static_assert(ShapeHasMemberCalculate<T> || ShapeHasStaticCalculate<T>,
                      "No 'Calculate' method found for type 'T'.");
      }
    }

    void print(std::ostream& os) const override { os << *object_; }

    // The Prototype Design Pattern
    constexpr std::unique_ptr<Interface> clone() const override {
      return std::make_unique<Model>(*this);
    }

    constexpr std::type_index typeidx() const override { return typeid(T); }

    constexpr virtual bool is(std::type_index idx) const {
      return typeid(T) == idx;
    }
  };

  // The Bridge Design Pattern
  std::unique_ptr<Interface> pimpl_;

 public:
  // A constructor template to create a bridge.
  template <class T>
  constexpr ShapeView(T* x) : pimpl_{new Model<T>(x)} {}

  constexpr ShapeView(const ShapeView& s) : pimpl_{s.pimpl_->clone()} {}

  constexpr ShapeView(ShapeView&& s) noexcept : pimpl_{std::move(s.pimpl_)} {}

  constexpr ShapeView& operator=(const ShapeView& s) {
    pimpl_ = s.pimpl_->clone();
    return *this;
  }

  constexpr ShapeView& operator=(ShapeView&& s) noexcept {
    pimpl_ = std::move(s.pimpl_);
    return *this;
  }

  constexpr std::type_index typeidx() const { return pimpl_->typeidx(); }

  template <class T>
  constexpr T& as() {
    return static_cast<Model<T>&>(*pimpl_).object_;
  }

  template <class T>
  constexpr const T& as() const {
    return static_cast<const Model<T>&>(*pimpl_).object_;
  }

  template <class T>
  constexpr bool is() const {
    return pimpl_->is(typeid(T));
  }
};


template <>
constexpr std::string Format(const Shape& shape) {
  return shape.pimpl_->Format();
}

template <>
constexpr int Calculate(const Shape& shape) {
  return shape.pimpl_->Calculate();
}

template <>
constexpr std::string Format(const ShapeView& shape) {
  return shape.pimpl_->Format();
}

template <>
constexpr int Calculate(const ShapeView& shape) {
  return shape.pimpl_->Calculate();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* User Code */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Circle {
  double radius_;

 public:
  constexpr explicit Circle(double radius) : radius_(radius) {}

  constexpr double radius() const { return radius_; }
};

std::ostream& operator<<(std::ostream& os, const Circle& circle) {
  return os << "Circle(radius = " << circle.radius() << ")";
}

constexpr std::string Format(const Circle& circle) {
  // - Iterate through the grid from -radius to +radius for both x and y
  // - Check if the point (x, y) is close enough to the circle's equation
  // (scaled for width)
  // - Append star for a circle point
  // - Otherwise, append space
  // - Add a newline after each row
  std::string out;
  for (int y = -circle.radius(); y <= circle.radius(); ++y) {
    for (int x = -2 * circle.radius(); x <= 2 * circle.radius(); ++x) {
      if (absolute(x * x / 4 + y * y - circle.radius() * circle.radius()) <=
          circle.radius())
        out += "*";
      else
        out += " ";
    }
    out += "\n";
  }

  return out;
}

constexpr int Calculate(const Circle& circle) { return 42; }

class Square {
  double width_;

 public:
  constexpr explicit Square(double width) : width_(width) {}

  constexpr double width() const { return width_; }

  // This Format method is implemented as a member function and still works for
  // a Shape.
  constexpr std::string Format() const {
    const char TOP_LEFT = 0xC9;      // ASCII: 201
    const char TOP_RIGHT = 0xBB;     // ASCII: 187
    const char BOTTOM_LEFT = 0xC8;   // ASCII: 200
    const char BOTTOM_RIGHT = 0xBC;  // ASCII: 188
    const char HORIZONTAL = 0xCD;    // ASCII: 205
    const char VERTICAL = 0xB3;      // ASCII: 186
    std::string box;
    box += TOP_LEFT;
    for (int i = 0; i < width() - 2; ++i) {
      box += HORIZONTAL;
    }
    box += TOP_RIGHT;
    box += "\n";

    for (int i = 0; i < width() - 2; ++i) {
      box += VERTICAL;
      for (int j = 0; j < width() - 2; ++j) {
        box += " ";
      }
      box += VERTICAL;
      box += "\n";
    }

    box += BOTTOM_LEFT;
    for (int i = 0; i < width() - 2; ++i) {
      box += HORIZONTAL;
    }
    box += BOTTOM_RIGHT;
    box += "\n";

    return box;
  }
};

std::ostream& operator<<(std::ostream& os, const Square& square) {
  return os << "Square(width = " << square.width() << ")";
}

constexpr int Calculate(const Square& circle) { return 42; }

class Triangle {
  double size;

 public:
  constexpr explicit Triangle(double size) : size(size) {}

  constexpr double radius() const { return size; }

  friend std::ostream& operator<<(std::ostream& os, const Triangle& circle) {
    return os << "Triangle(size = " << circle.radius() << ")";
  }

  constexpr std::string Format() const {
    std::string ret{""};
    for (int i = 0; i < size; ++i) {
      ret += std::string(size - i - 1, ' ');
      ret += std::string(2 * i + 1, '*');
      ret += '\n';
    }
    return ret;
  }

  constexpr int Calculate() const { return 42; }
};

// 1. Making it constexpr.
// - Interface may have constexpr virtual functions from C++20
// - std::unique_ptr is constexpr from C++23
// Ofcouse we have to use some trickery to actually prove that the
// class is constexpr viable. To be more specific, you may instantiate
// this class in a constexpr context but you may not return it outside
// of said context. This will result in an invalid use of interpreted memory.
// Generating strings is complicated at compile time, is a topic for another
// blog post.
static constexpr int CxCalculateShape = []() constexpr {
  Shape cx_shape{Circle{5.0}};
  return Calculate(cx_shape);
}();

// 2. Allowing the use to optionally provide a structurally conforming type
// instead of forcing them to write a static interface. This is optional, as it
// changes the semantics of the Shape class, perhaps you do not wish to ever use
// the member function - even if it is available.
//
// Inside the Shape::Model : We use constexpr if with requires clauses (or
// optionally make concepts) to check if the provided type has a member function
// which conforms to the specifcic name, arguments and return type we require.
// If so we can assume this is the method we should call instead of the static
// global interface.
// Of course it would be nice to provide a static assert when the required
// method is not available. If we try to do it like so, we will get a compile
// error:
/*
    constexpr std::string Format() const override {
      if constexpr (requires(T t) {
                      { t.Format() } -> std::same_as<std::string>;
                    }) {
        return object_.Format();
      } else if constexpr (requires(T t) {
                             { Format(t) } -> std::same_as<std::string>;
                           }) {
        using ::Format;
        return Format(object_);
      } else {
        static_assert(false,
                      "T must be a Shape or a Shape Structure.");
      }

    }
*/
// We must add a layer of indirection to the static assert, that is user SFINAE.
// One way would be to create a concept for having a member or static named
// method.
/*
      if constexpr (ShapeHasMemberFormat<T>) {
        return object_.Format();
      } else if constexpr (ShapeHasStaticFormat<T>) {
        using ::Format;
        return Format(object_);
      } else {
        static_assert(ShapeHasMemberFormat<T> || ShapeHasStaticFormat<T>,
                      "T must be a Shape or a Shape Structure.");
      }

*/

// 3. Klaus erased his types. Can we help him find them ?
// This could be by desgin that we cannot, but it is possible to retrieve the
// type of the erased object again and get a reference to the contained T.
// We can retrive (at aleast try) any type from the Shape by adding a method
// such as this:
/* Inside Shape class:
  template <class T>
  constexpr T& as() {
    return static_cast<ShapeModel<T>&>(*pimpl_).object_;
  }
*/
// To allow access to the .object_ member of any ShapeModel we have to add
// Shape as a friend inside ShapeModel. Lets say we added a square shapre to a
// vector we can now retrieve a reference to it like so:
/*
  shapes.emplace_back(Square{10.0});
  Square& square = shapes.back().as<Square>();
*/
// This is nice and all but what if I forgot where I put my square before
// getting a reference to it? We can also provide a method which checks if a T
// is currently contained in a Shape. To allow this we should add a new virtual
// method typeidx to the Interface, as we must be able check the type from the
// non-templated virtual pointer. Inside Model we have access to the concerete
// T, we can implement the override method inside Model<T> like so:
/*
constexpr std::type_index typeidx() const override {
      return typeid(T);
}

Then inside the Shape class:
  template <class T>
  constexpr bool is() {
    return pimpl_->typeidx() == typeid(T);
  }

Then use it like so:

  std::vector<Shape> shapes;
  shapes.emplace_back(Circle{5.0});
  shapes.emplace_back(Square{10.0});
  shapes.emplace_back(Circle{5.0});

  // Oh my! i lost my square...
  Square* my_square;
  auto square_loc = std::find_if(shapes.begin(), shapes.end(),
               [](const Shape& shape) { return shape.is<Square>(); });
  if (square_loc != shapes.end()) {
    // I found it!
    my_square = &shapes.back().as<Square>();
  }
  assert(square_loc != shapes.end() && "I lost my square!");

*/

// 4. Providing the user with a default base implementation, while also allowing
// partial overrides if desired. A mundane impl may look like so:
struct ShapeBase {
 public:
  friend std::ostream& operator<<(std::ostream& os, const ShapeBase& circle) {
    return os << "Base";
  }

  // constexpr std::string Format() const { return "[Base]"; }

  constexpr int Calculate() const { return 0; }
};

struct Pyramid : public ShapeBase {
 public:
  constexpr std::string Format() const {
    return
        R"(
               '
              /=\\
             /===\ \
            /=====\' \
           /=======\'' \
          /=========\ ' '\
         /===========\''   \
        /=============\ ' '  \
       /===============\   ''  \
      /=================\' ' ' ' \
     /===================\' ' '  ' \
    /=====================\' '   ' ' \
   /=======================\  '   ' /
  /=========================\   ' /
 /===========================\'  /
/=============================\/
)";
    ;
  }
};

// But what if the user wants to use a static Format function instead of a
// member function. We should afford them that flexibility. Using the current
// shape base the bat will not be drawn.
struct Bat : public ShapeBase {};

constexpr std::string Format(const Bat& bat) {
  return
      R"(
                 _..__.          .__.._
               .^"-.._ '-(\__/)-' _..-"^.
                      '-.' oo '.-'
                         `-..-'  fsc 
)";
}

// We can implement a more flexible base class using CRTP (Curiously Recurring
// Template Parameter) pattern.

// Sadly while this compiles, creating the base shape is not enough.
// The Shape class must know about the Base class. And when any user class is a
// base of the Base then we know to call the base's Format method instead of the
// user class's Format method. Currently if we try to run , we will get a stack
// overflow:
/*
Unhandled exception at 0x00007FF670844F29 in typeerasure.exe: 0xC00000FD: Stack
overflow (parameters: 0x0000000000000001, 0x00000090B6AB3F48).
*/
// This is because when Shape calls shape.Format() it will call our Base, inside
// our Base we call the member function .Format() which see's that the derived
// type has a .Format() and then dispatches the call back to Shape which calls
// shape.Format() again causing the recusrsion.

// The key to overcoming this isa clever managment of private members. That is,
// the base implementation should be private and not pollute the user's class
// with public members. Instead we have the SHape detect if any Model<T> is
// derived from Base, if so we call the base's CRTP method's instead - which
// will then be able to make the correct decision to use a member static of
// default impl. We must extened every method in the Shape::Model to account for
// a potential library provided base implementation.

// We should now see the base's format describing the location. In addition to
// the husky. When calling Format from Shape.
struct Husky : ShapeBaseCRTP<Husky> {};

constexpr std::string Format(const Husky& bat) {
  return
      R"(
                                ;\ 
                            |' \ 
         _                  ; : ;
         / `-.              /: : |
        |  ,-.`-.          ,': : |
        \  :  `. `.       ,'-. : |
         \ ;    ;  `-.__,'    `-.|
          \ ;   ;  :::  ,::'`:.  `.
           \   `-. :  `    :.    `.  \ 
           \   \    ,   ;   ,:    (\ 
            \   :., :.    ,'o)): ` `-.
            ,/,' ;' ,::"'`.`---'   `.  `-._
          ,/  :  ; '"      `;'          ,--`.
         ;/   :; ;             ,:'     (   ,:)
           ,.,:.    ; ,:.,  ,-._ `.     \""'/
           '::'     `:'`  ,'(  \`._____.-'"'
              ;,   ;  `.  `. `._`-.  \\ 
             ;:.  ;:       `-._`-.\  \`.
                '`:. :        |' `. `\  ) \ 
      -hrr-      ` ;:       |    `--\__,'
                    '`      ,'
                         ,-'
)";
}

// Last layer of indirection. We can add one more layer of indirection to
// completley remove all responsibility from the to user to inherit our
// ShapeBase.
/*
template<class T>
struct IndirectShape : T, ShapeBaseCRTP<IndirectShape<T>>
*/
// At this point its an implementation CHOICE wether we want all shapes to
// actually be stored as an IndirectShape<T> directly inside the Shape::Model.
// That is, any class we provide to shape will then be wrapped inside an
// IndirectShape<T> which is derived from ShapeBaseCRTP<IndirectShape<T>> and T
// by template deduction.
// The user can simply use an interface such as:
/*
  IndirectShape<Circle> indirect_circle{Circle{5.0}};
  shapes.push_back(indirect_circle);
*/
// The indirect circle gains all functionality of ShapeBaseCRTP while not
// directly deriving from it.
// Wether the user provided functionality is appended or override the
// underlying Base functionality
// is based on how the base is implemnted. Format shows an append operation,
// Calculate shows how you would overrided.
// I will copy the entire impl to a new file and implement a mandatory base
// style class.

// What if we dont want to own the shape? Is it necessary to actually copy the
// object. What if i simply want to store a pointer to any Shape - compatible
// class ?

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Application */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int AntonsSilverBullet() {
  Shape circle{Circle{5.0}};
  std::vector<Shape> shapes;
  shapes.emplace_back(Circle{5.0});
  shapes.emplace_back(Square{10.0});
  shapes.emplace_back(Triangle{10.0});
  shapes.emplace_back(Pyramid{});
  shapes.emplace_back(Bat{});
  shapes.emplace_back(Husky{});
  shapes.emplace_back(IndirectShape<Bat>{});

  // Oh my! i lost my square...
  Square* my_square;
  auto square_loc =
      std::find_if(shapes.begin(), shapes.end(),
                   [](const Shape& shape) { return shape.is<Square>(); });
  if (square_loc != shapes.end()) {
    // I found it!
    my_square = &shapes.back().as<Square>();
  }
  assert(square_loc != shapes.end() && "I lost my square!");
  assert(std::distance(shapes.begin(), square_loc) == 1 && "I lost my square!");

  IndirectShape<Circle> indirect_circle{Circle{5.0}};
  shapes.push_back(indirect_circle);

  for (const auto& shape : shapes) {
    std::cout << "Drawing: " << shape.typeidx().name() << std::endl;
    std::cout << Format(shape) << std::endl;
  }

  
  std::cout << "******* Drawing All Animals *******" << std::endl;
  std::vector<ShapeView> animal_views{};
  for (auto& shape : shapes) {
    // Get a view of my animals...
    if (shape.is<IndirectShape<Bat>>() || shape.is<Husky>() || shape.is<Bat>()) 
      animal_views.push_back(&shape);
    
  }
  // Draw my animals...
  for (auto& animal : animal_views) {
    std::cout << Format(animal) << std::endl;
  }

  return 0;
}
