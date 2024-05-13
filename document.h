#pragma once
#include <iostream>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

// объявление структуры документов с 3 переменными: id, relevance, rating
struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

// объявление перечисленных типов
enum class DocumentStatus {
    ACTUAL,        // актуальный
    IRRELEVANT,    // устаревший
    BANNED,        // отклонённый
    REMOVED        // удалённый
};