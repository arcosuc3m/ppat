all: test01
test01:
	${CXX} t01.cpp -Xclang -ast-dump -fsyntax-only
test02:
	${CXX} test02.cpp  -std=c++1y -Xclang -ast-dump -fsyntax-only
test03:
	${CXX} test03.cpp  -std=c++1y -Xclang -ast-dump -fsyntax-only
test04:
	${CXX} test04.cpp  -std=c++1y -Xclang -ast-dump -fsyntax-only


