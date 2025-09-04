#pragma once
#include <algorithm>
#include <concepts>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

template <>
void serialize(const Shape& shape);

template <>
void draw(const Shape& shape);

template <>
std::string Format(const Shape& shape);

// #endif  // __clang__

template <typename T>
concept IsShape = requires(T t) {
  serialize(t);
  draw(t);
  { std::declval<std::ostream&>() << t } -> std::same_as<std::ostream&>;
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

  // The External Polymorphism Design Pattern
  class ShapeConcept {
   public:
    virtual ~ShapeConcept() {}
    virtual void serialize() const = 0;
    virtual void draw() const = 0;
    virtual void print(std::ostream& os) const = 0;
    virtual std::string Format() const = 0;
    // The Prototype Design Pattern
    virtual std::unique_ptr<ShapeConcept> clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os,
                                    const ShapeConcept& shape) {
      shape.print(os);
      return os;
    }
  };

  template <IsShape T>
  class ShapeModel : public ShapeConcept {
    T object_;

   public:
    ShapeModel(const T& value) : object_{value} {}

    void serialize() const override {
      // CAUTION: The using declaration tells the compiler to look up the free
      // serialize() function rather than the member function.
      //
      // Reference: https://stackoverflow.com/a/32091297/4475887
      using ::serialize;

      serialize(object_);
    }

    void draw() const override {
      using ::draw;

      draw(object_);
    }

    std::string Format() const override {
      using ::Format;

      return Format(object_);
    }

    void print(std::ostream& os) const override { os << object_; }

    // The Prototype Design Pattern
    constexpr std::unique_ptr<ShapeConcept> clone() const override {
      return std::make_unique<ShapeModel>(*this);
    }
  };

  // The Bridge Design Pattern
  std::unique_ptr<ShapeConcept> pimpl_;

 public:
  // A constructor template to create a bridge.
  template <IsShape T>
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

class Circle {
  double radius_;

 public:
  explicit Circle(double radius) : radius_(radius) {}

  double radius() const { return radius_; }
};

std::ostream& operator<<(std::ostream& os, const Circle& circle);
void serialize(const Circle& circle);
void draw(const Circle& circle);

std::ostream& operator<<(std::ostream& os, const Circle& circle) {
  return os << "Circle(radius = " << circle.radius() << ")";
}

void serialize(const Circle& circle) {
  std::cout << "Serializing a Circle: " << circle << std::endl;
}

void draw(const Circle& circle) {
  std::cout << "Drawing a Circle: " << circle << std::endl;
}

std::string Format(const Circle& circle) {
  // - Iterate through the grid from -radius to +radius for both x and y
  // - Check if the point (x, y) is close enough to the circle's equation
  // (scaled for width)
  // - Append star for a circle point
  // - Otherwise, append space
  // - Add a newline after each row
  std::stringstream out;
  for (int y = -circle.radius(); y <= circle.radius(); ++y) {
    for (int x = -2 * circle.radius(); x <= 2 * circle.radius(); ++x) {
      if (std::abs(x * x / 4 + y * y - circle.radius() * circle.radius()) <=
          circle.radius())
        out << "*";
      else
        out << " ";
    }
    out << "\n";
  }

  return out.str();
}

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
  std::cout << "Drawing a Square: " << square << std::endl;
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

static int OriginalImpl() {
  Shape circle{Circle{5.0}};
  std::vector<Shape> shapes;
  shapes.emplace_back(Circle{5.0});
  shapes.emplace_back(Square{10.0});

  for (const auto& shape : shapes) {
    serialize(shape);
    draw(shape);
    std::cout << Format(shape);
  }

  return 0;
}