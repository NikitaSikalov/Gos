#include <iostream>

#include "logs_graph.h"

int main(int arc, char* argv[]) {
    using namespace logs_reader;

    std::ifstream input(argv[1]);
    TokensGraph graph;
    StreamLogsReader reader(input, &graph);

    reader.ReadAll();

    graph.Store(argv[2]);
    return 0;
}
