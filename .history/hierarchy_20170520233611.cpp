#include <functional>
#include <vector>
#include <deque>
#include <emscripten/bind.h>

using namespace emscripten;

extern "C" {

    class Node {
        public:
            Node() {}

            Node(std::vector<Node> children)
            : children(children) {}

            std::vector<Node> children;

            Node each_before(
                Node root,
                std::function<void(Node)> callback
            ) {
                std::deque<Node> nodes(1, root);
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

                return root;
            }
    };

    EMSCRIPTEN_BINDINGS(hierarchy) {
        class_<Node>("Node")
        .constructor<std::vector<Node>>()
        .property("children")
        .
        ;
    }
}