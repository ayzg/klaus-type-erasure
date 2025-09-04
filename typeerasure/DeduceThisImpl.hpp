#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <typeindex>
#include <cassert>
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
//   - Holds a pointer to `ShapeConcept` internally.
// - `class ShapeConcept`
//   - The internal interface of the Bridge Design Pattern.
//   - It is needed to hide the template parameter of `ShapeModel<T>`.
// - `class ShapeModel<T>`
//   - The templated implementation of `ShapeConcept`.
//   - Routes virtual functions to global functions.

// CAUTION: The following deleted functions serve 2 purposes:
// 1. Prevent the compiler from complaining about missing global functions
//    `serialize()` and `draw()` when seeing the using declarations in
//    `ShapeModel::serialize()` and `ShapeModel::draw()`, as if the compiler
//    did not see the `friend` definitions within `class Shape`.
// 2. Prevent runaway recursion in case a concrete `Shape` such as `Circle`
//    does not define a `serialize(const Circle&)` or `draw(const Circle&)`
//    function.
template <typename T>
void serialize(const T&) = delete;

template <typename T>
void draw(const T&) = delete;

template <typename T>
std::string Format(const T& shape) = delete;

// #ifdef __clang__

// CAUTION: Workaround for clang and msvc.
// The following forward declarations of explicit specialization of
// `serialize()` and `draw()` prevent Clang from complaining about redefintion
// errors.
class Shape;
class ShapeView;

template <>
void serialize(const Shape& shape);

template <>
void draw(const Shape& shape);

template <>
std::string Format(const Shape& shape);

template <>
void serialize(const ShapeView& shape);

template <>
void draw(const ShapeView& shape);

template <>
std::string Format(const ShapeView& shape);

// #endif  // __clang__

template <typename T>
concept IsShape = requires(T t) {
  serialize(t);
  draw(t);
  { std::declval<std::ostream&>() << t } -> std::same_as<std::ostream&>;
};

template <typename T>
concept IsShapeStructure = requires(T t) {
  { t.serialize() } -> std::same_as<void>;
  { t.draw() } -> std::same_as<void>;
  { t.Format() } -> std::same_as<std::string>;
  { std::declval<std::ostream&>() << t } -> std::same_as<std::ostream&>;
};

template <class T>
struct ShapeBase {
  friend Shape;

 private:
  void serialize(this T&& self) {
    if constexpr (IsShapeStructure<T>) {
      self.serialize();
    } else if constexpr (IsShape<T>) {
      using ::serialize;
      serialize(self);
    } else {
      std::cout << "BaseObject" << std::endl;
    }
  };

  void draw(this T&& self) {
    if constexpr (IsShapeStructure<T>) {
      self.draw();
    } else if constexpr (IsShape<T>) {
      using ::draw;
      draw(self);
    } else {
      std::cout << "[Drawing Nothing]" << std::endl;
    }
  };

  void print(this T&& self, std::ostream& os) {
    if constexpr (IsShapeStructure<T>) {
      self.draw();
    } else if constexpr (IsShape<T>) {
      using ::print;
      print(self);
    } else {
      std::cout << "[Printing Nothing]" << std::endl;
    }
  };

  template <class SelfT>
  std::string Format(this SelfT&& self) {
    if constexpr (IsShapeStructure<T>) {
      return static_cast<T&>(self).Format();
    } else if constexpr (IsShape<T>) {
      using ::Format;
      return Format(static_cast<T&>(self));
    } else {
      return "BaseObject";
    }
  };

  // std::unique_ptr<Shape::ShapeConcept> clone(this T&& self) {
  //   if constexpr (IsShapeStructure<T>) {
  //     return self.clone();
  //   } else if constexpr (IsShape<T>) {
  //     using ::clone;
  //     return clone(self);
  //   } else {
  //     throw "Can't clone base object";
  //   }
  // };
  template <class SelfT>
  void serialize(this const SelfT& self) {
    if constexpr (IsShapeStructure<T>) {
      static_cast<const T&>(self).serialize();
    } else if constexpr (IsShape<T>) {
      using ::serialize;
      serialize(static_cast<const T&>(self));
    } else {
      std::cout << "BaseObject" << std::endl;
    }
  };
  template <class SelfT>
  void draw(this const SelfT& self) {
    if constexpr (IsShapeStructure<T>) {
      static_cast<const T&>(self).draw();
    } else if constexpr (IsShape<T>) {
      using ::draw;
      draw(static_cast<const T&>(self));
    } else {
      std::cout << "[Drawing Nothing]" << std::endl;
    }
  };

  void print(this const T& self, std::ostream& os) {
    if constexpr (IsShapeStructure<T>) {
      self.print(os);
    } else if constexpr (IsShape<T>) {
      using ::print;
      print(self, os);
    } else {
      std::cout << "[Printing Nothing]" << std::endl;
    }
  };

  std::string Format(this const T& self) {
    if constexpr (IsShapeStructure<T>) {
      return self.Format();
    } else if constexpr (IsShape<T>) {
      using ::Format;
      return Format(self);
    } else {
      return "BaseObject";
    }
  };

  // std::unique_ptr<Shape::ShapeConcept> clone(this const T& self) {
  //   return self.clone();
  // };

  friend std::ostream& operator<<(std::ostream& os, const ShapeBase& shape) {
    shape.print(os);
    return os;
  }
  friend std::ostream& operator<<(std::ostream& os, ShapeBase&& shape) {
    shape.print(os);
    return os;
  }
};

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
  friend void serialize<>(const Shape& shape);
  friend void draw<>(const Shape& shape);
  friend std::string Format<>(const Shape& shape);
  
  friend std::ostream& operator<<(std::ostream& os, const Shape& shape) {
    return os << *shape.pimpl_;
  }

 public:
  // The External Polymorphism Design Pattern
  class ShapeConcept {
   public:
    constexpr virtual ~ShapeConcept() {}
    constexpr virtual void serialize() const = 0;
    constexpr virtual void draw() const = 0;
    constexpr virtual void print(std::ostream& os) const = 0;
    constexpr virtual std::string Format() const = 0;
    // The Prototype Design Pattern
    constexpr virtual std::unique_ptr<ShapeConcept> clone() const = 0;

    constexpr virtual std::list<Shape>& branches() = 0;
    constexpr virtual std::type_index typeidx() const = 0;

    friend std::ostream& operator<<(std::ostream& os,
                                    const ShapeConcept& shape) {
      shape.print(os);
      return os;
    }
  };

  template <class T>
  class ShapeModel : public ShapeConcept {
   private:
    friend Shape;
    T object_;
    std::list<Shape> branches_;
   public:
    constexpr ShapeModel(const T& value) : object_{value} {}

    void serialize() const override {
      // CAUTION: The using declaration tells the compiler to look up the free
      // serialize() function rather than the member function.
      //
      // Reference: https://stackoverflow.com/a/32091297/4475887

      if constexpr (IsShapeStructure<T>) {
        object_.serialize();
      } else if constexpr (IsShape<T>) {
        using ::serialize;
        serialize(object_);
      } else if constexpr (std::is_base_of_v<ShapeBase<T>, T>) {
        static_cast<const ShapeBase<T>&>(object_).serialize();
      } else {
        static_assert(IsShape<T> || IsShapeStructure<T>,
                      "T must be a Shape or a Shape Structure.");
      }
    }

    void draw() const override {
      if constexpr (IsShapeStructure<T>) {
        object_.draw();
      } else if constexpr (IsShape<T>) {
        using ::draw;
        draw(object_);
      } else if constexpr (std::is_base_of_v<ShapeBase<T>, T>) {
        static_cast<const ShapeBase<T>&>(object_).draw();
      } else {
        static_assert(IsShape<T> || IsShapeStructure<T>,
                      "T must be a Shape or a Shape Structure.");
      }
    }

    constexpr std::string Format() const override {
      if constexpr (requires(T t) {
                      { t.Format() } -> std::same_as<std::string>;
                    }) {
        std::string ret = object_.Format();
        for (const auto& branch : branches_) {
          ret += branch.pimpl_->Format();
        }
        return ret;
      } else if constexpr (IsShape<T>) {
        using ::Format;
        return Format(object_);
      } else if constexpr (std::is_base_of_v<ShapeBase<T>, T>) {
        return static_cast<const ShapeBase<T>&>(object_).Format();
      } else {
        static_assert(IsShape<T> || IsShapeStructure<T>,
                      "T must be a Shape or a Shape Structure.");
      }
    }

    void print(std::ostream& os) const override { os << object_; }

    // The Prototype Design Pattern
    constexpr std::unique_ptr<ShapeConcept> clone() const override {
      return std::make_unique<ShapeModel>(*this);
    }

    constexpr std::list<Shape>& branches() override { return branches_; }
    constexpr std::type_index typeidx() const override { return typeid(T); }
  };

 private:
  // The Bridge Design Pattern
  std::unique_ptr<ShapeConcept> pimpl_;

 public:
  template <class T>
   constexpr std::vector<Shape*> get_all_of() {
    std::vector<Shape*> result;
    if (is<T>()) {
      result.push_back(this);
    }
    for (auto& branch : pimpl_->branches()) {
      auto branch_result = branch.get_all_of<T>();
      result.insert(result.end(), branch_result.begin(), branch_result.end());
    }
    return result;

  }

  template <class T>
  constexpr T& as() {
    return static_cast<ShapeModel<T>&>(*pimpl_).object_;
  }

  template <class T>
  constexpr bool is() {
    return pimpl_->typeidx() == typeid(T);
  }

  // A constructor template to create a bridge.
  template <class T>
  Shape(const T& x) : pimpl_{new ShapeModel<T>(x)} {}

  Shape(const Shape& s) : pimpl_{s.pimpl_->clone()} {}

  Shape(Shape&& s) : pimpl_{std::move(s.pimpl_)} {}

  Shape& operator=(const Shape& s) {
    pimpl_ = s.pimpl_->clone();
    return *this;
  }

  Shape& operator=(Shape&& s) {
    pimpl_ = std::move(s.pimpl_);
    return *this;
  }

  void emplace_back(const Shape& s) { pimpl_->branches().emplace_back(s);
  }

  void emplace_back(Shape&& s) {
    pimpl_->branches().emplace_back(std::forward<Shape>(s));
  }

  void serialize() const {
    // CAUTION: The using declaration tells the compiler to look up the free
    // serialize() function rather than the member function.
    //
    // Reference: https://stackoverflow.com/a/32091297/4475887
    pimpl_->serialize();
  }

  void draw() const {
    pimpl_->draw();
  }

  std::string Format() const {
    return pimpl_->Format();
  }

  void print(std::ostream& os) const { os << *this; }
};

template <>
void serialize(const Shape& shape) {
  shape.pimpl_->serialize();
}

template <>
void draw(const Shape& shape) {
  shape.pimpl_->draw();
}

template <>
std::string Format(const Shape& shape) {
  return shape.pimpl_->Format();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* User Code */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Circle impl
class Circle {
  double radius_;

 public:
  constexpr explicit Circle(double radius) : radius_(radius) {}

  double radius() const { return radius_; }

  friend std::ostream& operator<<(std::ostream& os, const Circle& circle) {
    return os << "Circle(radius = " << circle.radius() << ")";
  }

  void serialize() const {
    std::cout << "Serializing a Circle: " << *this << std::endl;
  }

  void draw() const {
    std::cout << "Drawing a Circle: \n" << this->Format() << std::endl;
  }

  std::string Format() const {
    // - Iterate through the grid from -radius to +radius for both x and y
    // - Check if the point (x, y) is close enough to the circle's equation
    // (scaled for width)
    // - Append star for a circle point
    // - Otherwise, append space
    // - Add a newline after each row
    std::string out{};
    for (int y = -radius(); y <= radius(); ++y) {
      for (int x = -2 * radius(); x <= 2 * radius(); ++x) {
        if (std::abs(x * x / 4 + y * y - radius() * radius()) <= radius())
          out += "*";
        else
          out += " ";
      }
      out += "\n";
    }

    return out;
  }
};

// Square impl
class Square {
  double width_;

 public:
  explicit Square(double width) : width_(width) {}

  double width() const { return width_; }
};

std::ostream& operator<<(std::ostream& os, const Square& square);
void serialize(const Square& square);
void draw(const Square& square);
std::string Format(const Square& square);

std::ostream& operator<<(std::ostream& os, const Square& square) {
  return os << "Square(width = " << square.width() << ")";
}

void serialize(const Square& square) {
  std::cout << "Serializing a Square: " << square << std::endl;
}

void draw(const Square& square) {
  std::cout << "Drawing a Square: \n" << Format(square) << std::endl;
}

std::string Format(const Square& square) {
  const char TOP_LEFT = 0xC9;      // ASCII: 201
  const char TOP_RIGHT = 0xBB;     // ASCII: 187
  const char BOTTOM_LEFT = 0xC8;   // ASCII: 200
  const char BOTTOM_RIGHT = 0xBC;  // ASCII: 188
  const char HORIZONTAL = 0xCD;    // ASCII: 205
  const char VERTICAL = 0xB3;      // ASCII: 186
  std::stringstream box;
  box << TOP_LEFT;
  for (int i = 0; i < square.width() - 2; ++i) {
    box << HORIZONTAL;
  }
  box << TOP_RIGHT << "\n";

  for (int i = 0; i < square.width() - 2; ++i) {
    box << VERTICAL;
    for (int j = 0; j < square.width() - 2; ++j) {
      box << " ";
    }
    box << VERTICAL << "\n";
  }

  box << BOTTOM_LEFT;
  for (int i = 0; i < square.width() - 2; ++i) {
    box << HORIZONTAL;
  }
  box << BOTTOM_RIGHT << "\n";

  return box.str();
}

// Triangle impl
struct Triangle : ShapeBase<Triangle> {
  // using ShapeBase<Triangle>::serialize;
  // using ShapeBase<Triangle>::draw;
  // using ShapeBase<Triangle>::Format;
  // using ShapeBase<Triangle>::print;

  double size{};

  Triangle() {}

  std::string Format() const {
    std::string ret{""};
    for (int i = 0; i < size; ++i) {
      ret+= std::string(size - i - 1, ' ');
      ret += std::string(2 * i + 1, '*');
      ret += '\n';
    }
    return ret;
  }

  Triangle(double size) : size(size) {}
  friend std::ostream& operator<<(std::ostream& os, const Triangle& shape) {
    // shape.print(os);
    return os;
  }
};

static int DeduceThisImpl() {
  // Node-based type erased object. Can be any object conforming to the required interface
  // or any object which implements a static interface, or any object which inherits from
  // ShapeBase<T>. This gives the user the most felixbility possible in the way they can
  // implement an object.

  // Circle is a structural implementation of a shape
  Shape circle{Circle{5.0}};

  // An object may recursivley contain other objects.
  circle.emplace_back(Square{10.0});
  circle.emplace_back(Triangle{10.0});

  // The object may be stored in a container.
  std::vector<Shape> shapes;
  shapes.emplace_back(Circle{5.0});
  shapes.back().emplace_back(Square{10.0});
  shapes.back().emplace_back(Triangle{10.0});
  shapes.back().emplace_back(Circle{10.0});

  // The shape type is erased, but it can still be queried for.
  if (shapes.back().is<Circle>()) {
    std::cout << "The last shape is a Circle." << std::endl;
  } else {
    std::cout << "The last shape is NOT a Circle." << std::endl;
  }

  // Retrieve a reference to the underlying stored original object.
  std::cout << shapes.back().as<Circle>().radius() << std::endl;

  // Find an object based on it's type.
  auto all_circles = shapes.back().get_all_of<Circle>();
  assert(all_circles.size() == 2 &&
         "There should be 2 circles counter, the root and one branch.");

  // Add an 'interface' based user implementation of a shape.
  shapes.emplace_back(Square{10.0});

  // Add a shape which inherits from ShapeBase<T> and implements a partial interface.
  shapes.emplace_back(Triangle{10.0});


  for (const auto& shape : shapes) {
    //serialize(shape);
    //draw(shape);
    std::cout << Format(shape); // Format is applied recursivley here, all branches of the shape are also included.
    //shape.serialize();
    //shape.draw();
  }

  // for (const auto& shape_view : shape_views) {
  //   serialize(shape_view);
  //   draw(shape_view);
  //   std::cout << Format(shape_view);
  //   shape_view.serialize();
  //   shape_view.draw();
  // }
  return 0;
}