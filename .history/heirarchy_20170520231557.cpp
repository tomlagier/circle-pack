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
    Node node;

    do {
        node = nodes.front();
        nodes.pop_front();
        if (!node.children.empty()) {
            for (int i = 0;  i < node.children.size(); i++) {
                nodes.push_back(node.children[i]);
            }
        }
    } while (!nodes.empty());
}