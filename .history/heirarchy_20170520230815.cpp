#include <functional>
#include <vector>
#include <deque>

class Node {
    public:
      std::vector<Node> children;
};

Node eachBefore(
    Node n,
    std::function<void(Node)> callback
) {
    std::deque<Node> nodes(1, n);
    while(c = ) {

    }
}
