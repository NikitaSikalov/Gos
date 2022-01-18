#include <cstdint>
#include <queue>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <sstream>
#include <unordered_set>
#include <fstream>
#include <assert.h>

namespace logs_reader {
    struct Vertex;

    struct VertexQueueItem {
        VertexQueueItem(std::shared_ptr<Vertex> vertex, uint64_t count = 1) : vptr(vertex), count(count) {}

        std::shared_ptr<Vertex> vptr;
        uint64_t count{1};

        bool IsEmpty() const {
            return !count;
        }
    };

    class Vertex {
    public:
        enum class VertexType {
            START,
            REGULAR,
            END
        };

    public:
        Vertex(VertexType type) : type_(type) {}

        Vertex(uint64_t token_id) : token_id_(token_id) {}

    protected:
        Vertex() = default;

    public:
        void AddNextVertex(std::shared_ptr<Vertex> vertex);
        void AddQueueItem(const VertexQueueItem &);

        std::shared_ptr<Vertex> DequeueNextVertex();

        uint64_t GetTokenId();

        VertexType GetType();

        bool IsEmptyQueue();


        const std::queue<VertexQueueItem> &GetQueue();

        bool operator==(const Vertex &other) const {
            return token_id_ == other.token_id_ && type_ == other.type_;
        }

    private:
        const uint64_t token_id_{0};
        VertexType type_{VertexType::REGULAR};
        std::queue<VertexQueueItem> queue_;
    };

    class BagOfTokens {
    public:
        BagOfTokens() = default;

    public:
        uint64_t AddToken(const std::string &token);
        void InsertToken(const std::string &token, uint64_t token_id);

        uint64_t GetTokenId(const std::string &token) {
            return index_[token];
        }

        std::string GetTokenById(uint64_t token_id);

        const std::map<uint64_t, std::string>& GetReverseIndex() const;

    private:
        uint64_t counter_{0};
        std::map<std::string, uint64_t> index_;
        std::map<uint64_t, std::string> reverse_index_;
    };

    class TokensGraph {
    public:
        TokensGraph();

    public:
        void StartNewLogLine();

        void EndLogLine();

        void AddNextLogToken(const std::string &token);

        void InitReadMode();

        void DequeueToken();

        bool IsEndLogLine();

        bool IsEnd();

        std::string GetCurrentToken();

        std::vector<std::shared_ptr<Vertex>> GetTopSortedVertexes();
    public:
        void Store(const std::string &filename);

        void Load(const std::string &filename);

    private:
        std::map<uint64_t, std::vector<std::shared_ptr<Vertex>>> vertex_index_;
        std::shared_ptr<Vertex> start_vertex_;
        std::shared_ptr<Vertex> end_vertex_;
        std::shared_ptr<Vertex> current_vertex_;
        BagOfTokens tokens_index_;

    private:
        std::unordered_set<std::shared_ptr<Vertex>> used_vertexes_;

    private:
        std::shared_ptr<Vertex> GetNextVertex(uint64_t token_id);
    };

    class StreamLogsReader {
    public:
        StreamLogsReader(std::istream &logs_stream, TokensGraph *tokens_graph) : logs_stream_(logs_stream),
                                                                                 tokens_graph_(tokens_graph) {};
    public:
        void ReadLogLine();

        void ReadAll();

        bool IsEnd();

    private:
        std::istream &logs_stream_;
        TokensGraph *tokens_graph_;
    };

    class GraphLogsReader {
    public:
        GraphLogsReader(TokensGraph *graph) : tokens_graph_(graph) {
            graph->InitReadMode();
        }

    public:
        std::string ReadLogLine();

        bool IsEnd();

    private:
        TokensGraph *tokens_graph_;
    };
}
