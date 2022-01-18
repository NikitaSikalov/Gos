#include <iostream>

#include "logs_graph.h"

int main(int arc, char* argv[]) {
    using namespace logs_reader;

    TokensGraph graph;
    graph.Load(argv[1]);

    std::ofstream output(argv[2]);
    GraphLogsReader graph_reader(&graph);
    while (!graph_reader.IsEnd()) {
         output << graph_reader.ReadLogLine() << std::endl;
     }

    return 0;
}
