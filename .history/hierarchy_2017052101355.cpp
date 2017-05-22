#include <functional>
#include <vector>
#include <deque>

class Node {
    public:
        Node() {}

        Node(Node* children)
        : children(children) {}

        Node* children;
        Node* getChildren() const {
            return children;
        }

        void setChildren(Node* children_) {
            children = children_;
        }

        Node eachBefore(
            std::function<void(Node)> callback
        ) {
            std::deque<Node> nodes(1, *this);
            Node node;

            do {
                node = nodes.front();
                nodes.pop_front();
                callback(node);
                if (!node.children.empty())
                {
                    for (int i = 0;  i < node.children.size(); i++) {
                        nodes.push_back(node.children[i]);
                    }
                }
            } while (!nodes.empty());

            return *this;
        }
};