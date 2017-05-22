#include <functional>
#include <vector>

class Node {
    public:
      std::vector<Node> children;
};

Node eachBefore(
    Node n,
    std::function<void(Node)> callback
) {
    std::vector<Node> nodes(1, n);

}
