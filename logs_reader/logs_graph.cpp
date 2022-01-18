#include "logs_graph.h"
#include "proto/graph.pb.h"

namespace {
    using namespace logs_reader;

    Vertex::VertexType GetVertexType(const serializable::Vertex &vertex) {

        if (vertex.type() == serializable::Vertex_VertexType_START) {
            return Vertex::VertexType::START;
        }

        if (vertex.type() == serializable::Vertex_VertexType_REGULAR) {
            return Vertex::VertexType::REGULAR;
        }

        return Vertex::VertexType::END;
    }

    serializable::Vertex::VertexType SerializeVertexType(Vertex::VertexType type) {
        if (type == Vertex::VertexType::START) {
            return serializable::Vertex_VertexType_START;
        }

        if (type == Vertex::VertexType::END) {
            return serializable::Vertex_VertexType_END;
        }

        return serializable::Vertex_VertexType_REGULAR;
    }
}

namespace {
    using namespace logs_reader;

    void TopSort(std::vector<std::shared_ptr<Vertex>> *vertexes, std::shared_ptr<Vertex> v,
                 std::unordered_set<Vertex *>* used) {

        used->insert(v.get());
        auto queue = v->GetQueue();
        while (!queue.empty()) {
            auto item = queue.front();
            assert(!item.IsEmpty());
            queue.pop();

            auto neighbour = item.vptr;
            if (used->find(neighbour.get()) != used->end()) {
                continue;
            }
            TopSort(vertexes, neighbour, used);
        }

        vertexes->push_back(v);
    }
}

namespace logs_reader {
    void Vertex::AddNextVertex(std::shared_ptr<Vertex> vertex) {
        if (!queue_.empty() && *vertex == *queue_.back().vptr) {
            queue_.back().count++;
            return;
        }

        queue_.emplace(vertex);
    }

    bool Vertex::IsEmptyQueue() {
        while (!queue_.empty() && queue_.front().IsEmpty()) {
            queue_.pop();
        }
        return queue_.empty();
    }

    Vertex::VertexType Vertex::GetType() {
        return type_;
    }

    const std::queue<VertexQueueItem> & Vertex::GetQueue() {
        return queue_;
    }

    uint64_t Vertex::GetTokenId() {
        return token_id_;
    }

    void Vertex::AddQueueItem(const VertexQueueItem & queue_item) {
        queue_.push(queue_item);
    }

    std::shared_ptr<Vertex> Vertex::DequeueNextVertex() {
        if (queue_.front().IsEmpty()) {
            queue_.pop();
        }
        VertexQueueItem &item = queue_.front();
        assert(!item.IsEmpty());
        --item.count;

        return item.vptr;
    }

    uint64_t BagOfTokens::AddToken(const std::string &token) {
        if (index_.find(token) != index_.end()) {
            return GetTokenId(token);
        }

        const uint64_t token_id = counter_++;

        // Check that reverse index does not contain this token
        assert(reverse_index_.find(token_id) == reverse_index_.end());
        // Overflow check
        assert(counter_);

        index_[token] = token_id;
        reverse_index_[token_id] = token;
        return token_id;
    }

    const std::map<uint64_t, std::string>& BagOfTokens::GetReverseIndex() const {
        return reverse_index_;
    }

    void BagOfTokens::InsertToken(const std::string &token, uint64_t token_id) {
        index_[token] = token_id;
        reverse_index_[token_id] = token;
    }

    std::string BagOfTokens::GetTokenById(uint64_t token_id) {
        return reverse_index_[token_id];
    }

    TokensGraph::TokensGraph() {
        start_vertex_ = std::make_shared<Vertex>(Vertex::VertexType::START);
        end_vertex_ = std::make_shared<Vertex>(Vertex::VertexType::END);
        current_vertex_ = start_vertex_;
    }

    void TokensGraph::StartNewLogLine() {
        current_vertex_ = start_vertex_;
        used_vertexes_.clear();
    }

    void TokensGraph::EndLogLine() {
        current_vertex_->AddNextVertex(end_vertex_);
        current_vertex_ = end_vertex_;
    }

    void TokensGraph::AddNextLogToken(const std::string &token) {
        assert(current_vertex_ != end_vertex_);

        uint64_t token_id = tokens_index_.AddToken(token);
        std::shared_ptr<Vertex> next_vertex = GetNextVertex(token_id);
        current_vertex_->AddNextVertex(next_vertex);
        current_vertex_ = next_vertex;
    }

    std::shared_ptr<Vertex> TokensGraph::GetNextVertex(uint64_t token_id) {
        auto &vertexes = vertex_index_[token_id];
        for (auto &vertex : vertexes) {
            if (used_vertexes_.find(vertex) == used_vertexes_.end()) {
                used_vertexes_.insert(vertex);
                return vertex;
            }
        }

        std::shared_ptr new_vertex = std::make_shared<Vertex>(token_id);
        vertex_index_[token_id].push_back(new_vertex);
        used_vertexes_.insert(new_vertex);

        return new_vertex;
    }

    void TokensGraph::InitReadMode() {
        StartNewLogLine();
    }

    bool TokensGraph::IsEnd() {
        return start_vertex_->IsEmptyQueue();
    }

    bool TokensGraph::IsEndLogLine() {
        return current_vertex_ == end_vertex_;
    }

    void TokensGraph::DequeueToken() {
        current_vertex_ = current_vertex_->DequeueNextVertex();
    }

    std::string TokensGraph::GetCurrentToken() {
        assert(current_vertex_ != start_vertex_);
        assert(current_vertex_ != end_vertex_);

        return tokens_index_.GetTokenById(current_vertex_->GetTokenId());
    }

    std::vector<std::shared_ptr<Vertex>> TokensGraph::GetTopSortedVertexes() {
        std::vector<std::shared_ptr<Vertex>> vertexes;
        std::unordered_set<Vertex*> used;
        TopSort(&vertexes, start_vertex_, &used);

        return vertexes;
    }

    void TokensGraph::Store(const std::string &filename) {
        std::ofstream out(filename);

        serializable::TokensGraph graph;

        std::vector<std::shared_ptr<Vertex>> top_sorted_vertexes = GetTopSortedVertexes();

        for (const auto vertex : top_sorted_vertexes) {
            uint64_t vertex_id = reinterpret_cast<uint64_t>(vertex.get());
            serializable::Vertex *vertex_msg = graph.add_vertexes();
            vertex_msg->set_token_id(vertex->GetTokenId());
            vertex_msg->set_id(vertex_id);
            vertex_msg->set_type(SerializeVertexType(vertex->GetType()));

            std::queue<VertexQueueItem> queue = vertex->GetQueue();
            while (!queue.empty()) {
                auto item = queue.front();
                queue.pop();
                auto* neighbour = vertex_msg->add_neighbours();
                uint64_t nid = reinterpret_cast<uint64_t>(item.vptr.get());
                neighbour->set_id(nid);
                neighbour->set_count(item.count);
            }
        }

        for (const auto&[token_id, token] : tokens_index_.GetReverseIndex()) {
            auto &tokens = *graph.mutable_tokens();
            tokens[token_id] = token;
        }

        graph.SerializeToOstream(&out);
    }

    void TokensGraph::Load(const std::string &filename) {
        std::ifstream input(filename);

        serializable::TokensGraph graph;
        graph.ParseFromIstream(&input);

        std::map<uint64_t, std::shared_ptr<Vertex>> vertex_table;
        for (int i = 0; i < graph.vertexes_size(); ++i) {
            auto &vertex_serializable = graph.vertexes(i);
            uint64_t vertex_id = vertex_serializable.id();
            uint64_t token_id = vertex_serializable.token_id();
            auto vtype = GetVertexType(vertex_serializable);

            std::shared_ptr<Vertex> vertex = vtype == Vertex::VertexType::START || vtype == Vertex::VertexType::END
                                             ? std::make_shared<Vertex>(vtype)
                                             : std::make_shared<Vertex>(token_id);

            if (vtype == Vertex::VertexType::START) {
                start_vertex_ = vertex;
            }

            if (vtype == Vertex::VertexType::END) {
                end_vertex_ = vertex;
            }

            vertex_table[vertex_id] = vertex;

            for (int j = 0; j < vertex_serializable.neighbours_size(); ++j) {
                auto neighbour = vertex_serializable.neighbours(j);
                uint64_t nid = neighbour.id();
                uint64_t count = neighbour.count();

                assert(vertex_table.find(nid) != vertex_table.end());

                VertexQueueItem queue_item(vertex_table[nid], count);
                vertex->AddQueueItem(queue_item);
            }

            vertex_index_[token_id].push_back(vertex);
        }

        for (int i = 0; i < graph.tokens_size(); ++i) {
            const auto &tokens_index = graph.tokens();
            for (const auto&[token_id, token] : tokens_index) {
                tokens_index_.InsertToken(token, token_id);
            }
        }
    }

    void StreamLogsReader::ReadLogLine() {
        std::string line;
        std::getline(logs_stream_, line);
        std::istringstream tokens_stream(line);

        tokens_graph_->StartNewLogLine();
        while (!tokens_stream.eof()) {
            std::string token;
            tokens_stream >> token;

            if (token.empty()) {
                continue;
            }

            tokens_graph_->AddNextLogToken(token);
        }
        tokens_graph_->EndLogLine();
    }

    void StreamLogsReader::ReadAll() {
        while (!IsEnd()) {
            ReadLogLine();
        }
    }

    bool StreamLogsReader::IsEnd() {
        return logs_stream_.eof();
    }

    bool GraphLogsReader::IsEnd() {
        return tokens_graph_->IsEnd();
    }

    std::string GraphLogsReader::ReadLogLine() {
        std::string line;
        const std::string delim = " ";

        tokens_graph_->StartNewLogLine();
        tokens_graph_->DequeueToken();
        while (!tokens_graph_->IsEndLogLine()) {
            std::string token = tokens_graph_->GetCurrentToken();
            if (!line.empty()) {
                line += delim;
            }
            line += token;
            tokens_graph_->DequeueToken();
        }

        return line;
    }
}
