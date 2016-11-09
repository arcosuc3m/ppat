#include <iostream>
#include <vector>
#include <fstream>
//#define MAX 1000000
#define MAX 10000000

/* with cout - cannot be parallelize */
void doBadStuff(int i) {
  int x = i * i + 2 * i;
  int y = x / 2 + x % 2;
  double z =  (x+y)/7.0 + (x+i)/7.0;
  double w = z * z / 2.0;
  w += w;
  std::cout << i << std::endl;
}

/* without cout - can be parallelize */
void doStuff(int i) {
  int x = i * i + 2 * i;
  int y = x / 2 + x % 2;
  double z =  (x+y)/7.0 + (x+i)/7.0;
  double w = z * z / 2.0;
  w += w;
}

int main() {
  unsigned int i = 0;
  // Test1 - Test brackets work
  while ( ++i < MAX)
    doStuff(i);

  // Test2 - No possible to paralellize
  i = 0;
  while ( i < MAX) {
    doBadStuff(i);
    i++;
  }

  // Test3 - Function content outside function
  i = 0;
  while ( i < MAX) {
    int x = i * i + 2 * i;
    int y = x / 2 + x % 2;
    double z =  (x+y)/7.0 + (x+i)/7.0;
    double w =  z * z / 2.0;
    w += w;
    std::cout << i << std::endl;
    ++i;
  }

  // Test4 - Vector as stream
  std::vector<int> v(MAX);
  for (i = 0; i < MAX; ++i) 
    v[i] = 1;

  auto it = v.begin();
  while ( it < v.end() ) {
    std::cout << *it << std::endl;
    doStuff(*it - 1);
    int x = *it;
    it++;
    *it += x;
  }

#define BUFF_SIZE 100
  // Test6 - File test with function
  std::ifstream test{"test.txt"};
  while ( test >> i ) {
    doStuff(i);
  }
  test.close();

  // Test7 - File test with sequential function (cout)
  test.open("test.txt");
  while ( test >> i ) {
    std::cout << i << std::endl;
  }
  test.close();

  // Test8 - Update outside the while header
  test.open("test.txt");
  test >> i;
  while ( i ) {
    std::cout << i << std::endl;
	  test >> i;
  }
  test.close();

  // Test9 - too many temporal variables
  test.open("test.txt");
  test >> i;
  while ( i ) {
    std::cout << i << std::endl;
		int x;
	  test >> x;
    int y = x;
    i = y;
  }
  test.close();
}
