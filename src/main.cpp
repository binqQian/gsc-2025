#include <iostream>
#include <string>

// 简单计算器函数
double calculate(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b != 0) return a / b;
            throw std::runtime_error("除数不能为0");
        default:
            throw std::runtime_error("不支持的运算符");
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cout << "使用方法: " << argv[0] << " <数字1> <运算符(+,-,*,/)> <数字2>" << std::endl;
            return 1;
        }

        double num1 = std::stod(argv[1]);
        char op = argv[2][0];
        double num2 = std::stod(argv[3]);

        double result = calculate(num1, num2, op);
        std::cout << num1 << " " << op << " " << num2 << " = " << result << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
