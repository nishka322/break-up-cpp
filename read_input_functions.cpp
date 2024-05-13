#include "read_input_functions.h"


// функция считывания слов
std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}
// функция считывания количества слов
int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}