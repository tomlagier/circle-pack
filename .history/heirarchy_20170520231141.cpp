#include <functional>
#include <vector>
#include <deque>

class Node {
    public:
      std::vector<Node> children;
};

Node eachBefore(
    Node root,
    std::function<void(Node)> callback
) {
    std::deque<Node> nodes(1, root);
    Node current;

    do {
        current = nodes.front();
        nodes.pop_front();
        if (current.child)
    } while (!nodes.empty());
}
