#include <iostream>
#include <string>

class MyClass {
private:
    char* name_{nullptr};

public:
    MyClass() : name_(nullptr) {}
    MyClass(char* name) : name_(name) {}
    ~MyClass() = default;

    void SetName(const char* name) {
        name_ = const_cast<char*>(name);
    }
    char* GetName() {
        return name_;
    }
};

int main() {
    std::string name = "myname";
    MyClass obj;
    obj.SetName(name.c_str());
    std::cout << obj.GetName() << std::endl;

    return 0;
}
