#pragma once
#include <algorithm>
#include <cmath>
#include "document.h"
#include <iostream>
#include <map>
#include <numeric>
#include "read_input_functions.h"
#include "string_processing.h"
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>


using namespace std::string_literals;

class SearchServer {
public:
    // Вместо SetStopWords

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        //Конструктор класса SearchServer выбрасывать исключение
        // invalid_argument если любое из переданных стоп-слов содержит недопустимые символы
        for(const auto& stop_word: stop_words_){
            if(IsValidWord(stop_word) == false){
                throw std::invalid_argument("invalid word in class constructor");
            }
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
            : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }
// функция добавления слов поискового запроса (без стоп-слов) в documents_
//Метод AddDocument выбрасывет исключение invalid_argument в следующих ситуациях:
    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                                   const std::vector<int>& ratings);
    // Фильтрация документов должна производиться до отсечения топа из пяти штук.
    // функция вывода 5 наиболее релевантных результатов из всех найденных

    template<typename predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, predicate predict) const;

    std::vector<Document>  FindTopDocuments(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
//Метод должен возвращать количество документов в поисковой системе.
    int GetDocumentCount() const {
        return documents_.size();
    }
//В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
// Слова не должны дублироваться. Отсортированы по возрастанию.
// Если нет пересечений по плюс-словам или есть минус-слово, вектор слов вернуть пустым.
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
                                                        int document_id)
    // Если документ не соответствует запросу(нет пересечений по плюс - словам
    // или есть минус - слово), вектор слов нужно вернуть пустым.
    const;

    int GetDocumentId(const int index) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids; //для хранения айдишников

    bool IsStopWord(const std::string& word) const;

// функция считывания слов поискового запроса и удаления из него стоп-слов (считывание плюс-слов)
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    // обработка минус-слов запроса
    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    // Existence required
    // вычисляем IDF - делим количество документов
    // где встречается слово на количество всех документов и берём нат.логарифм
    double ComputeWordInverseDocumentFreq(const std::string& word) const;
// функция вывода ВСЕХ найденных результатов по релевантности по формуле TF-IDF

    template<typename DocPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocPredicate doc_pred) const;

    static bool IsValidWord(const std::string& word);
};
