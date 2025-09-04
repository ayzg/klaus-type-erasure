# Anton's Silver Bullet
## WIP
An in-depth analysis and application of Klaus Iglberger's C++ type erasure pattern. 
Along with practial extensions to the pattern using C++23 features.

The concept is a hypothetical case: You are a library maintainer of an ASCII rendering engine, who decided use Klaus's type erasure to allow users to draw
diffrent shapes to console. Your customers(library users) are now asking for more features in terms of flexibility. What can be done to extend the 
pattern for practical applications?

### Original Impl Interface
```
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
```

### Extended Impl Interface
```
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
```


**Output**
```
Drawing: class Circle
      *********
   ****       ****
 ***             ***
***               ***
**                 **
**                 **
**                 **
***               ***
 ***             ***
   ****       ****
      *********

Drawing: class Square
╔════════╗
│        │
│        │
│        │
│        │
│        │
│        │
│        │
│        │
╚════════╝

Drawing: class Triangle
         *
        ***
       *****
      *******
     *********
    ***********
   *************
  ***************
 *****************
*******************

Drawing: struct Pyramid

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

Drawing: struct Bat

                 _..__.          .__.._
               .^"-.._ '-(\__/)-' _..-"^.
                      '-.' oo '.-'
                         `-..-'  fsc

Drawing: struct Husky
[X:0|Y:0]

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

Drawing: struct IndirectShape<struct Bat>
[X:0|Y:0]

                 _..__.          .__.._
               .^"-.._ '-(\__/)-' _..-"^.
                      '-.' oo '.-'
                         `-..-'  fsc

Drawing: struct IndirectShape<class Circle>
[X:0|Y:0]
      *********
   ****       ****
 ***             ***
***               ***
**                 **
**                 **
**                 **
***               ***
 ***             ***
   ****       ****
      *********

******* Drawing All Animals *******

                 _..__.          .__.._
               .^"-.._ '-(\__/)-' _..-"^.
                      '-.' oo '.-'
                         `-..-'  fsc

[X:0|Y:0]

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

[X:0|Y:0]

                 _..__.          .__.._
               .^"-.._ '-(\__/)-' _..-"^.
                      '-.' oo '.-'
                         `-..-'  fsc


```
