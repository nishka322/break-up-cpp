#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    if ((document_id < 0) || documents_.count(document_id)) {//Попытка добавить документ с отрицательным id;
        throw std::invalid_argument("Document id less then zero");
    }
    if (documents_.count(document_id)){//Попытка добавить документ c id ранее добавленного документа;
        throw std::invalid_argument("repeat document id");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids.push_back(document_id);
    //return 0;
}

template<typename predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, predicate predict) const {
    if (IsValidWord(raw_query) == false){
        throw std::invalid_argument("Invalid word in FindTopDocument function");
    }
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predict);
    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (abs(lhs.relevance - rhs.relevance) < std::numeric_limits<double>::epsilon()) { //Избавились от магических чисел через стандартный std::numeric_limits<double>::epsilon()
                 return lhs.rating > rhs.rating;
             } //else {  else не обязателен, т.к. используется return
             return lhs.relevance > rhs.relevance;
             //}
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

std::vector<Document>  SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const{ //Если тут задать статус по умолчанию, то FindTopDocuments(const string& raw_query) будет не нужен
    if (IsValidWord(raw_query) == false){
        throw std::invalid_argument("Invalid word in FindTopDocument function");
    }
    auto predict = [status](int document_id, DocumentStatus doc_status, int rating) {
        return doc_status == status;
    };
    return FindTopDocuments(raw_query, predict);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                   int document_id)
// Если документ не соответствует запросу(нет пересечений по плюс - словам
// или есть минус - слово), вектор слов нужно вернуть пустым.
const {
    if (IsValidWord(raw_query) == false){
        throw std::invalid_argument("Invalid word in MatchDocument function");
    }
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    auto result = std::tuple{matched_words, documents_.at(document_id).status};
    return result;
}

int SearchServer::GetDocumentId(const int index) const {
    return document_ids.at(index);
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (IsValidWord(word) == false) {//Наличие недопустимых символов (с кодами от 0 до 31) в тексте добавляемого документа.
            throw std::invalid_argument("Invalid word");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    // Use std::accumulate вместо цикла
    if (ratings.empty()) return 0;
    return accumulate(begin(ratings), end(ratings), 0)
           / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    QueryWord result;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (IsValidWord(text) == false) {//Наличие недопустимых символов (с кодами от 0 до 31) в тексте добавляемого документа.
        throw std::invalid_argument("Invalid word. Words ASCII 0-31.");
    }
    if (text.empty() || text[0] == '-') {//.empty отсекает "кот -", text[0] == '-' отсекает "--кот"
        throw std::invalid_argument("Invalid minus word in ParseQueryWord"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

template<typename DocPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocPredicate doc_pred) const  {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_info = documents_.at(document_id);
            if (doc_pred(document_id, document_info.status, document_info.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

    bool SearchServer::IsValidWord(const std::string& word) {//проверка слова на наличие спецсимволов
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}
