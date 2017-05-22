#include <functional>

class Node {
    public:
      Node *children;
};

Node eachBefore(
    Node n,
    std::function<void(Node)> callback
) {

}
